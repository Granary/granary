/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * mangle.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/mangle.h"
#include "granary/basic_block.h"
#include "granary/code_cache.h"
#include "granary/state.h"
#include "granary/detach.h"
#include "granary/register.h"
#include "granary/hash_table.h"
#include "granary/emit_utils.h"

#include "granary/dbl.h"
#include "granary/ibl.h"


extern "C" {
    extern uint16_t granary_bswap16(uint16_t);
    extern void granary_asm_direct_branch_template(void);
}


namespace granary {


    enum {
        MAX_NUM_POLICIES = 1 << mangled_address::NUM_MANGLED_BITS,
        HOTPATCH_ALIGN = 8
    };


    /// Hash table of previously constructed IBL entry stubs.
    static operand reg_target_addr; // arg1, rdi
    static operand reg_target_addr_16; // arg1_16, di


    STATIC_INITIALISE_ID(ibl_stub_table, {
        reg_target_addr = reg::arg1;
        reg_target_addr_16 = reg::arg1_16;
    })


    /// Make an IBL stub. This is used by indirect jmps, calls, and returns.
    /// The purpose of the stub is to set up the registers and stack in a
    /// canonical way for entry into the indirect branch lookup routine.
    void instruction_list_mangler::mangle_ibl_lookup(
        instruction_list &ibl,
        instruction in,
        instrumentation_policy target_policy,
        operand target,
        ibl_entry_kind ibl_kind,
        app_pc cti_addr
    ) throw() {

        int stack_offset(0);

        if(IBL_ENTRY_RETURN == ibl_kind) {

            // Kernel space: save `reg_target_addr` and load the return address.
            if(!REDZONE_SIZE) {
                FAULT; // Should not be calling this from kernel space.

            // User space: overlay the redzone on top of the return address,
            // and default over to our usual mechanism.
            } else {
                stack_offset = REDZONE_SIZE - 8;
            }

        // Normal user space JMP; need to protect the stack against the redzone.
        } else {
            stack_offset = REDZONE_SIZE;
        }

        // Note: CALL and RET instructions do not technically need to protect
        //       against the redzone, as CALLs implicitly clobber part of the
        //       stack, and RETs implicitly release part of it. However, for
        //       consistency in the ibl_exit_routines, we use this.

        // Shift the stack. If this is a return in user space then we shift if
        // by 8 bytes less than the redzone size, so that the return address
        // itself "extends" the redzone by those 8 bytes.
        if(stack_offset) {
            ibl.insert_before(in, lea_(reg::rsp, reg::rsp[-stack_offset]));
        }

        ibl.insert_before(in, push_(reg_target_addr));
        stack_offset += 8;

        // If this was a call, then the stack offset was also shifted by
        // the push of the return address
        if(IBL_ENTRY_CALL == ibl_kind) {
            stack_offset += 8;
        }

        // Adjust the target operand if it's on the stack
        if(dynamorio::BASE_DISP_kind == target.kind
        && dynamorio::DR_REG_RSP == target.value.base_disp.base_reg) {
            target.value.base_disp.disp += stack_offset;
        }

        // Make a "fake" basic block so that we can also instrument / mangle
        // the memory loads needed to complete this indirect call or jump.
        instruction_list tail_bb(INSTRUCTION_LIST_STUB);

        // Load the target address into `reg_target_addr`. This might be a
        // normal base/disp kind, or a relative address, or an absolute
        // address.
        if(dynamorio::REG_kind != target.kind) {

            bool mangled_target(false);

            // Something like: `CALL *%FS:0xF00` or `CALL *%FS:%RAX` or
            // `call *%FS:(%RAX);`.
            if(dynamorio::DR_SEG_FS == target.seg.segment
            || dynamorio::DR_SEG_GS == target.seg.segment) {

                // Leave as is.

            // Normal relative/absolute address.
            } else if(dynamorio::opnd_is_rel_addr(target)
                   || dynamorio::opnd_is_abs_addr(target)) {

                app_pc target_addr(target.value.pc);

                // Do an indirect load using abs address.
                if(is_far_away(target_addr, estimator_pc)) {
                    tail_bb.append(mangled(mov_imm_(
                        reg_target_addr,
                        int64_(reinterpret_cast<uint64_t>(target_addr)))));
                    target = *reg_target_addr;
                    mangled_target = true;
                }
            }

            tail_bb.append(mov_ld_(reg_target_addr, target));

            // Notify higher levels of instrumentation that might be doing
            // memory operand interposition that this insruction should not be
            // interposed on.
            if(mangled_target) {
                tail_bb.last().set_mangled();
            }

        // Target is in a register.
        } else if(reg_target_addr.value.reg != target.value.reg) {
            tail_bb.append(mov_ld_(reg_target_addr, target));
        }

        // Instrument the memory instructions needed to complete this CALL
        // or JMP.
        instruction tail_bb_end(tail_bb.append(label_()));
        bool tail_bb_changed(false);
        if(IBL_ENTRY_CALL == ibl_kind || IBL_ENTRY_JMP == ibl_kind) {
            instrumentation_policy tail_policy(policy);

            // Kill all flags so that the instrumentation can use them if
            // possible.
            if(IBL_ENTRY_CALL == ibl_kind) {
                tail_bb.append(mangled(popf_()));
            }

            // Make sure all other registers appear live.
            tail_bb.append(mangled(jmp_(instr_(tail_bb_end))));

            const unsigned old_size(tail_bb.length());

            tail_policy.instrument(cpu, bb, tail_bb);
            instruction_list_mangler sub_mangler(
                cpu, bb, tail_bb, stub_ls, policy, estimator_pc);
            sub_mangler.mangle();

            // Not quite a perfect test, but the idea here is that if the client
            // tool is actually instrumenting the tail_bb, then we want to
            // append it all onto the original basic block, but otherwise we'll
            // move it out of the code cache and into the stub area. The key
            // idea is that we don't want to have to do any alignment for
            // potentially hot-patchable instructions introduced into stub code.
            // At the same time, if code is injected into the stub but not
            // placed into the basic block, then we won't be able to related
            // that fault back to the client tools `handle_interrupt` function.
            if(old_size != tail_bb.length()) {
                tail_bb_changed = true;
            }
        }

        instruction ibl_stand_in;
        if(tail_bb_changed) {
            ibl_stand_in = ls.append(label_());
        }

        // Add the instructions back into the stub.
        for(instruction tail_in(tail_bb.first()), next_tail_in;
            tail_in.is_valid();
            tail_in = next_tail_in) {

            if(tail_in == tail_bb_end) {
                break;
            }

            next_tail_in = tail_in.next();
            tail_bb.remove(tail_in);

            if(tail_bb_changed) {
                ls.insert_before(ibl_stand_in, tail_in);
            } else {
                ibl.insert_before(in, tail_in);
            }
        }

        // Extend the basic block with the IBL lookup stub.
        ibl_lookup_stub(ibl, in, target_policy, cti_addr);
    }


    /// Add a direct branch slot; this is a sort of "formula" for direct
    /// branches that pushes two addresses and then jmps to an actual
    /// direct branch handler.
    void instruction_list_mangler::mangle_direct_cti(
        instruction in,
        operand target,
        instrumentation_policy target_policy
    ) throw() {

        app_pc target_pc(target.value.pc);
        app_pc detach_target_pc(nullptr);
        mangled_address am(target_pc, target_policy);

        // Keep the target as-is.
        if(is_code_cache_address(target_pc)
        || is_wrapper_address(target_pc)
        || is_gencode_address(target_pc)) {
            detach_target_pc = target_pc;
        }

        // First detach check: try to see if we should detach from our current
        // policy context, before any context conversion can happen.
        if(!detach_target_pc && target_policy.can_detach()){
            detach_target_pc = find_detach_target(
                target_pc, target_policy.context());
        }

        // Fall-through:
        //      1) Either the code cache / wrapper address is too far
        //         away, and so we depend on the later code for making a
        //         jump slot; or
        //      2) We need to figure out if we want to:
        //              i)   Instrument host code.
        //              ii)  Detach from host/app code.
        //              iii) Instrument app code.

        // If we're in application code and we want to automatically instrument
        // host code, then figure out if we can do that.
        if(!detach_target_pc
        && !policy.is_in_host_context()
        && is_host_address(target_pc)) {
            if(policy.is_host_auto_instrumented()) {
                target_policy.in_host_context(true);
                am = mangled_address(target_pc, target_policy);
            } else {
                detach_target_pc = target_pc;
            }
        }

        // Forcibly resolve the target policy to the instruction.
        in.set_policy(target_policy);

        // Otherwise, we're either in application or host code, and we may or
        // may not want to detach.
        //
        // This can be a fall-through from above, where we want to auto-
        // instrument the host, but there is a host-context detach point which
        // must be considered.
        if(!detach_target_pc) {
            detach_target_pc = find_detach_target(
                target_pc, target_policy.context());
        }

        // Okay, look in the code cache to see if we've already translated the
        // target code.
        if(!detach_target_pc) {
            detach_target_pc = code_cache::lookup(am.as_address);
        }

        // If this is a detach point then replace the target address with the
        // detach address. This can be tricky because the instruction might not
        // be a call/jmp (i.e. it might be a conditional branch)
        if(detach_target_pc) {

            if(is_far_away(estimator_pc, detach_target_pc)) {

                // TODO: convert to an alternative form in the case of a
                //       conditional branch.
                ASSERT(in.is_call() || in.is_jump());

                app_pc *slot = global_state::FRAGMENT_ALLOCATOR->allocate<app_pc>();
                *slot = detach_target_pc;

                // Regardless of return address transparency, a direct call to
                // a detach target *always* needs to be a call so that control
                // returns to the code cache.
                if(in.is_call()) {
                    in.replace_with(
                        mangled(call_ind_(absmem_(slot, dynamorio::OPSZ_8))));
                } else {
                    in.replace_with(
                        mangled(jmp_ind_(absmem_(slot, dynamorio::OPSZ_8))));
                }

            } else {
                in.set_cti_target(pc_(detach_target_pc));
                in.set_mangled();
            }

        } else {
            // Replace `in` with a DBL lookup stub.
            insert_dbl_lookup_stub(ls, stub_ls, in, am);
        }
    }


#if GRANARY_IN_KERNEL
    extern "C" {
        extern intptr_t NATIVE_SYSCALL_TABLE;
        extern intptr_t SHADOW_SYSCALL_TABLE;
    }
#endif


    /// Mangle an indirect control transfer instruction.
    void instruction_list_mangler::mangle_indirect_cti(
        instruction in,
        operand target,
        instrumentation_policy target_policy
    ) throw() {

        if(in.is_return()) {
            IF_PERF( perf::visit_mangle_return(); )
#if !CONFIG_ENABLE_DIRECT_RETURN
            if(!policy.return_address_is_in_code_cache()) {

                // TODO: handle RETn/RETf with a byte count.
                ASSERT(dynamorio::IMMED_INTEGER_kind != in.instr->u.o.src0.kind);

                mangle_ibl_lookup(
                    ls, in, target_policy, target,
                    IBL_ENTRY_RETURN, in.pc());
                ls.remove(in);
            }
#endif
            return;
        }

#if GRANARY_IN_KERNEL && CONFIG_INSTRUMENT_HOST
        // Linux-specific special case: Optimise for syscall entry points.
        // Note: We depend on sign-extension of the 32-bit displacement here.
        if(NATIVE_SYSCALL_TABLE && SHADOW_SYSCALL_TABLE
        && dynamorio::BASE_DISP_kind == target.kind
        && NATIVE_SYSCALL_TABLE == (intptr_t) target.value.base_disp.disp) {
            target.value.base_disp.disp = (int) SHADOW_SYSCALL_TABLE;
            in.set_cti_target(target);
            in.set_mangled();
            return;
        }
#endif

        // Indirect CALL.
        if(in.is_call()) {
            instruction call_target(stub_ls.append(label_()));
            instruction insert_point(stub_ls.append(label_()));

            IF_PERF( perf::visit_mangle_indirect_call(); )
            mangle_ibl_lookup(
                stub_ls, insert_point, target_policy, target,
                IBL_ENTRY_CALL, in.pc());

            ls.insert_before(in, mangled(call_(instr_(call_target))));
            ls.remove(in);

        // Indirect JMP.
        } else {
            IF_PERF( perf::visit_mangle_indirect_jmp(); )
            mangle_ibl_lookup(
                ls, in, target_policy, target,
                IBL_ENTRY_JMP, in.pc());
            ls.remove(in);
        }
    }


    /// Mangle a control-transfer instruction. This handles both direct and
    /// indirect CTIs.
    void instruction_list_mangler::mangle_cti(
        instruction in
    ) throw() {

        instrumentation_policy target_policy(in.policy());
        if(!target_policy) {
            target_policy = policy;
        }

        target_policy.begins_functional_unit(false); // Sane default.

        if(dynamorio::OP_iret == in.op_code()) {
            // TODO?
            return;

        } else if(in.is_return()) {

            ASSERT(dynamorio::OP_ret_far != in.op_code());

            target_policy.inherit_properties(policy, INHERIT_RETURN);
            target_policy.return_target(true);
            target_policy.indirect_cti_target(false);
            target_policy.in_host_context(false);
            target_policy.return_address_in_code_cache(false);

            // Forcibly resolve the policy. Unlike indirect CTIs, we don't
            // mark the target as host auto-instrumented. The protocol here is
            // that we don't want to auto-instrument host code on a return,
            // even if that behaviour is set within the policy.
            in.set_policy(target_policy);

            mangle_indirect_cti(
                in,
                operand(*reg::rsp),
                target_policy
            );

        } else {
            operand target(in.cti_target());

            if(in.is_call()) {
                target_policy.inherit_properties(policy, INHERIT_CALL);
                target_policy.return_address_in_code_cache(true);
                target_policy.begins_functional_unit(true);
            } else {
                target_policy.inherit_properties(policy, INHERIT_JMP);
                target_policy.begins_functional_unit(false);
            }

            // Direct CTI.
            if(dynamorio::opnd_is_pc(target)) {

                // Sane defaults until we resolve more info.
                target_policy.return_target(false);
                target_policy.indirect_cti_target(false);

                mangle_direct_cti(in, target, target_policy);

            // Indirect CTI.
            } else if(!dynamorio::opnd_is_instr(target)) {
                target_policy.return_target(false);
                target_policy.indirect_cti_target(true);

                // Note: Indirect JMPs are treated as beginning a functional
                //       unit (indirect tail-call), whereas we don't have enough
                //       information at runtime to make this judgement of
                //       direct jumps (potential direct tail-calls).
                target_policy.begins_functional_unit(true);

                // Tell the code cache lookup routine that if we switch to host
                // code that we can instrument it. The protocol here is that if
                // we aren't auto-instrumenting, and if the client
                // instrumentation marks a CTI as going to a host context, then
                // we will instrument it. If the CTI actually goes to app code,
                // then we auto-convert the policy to be in the app context. If
                // we are auto-instrumenting, then the behaviour is as if every
                // indirect CTI were marked as going to host code, and so we
                // do the right thing.
                if(target_policy.is_host_auto_instrumented()) {
                    target_policy.in_host_context(true);
                }

                // Forcibly resolve the policy.
                in.set_policy(target_policy);

                mangle_indirect_cti(
                    in,
                    target,
                    target_policy
                );

            // CTI to a label.
            } else {
                ASSERT(target_policy == policy);
            }
        }
    }


    void instruction_list_mangler::mangle_lea(
        instruction in
    ) throw() {
        if(dynamorio::REL_ADDR_kind != in.instr->u.o.src0.kind) {
            return;
        }

        // It's an LEA to a far address; convert to a 64-bit move.
        app_pc target_pc(in.instr->u.o.src0.value.pc);
        if(is_far_away(estimator_pc, target_pc)) {
            in.replace_with(mov_imm_(
                in.instr->u.o.dsts[0],
                int64_(reinterpret_cast<uint64_t>(target_pc))));
        }
    }


    /// Propagate a delay region across mangling. If we have mangled a single
    /// instruction that begins/ends a delay region into a sequence of
    /// instructions then we need to change which instruction logically begins
    /// the interrupt delay region's begin/end bounds.
    void instruction_list_mangler::propagate_delay_region(
        instruction IF_KERNEL(in),
        instruction IF_KERNEL(first),
        instruction IF_KERNEL(last)
    ) throw() {
#if GRANARY_IN_KERNEL
        if(in.begins_delay_region() && first.is_valid()) {
            in.remove_flag(instruction::DELAY_BEGIN);
            first.add_flag(instruction::DELAY_BEGIN);
        }

        if(in.ends_delay_region() && last.is_valid()) {
            in.remove_flag(instruction::DELAY_END);
            last.add_flag(instruction::DELAY_END);
        }
#endif
    }


    /// Find a far memory operand and its size. If we've already found one in
    /// this instruction then don't repeat the check.
    static void find_far_operand(
        const operand_ref op,
        const const_app_pc &estimator_pc,
        operand &far_op,
        bool &has_far_op
    ) throw() {
        if(has_far_op || dynamorio::REL_ADDR_kind != op->kind) {
            return;
        }

        if(!is_far_away(estimator_pc, op->value.addr)) {
            return;
        }

        // if the operand is too far away then we will need to indirectly load
        // the operand through its absolute address.
        has_far_op = true;
        far_op = *op;
    }


    /// Update a far operand in place.
    static void update_far_operand(operand_ref op, operand &new_op) throw() {
        if(dynamorio::REL_ADDR_kind != op->kind
        && dynamorio::PC_kind != op->kind) {
            return;
        }

        op.replace_with(new_op); // update the op in place
    }


    /// Mangle a `push addr`, where `addr` is unreachable. A nice convenience
    /// in user space is that we don't need to worry about the redzone because
    /// `push` is operating on the stack.
    void instruction_list_mangler::mangle_far_memory_push(
        instruction in,
        bool first_reg_is_dead,
        dynamorio::reg_id_t dead_reg_id,
        dynamorio::reg_id_t spill_reg_id,
        uint64_t addr
    ) throw() {
        instruction first_in;
        instruction last_in;

        if(first_reg_is_dead) {
            const operand reg_addr(dead_reg_id);
            first_in = ls.insert_before(in, mov_imm_(reg_addr, int64_(addr)));
            in.replace_with(push_(*reg_addr));

        } else {
            const operand reg_addr(spill_reg_id);
            const operand reg_value(spill_reg_id);
            first_in = ls.insert_before(in, lea_(reg::rsp, reg::rsp[-8]));
            ls.insert_before(in, push_(reg_addr));
            ls.insert_before(in, mov_imm_(reg_addr, int64_(addr)));
            ls.insert_before(in, mov_ld_(reg_value, *reg_addr));

            in.replace_with(mov_st_(reg::rsp[8], reg_value));

            last_in = ls.insert_after(in, pop_(reg_addr));
        }

        propagate_delay_region(in, first_in, last_in);
    }


    /// Mangle a `pop addr`, where `addr` is unreachable. A nice convenience
    /// in user space is that we don't need to worry about the redzone because
    /// `pop` is operating on the stack.
    void instruction_list_mangler::mangle_far_memory_pop(
        instruction in,
        bool first_reg_is_dead,
        dynamorio::reg_id_t dead_reg_id,
        dynamorio::reg_id_t spill_reg_id,
        uint64_t addr
    ) throw() {
        instruction first_in;
        instruction last_in;

        if(first_reg_is_dead) {
            const operand reg_value(dead_reg_id);
            const operand reg_addr(spill_reg_id);

            first_in = ls.insert_before(in, pop_(reg_value));
            ls.insert_before(in, push_(reg_addr));
            ls.insert_before(in, mov_imm_(reg_addr, int64_(addr)));

            in.replace_with(mov_st_(*reg_addr, reg_value));

            last_in = ls.insert_after(in, pop_(reg_addr));

        } else {
            const operand reg_value(dead_reg_id);
            const operand reg_addr(spill_reg_id);

            first_in = ls.insert_before(in, push_(reg_value));
            ls.insert_before(in, push_(reg_addr));
            ls.insert_before(in, mov_imm_(reg_addr, int64_(addr)));
            ls.insert_before(in, mov_ld_(reg_value, reg::rsp[16]));

            in.replace_with(mov_st_(*reg_addr, reg_value));

            ls.insert_after(in, pop_(reg_addr));
            ls.insert_after(in, pop_(reg_value));
            last_in = ls.insert_after(in, lea_(reg::rsp, reg::rsp[8]));
        }

        propagate_delay_region(in, first_in, last_in);
    }


    /// Mangle %rip-relative memory operands into absolute memory operands
    /// (indirectly through a spill register) in user space. This checks to see
    /// if %rip-relative operands and > 4gb away and converts them to a
    /// different form. This is a "two step" process, in that a DR instruction
    /// might have multiple memory operands (e.g. inc, add), and some of them
    /// must all be equivalent.
    ///
    /// We assume it's always legal to convert %rip-relative into a base/disp
    /// type operand (of the same size).
    void instruction_list_mangler::mangle_far_memory_refs(
        instruction in
    ) throw() {
        IF_TEST( const bool was_atomic(in.is_atomic()); )

        bool has_far_op(false);
        operand far_op;

        in.for_each_operand(
            find_far_operand, estimator_pc, far_op, has_far_op);

        if(!has_far_op) {
            return;
        }

        const uint64_t addr(reinterpret_cast<uint64_t>(far_op.value.pc));

        register_manager rm;
        rm.revive_all();

        // peephole optimisation; ideally will allow us to avoid spilling a
        // register by finding a dead register.
        instruction next_in(in.next());
        if(next_in.is_valid()) {
            rm.visit(next_in);
        }

        rm.visit(in);
        dynamorio::reg_id_t dead_reg_id(rm.get_zombie());

        rm.kill_all();
        rm.revive(in);
        rm.kill(dead_reg_id);
        dynamorio::reg_id_t spill_reg_id(rm.get_zombie());

        // overload the dead register to be a second spill register; needed for
        // `pop addr`.
        bool first_reg_is_dead(!!dead_reg_id);
        if(!first_reg_is_dead) {
            dead_reg_id = rm.get_zombie();
        }

        // push and pop need to be handled specially because they operate on
        // the stack, so the usual save/restore is not legal.
        switch(in.op_code()) {
        case dynamorio::OP_push:
            return mangle_far_memory_push(
                in, first_reg_is_dead, dead_reg_id, spill_reg_id, addr);
        case dynamorio::OP_pop:
            return mangle_far_memory_pop(
                in, first_reg_is_dead, dead_reg_id, spill_reg_id, addr);
        default: break;
        }

        operand used_reg;
        instruction first_in;
        instruction last_in;

        // use a dead register
        if(first_reg_is_dead) {
            used_reg = dead_reg_id;
            first_in = ls.insert_before(in, mov_imm_(used_reg, int64_(addr)));

        // spill a register, then use that register to load the value from
        // memory. Note: the ordering of managing `first_in` is intentional and
        // done for delay propagation.
        } else {

            used_reg = spill_reg_id;
            first_in = ls.insert_before(in, push_(used_reg));
            IF_USER( first_in = ls.insert_before(first_in,
                lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )
            ls.insert_before(in, mov_imm_(used_reg, int64_(addr)));
            last_in = ls.insert_after(in, pop_(used_reg));
            IF_USER( last_in = ls.insert_after(last_in,
                lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )
        }

        operand_base_disp new_op_(*used_reg);
        new_op_.size = far_op.size;

        operand new_op(new_op_);
        in.for_each_operand(update_far_operand, new_op);

        ASSERT(was_atomic == in.is_atomic());

        // propagate interrupt delaying.
        propagate_delay_region(in, first_in, last_in);
    }


    /// Mangle a bit scan to check for a 0 input. If the input is zero, the ZF
    /// flag is set (as usual), but the destination operand is always given a
    /// value of `~0`.
    ///
    /// The motivation for this mangling was because of how the undefined
    /// behaviour of the instruction (in input zero) seemed to have interacted
    /// with the watchpoints instrumentation. The kernel appears to expect the
    /// value to be -1 when the input is 0, so we emulate that.
    void instruction_list_mangler::mangle_bit_scan(instruction in) throw() {
        const operand op(in.instr->u.o.src0);
        const operand dest_op(in.instr->u.o.dsts[0]);
        operand undefined_value;

        register_scale undef_scale(REG_64);
        switch(dynamorio::opnd_size_in_bytes(dest_op.size)) {
        case 1: undefined_value = int8_(-1);    undef_scale = REG_8;  break;
        case 2: undefined_value = int16_(-1);   undef_scale = REG_16; break;
        case 4: undefined_value = int32_(-1);   undef_scale = REG_32; break;
        case 8: undefined_value = int64_(-1);   undef_scale = REG_64; break;
        default: ASSERT(false); break;
        }

        register_manager rm;
        rm.kill_all();
        rm.revive(in);

        // We spill regardless so that we can store the "undefined" value.
        const dynamorio::reg_id_t undefined_source_reg_64(rm.get_zombie());
        const operand undefined_source_64(undefined_source_reg_64);
        const operand undefined_source(register_manager::scale(
            undefined_source_reg_64, undef_scale));

        in = ls.insert_after(in, push_(undefined_source_64));
        in = ls.insert_after(in, mov_imm_(undefined_source, undefined_value));
        in = ls.insert_after(in,
            cmovcc_(dynamorio::OP_cmovz, dest_op, undefined_source));
        ls.insert_after(in, pop_(undefined_source_64));
    }


    /// Convert non-instrumented instructions that change control-flow into
    /// mangled instructions.
    void instruction_list_mangler::mangle(void) throw() {

        instruction in(ls.first());
        instruction next_in;

        // go mangle instructions; note: indirect CTI mangling happens here.
        for(; in.is_valid(); in = next_in) {
            const bool is_mangled(in.is_mangled());
            next_in = in.next();

            // native instruction, we might need to mangle it.
            if(in.is_cti()) {
                if(!is_mangled) {
                    mangle_cti(in);
                }

            // Look for cases where an `LEA` loads from a memory address that is
            // too far away and fix it.
            } else if(dynamorio::OP_lea == in.op_code()) {

                IF_PERF( const unsigned old_num_ins(ls.length()); )
                mangle_lea(in);
                IF_PERF( perf::visit_mem_ref(ls.length() - old_num_ins); )

            // Look for uses of relative addresses in operands that are no
            // longer reachable with %rip-relative encoding, and convert to a
            // use of an absolute address.
            } else {
                IF_PERF( const unsigned old_num_ins(ls.length()); )
                mangle_far_memory_refs(in);
                IF_PERF( perf::visit_mem_ref(ls.length() - old_num_ins); )
            }
        }
    }


    /// Make sure that we emit a basic block that meets all alignment
    /// requirements necessary for hot-patching direct control transfer
    /// instructions.
    unsigned instruction_list_mangler::align(
        instruction_list &ls,
        unsigned curr_align
    ) throw() {
        instruction in;
        instruction next_in;
        unsigned size(0);

        for(in = ls.first(); in.is_valid(); in = next_in) {
            next_in = in.next();
            unsigned in_size(in.encoded_size());
            const bool is_hot_patchable(in.is_patchable());
            const unsigned cache_line_offset(
                curr_align % CONFIG_MIN_CACHE_LINE_SIZE);

            // Make sure that hot-patchable instructions don't cross cache
            // line boundaries.
            if(is_hot_patchable
            && CONFIG_MIN_CACHE_LINE_SIZE < (cache_line_offset + in_size)) {
                ASSERT(in.prev().is_valid());
                const unsigned forward_align(
                    CONFIG_MIN_CACHE_LINE_SIZE - cache_line_offset);
                ASSERT(8 > forward_align);
                insert_nops_after(ls, in.prev(), forward_align);
                in_size += forward_align;
                IF_PERF( perf::visit_align_nop(forward_align); )
            }

            curr_align += in_size;
            size += in_size;
        }

        return size;
    }


    /// Constructor
    instruction_list_mangler::instruction_list_mangler(
        cpu_state_handle cpu_,
        basic_block_state &bb_,
        instruction_list &ls_,
        instruction_list &stub_ls_,
        instrumentation_policy policy_,
        const_app_pc estimator_pc_
    ) throw()
        : cpu(cpu_)
        , bb(bb_)
        , policy(policy_)
        , ls(ls_)
        , stub_ls(stub_ls_)
        , estimator_pc(estimator_pc_)
    { }
}
