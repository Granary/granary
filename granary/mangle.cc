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
#include "granary/predict.h"

/// Used to unroll registers in the opposite order in which they are saved
/// by PUSHA in x86/asm_helpers.asm. This is so that we can operate on the
/// pushed state on the stack as a form of machine context.
#define DPM_DECLARE_REG(reg) \
    uint64_t reg;


#define DPM_DECLARE_REG_CONT(reg, rest) \
    rest \
    DPM_DECLARE_REG(reg)


/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define DEFINE_DIRECT_JUMP_RESOLVER(opcode, size) \
    direct_branch_ ## opcode = \
        make_direct_cti_patch_func<opcode ## _ >( \
            granary_asm_direct_branch_template);

#define DEFINE_XMM_SAFE_DIRECT_JUMP_RESOLVER(opcode, size) \
    direct_branch_ ## opcode ## _xmm = \
        make_direct_cti_patch_func<opcode ## _ >( \
            granary_asm_xmm_safe_direct_branch_template);


/// Used to forward-declare the assembly function patches. These patch functions
/// eventually call the templates.
#define CASE_DIRECT_JUMP_MANGLER(opcode, size) \
    case dynamorio::OP_ ## opcode: \
        if(!(direct_branch_ ## opcode)) { \
            DEFINE_DIRECT_JUMP_RESOLVER(opcode, size) \
        } \
        return direct_branch_ ## opcode;

#define CASE_XMM_SAFE_DIRECT_JUMP_MANGLER(opcode, size) \
    case dynamorio::OP_ ## opcode: \
        if(!(direct_branch_ ## opcode ## _xmm)) { \
            DEFINE_XMM_SAFE_DIRECT_JUMP_RESOLVER(opcode, size) \
        } \
        return direct_branch_ ## opcode ## _xmm;

/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define DECLARE_DIRECT_JUMP_MANGLER(opcode, size) \
    static app_pc direct_branch_ ## opcode = nullptr;

#define DECLARE_XMM_SAFE_DIRECT_JUMP_MANGLER(opcode, size) \
    static app_pc direct_branch_ ## opcode ## _xmm = nullptr;


extern "C" {
    extern void granary_asm_xmm_safe_direct_branch_template(void);
    extern void granary_asm_direct_branch_template(void);
}


namespace granary {

    namespace {

        /// Specific instruction manglers used for branch lookup.
#if CONFIG_TRACK_XMM_REGS || GRANARY_IN_KERNEL
        DECLARE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(DECLARE_DIRECT_JUMP_MANGLER)
#endif
#if !GRANARY_IN_KERNEL
        DECLARE_XMM_SAFE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(DECLARE_XMM_SAFE_DIRECT_JUMP_MANGLER)
#endif
    }


    enum {
        MAX_NUM_POLICIES = 1 << mangled_address::NUM_MANGLED_BITS
    };


    /// A machine context representation for direct jump/call patches. The
    /// structure contains as much state as is saved by the direct assembly
    /// direct jump patch functions.
    ///
    /// The high-level operation here is that the patch function will know
    /// what to patch because of the return address on the stack (we guarantee
    /// that hot-patchable instructions will be aligned on 8-byte boundaries)
    /// and that we know where we need to patch the target to by looking at the
    /// relative target offset from the beginning of application code.
    struct direct_cti_patch_mcontext {

        // low on the stack

        /// Saved registers.
        ALL_REGS(DPM_DECLARE_REG_CONT, DPM_DECLARE_REG)

        /// Saved flags.
        eflags flags;

        /// The target address of a jump, including the policy to use when
        /// translating the target basic block.
        mangled_address target_address;

        /// Return address into the patch code located in the tail of the
        /// basic block being patched. We use this to figure out the instruction
        /// to patch because the tail code ends with a jmp to that instruction.
        app_pc return_address_into_patch_tail;

    } __attribute__((packed));


    /// Nice names for registers used by the IBL and RBL.
    static operand reg_target_addr; // arg1, rdi
    static operand reg_compare_addr; // rcx
    static operand reg_compare_addr_32; // ecx


#if CONFIG_ENABLE_IBL_PREDICTION_STUBS
    static operand reg_predict_table_ptr; // arg2, rsi
    static operand reg_predict_ptr; // ret, rax
#endif

    STATIC_INITIALISE_ID(ibl_registers, {
        reg_target_addr = reg::arg1;
        reg_compare_addr = reg::rcx;
        reg_compare_addr_32 = reg::ecx;
        IF_IBL_PREDICT( reg_predict_table_ptr = reg::arg2; )
        IF_IBL_PREDICT( reg_predict_ptr = reg::rax; )
    })


    /// Make an IBL stub. This is used by indirect JMPs, CALLs, and RETs.
    /// The purpose of the stub is to set up the registers and stack in a
    /// canonical way for entry into the indirect branch lookup table.
    instruction instruction_list_mangler::ibl_entry_stub(
        instruction_list &ibl,
        instruction in,
        instrumentation_policy target_policy,
        operand target,
        ibl_entry_kind ibl_kind
    ) throw() {

        instruction in_ret(in);

        int stack_offset(0);
        IF_PERF( const unsigned num_instruction(ibl.length()); )

        if(IBL_ENTRY_RETURN == ibl_kind) {

            // Kernel space: save `reg_target_addr` and load the return address.
            if(!REDZONE_SIZE) {
                FAULT; // should not be calling this from kernel space

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
        //       consistency in the ibl_exit_routines, we use this. This reduces
        //       the number of additional pseudo policies to 1.

        // Shift the stack. If this is a return in user space then we shift if
        // by 8 bytes less than the redzone size, so that the return address
        // itself "extends" the redzone by those 8 bytes.
        if(stack_offset) {
            in = ibl.insert_after(in, lea_(reg::rsp, reg::rsp[-stack_offset]));
        }

        in = ibl.insert_after(in, push_(reg_target_addr));
        IF_IBL_PREDICT( in = ibl.insert_after(in, push_(reg_predict_table_ptr)); )
        stack_offset += IF_IBL_PREDICT_ELSE(16, 8);

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
        instruction_list tail_bb;

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
            } else if(dynamorio::REL_ADDR_kind == target.kind
            || dynamorio::opnd_is_abs_addr(target)) {

                app_pc target_addr(target.value.pc);

                // Do an indirect load using abs address.
                if(is_far_away(target_addr, estimator_pc)) {
                    tail_bb.append(mov_imm_(
                        reg_target_addr,
                        int64_(reinterpret_cast<uint64_t>(target_addr))));
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
        if(IBL_ENTRY_CALL == ibl_kind || IBL_ENTRY_JMP == ibl_kind) {
            instrumentation_policy tail_policy(policy);

            // Kill this registers so that the instrumentation can use it.
            IF_IBL_PREDICT( tail_bb.append(
                mangled(mov_imm_(reg_predict_table_ptr, int64_(0)))); )

            // Kill all flags so that the instrumentation can use them if
            // possible.
            if(IBL_ENTRY_CALL == ibl_kind) {
                tail_bb.append(mangled(popf_()));
            }

            // Make sure all other registers appear live.
            tail_bb.append(mangled(jmp_(instr_(tail_bb_end))));

            tail_policy.instrument(cpu, *bb, tail_bb);
            mangle(tail_bb);
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
            in = ibl.insert_after(in, tail_in);
        }

        switch(ibl_kind) {
        case IBL_ENTRY_CALL:
        case IBL_ENTRY_JMP: {

#if CONFIG_ENABLE_IBL_PREDICTION_STUBS

            // Allocate a table pointer for this basic block. This value of
            // this table pointer can be changed at runtime by the code_cache.
            prediction_table **predict_table(cpu->small_allocator. \
                allocate<prediction_table *>());

            *predict_table = prediction_table::get_default(
                cpu, ibl_entry_routine(target_policy));

            if(is_far_away(predict_table, estimator_pc)) {
                in = ibl.insert_after(in, mov_imm_(reg_predict_table_ptr,
                    int64_(reinterpret_cast<uint64_t>(predict_table))));
            } else {
                in = ibl.insert_after(in, lea_(
                    reg_predict_table_ptr,
                    absmem_(predict_table, dynamorio::OPSZ_8)));
            }

            ibl.insert_after(in,
                mangled(jmp_(pc_(ibl_predict_entry_routine()))));

#else
            in = ibl.insert_after(in, push_(reg_compare_addr));
            ibl.insert_after(in,
                mangled(jmp_(pc_(ibl_entry_routine(target_policy)))));
#endif

            IF_PERF( perf::visit_ibl_stub(ibl.length() - num_instruction); )

            break;
        }
        case IBL_ENTRY_RETURN:
            break;
        }

        return in_ret;
    }


#if !CONFIG_ENABLE_DIRECT_RETURN


    /// Forward declaration.
    static void ibl_exit_stub(
        instruction_list &ibl,
        app_pc target_pc
    ) throw();


    /// Checks to see if a return address is in the code cache. If so, it
    /// RETs to the address, otherwise it JMPs to the IBL entry routine.
    app_pc instruction_list_mangler::rbl_entry_routine(
        instrumentation_policy target_policy
    ) throw() {
        static volatile app_pc routine[MAX_NUM_POLICIES] = {nullptr};
        const unsigned target_policy_bits(target_policy.encode());
        if(routine[target_policy_bits]) {
            return routine[target_policy_bits];
        }

        instruction_list ibl;

#   if !GRANARY_IN_KERNEL
        ibl_entry_stub(
            ibl, ibl.last(), target_policy, operand(*reg::rsp), IBL_ENTRY_RETURN);
        ibl.append(push_(reg_compare_addr));
#   else
        // This overlays the return and target address, and does some tricks to
        // save `reg_target_addr` in place of the return address, without doing
        // and `XCHG` on memory (which would be a locked instruction).
        IF_IBL_PREDICT( ibl.append(push_(reg_predict_table_ptr)); )
        ibl.append(push_(reg_compare_addr));
        ibl.append(mov_ld_(reg_compare_addr, reg_target_addr));
        ibl.append(mov_ld_(reg_target_addr, reg::rsp[IF_IBL_PREDICT_ELSE(16, 8)]));
        ibl.append(mov_st_(reg::rsp[IF_IBL_PREDICT_ELSE(16, 8)], reg_compare_addr));
#   endif

        // on the stack:
        //      return address
        //      redzone - 8             (cond. user space)
        //      reg_target_addr         (saved: arg1)
        //      reg_predict_table_ptr   (cond. saved: arg2)
        //      reg_compare_addr        (saved: rcx)

        // The call instruction will be 8 byte aligned, and will occupy ~5bytes.
        // the subsequent jmp needed to link the basic block (which ends with
        // the call) will also by 8 byte aligned, and will be padded to 8 bytes
        // (so that the call's basic block and the subsequent block can be
        // linked with the direct branch lookup/patch mechanism. Thus, we can
        // move the return address forward, then align it back to 8 bytes and
        // expect to find the magic value which begins the basic block meta
        // info.
        ibl.append(lea_(
            reg_compare_addr, reg_target_addr[16 - RETURN_ADDRESS_OFFSET]));

        // Load and zero-extend the (potential) 32-bit header.
        operand header_mem(*reg_compare_addr);
        header_mem.size = dynamorio::OPSZ_4;
        ibl.append(mov_ld_(reg_compare_addr_32, seg::cs(header_mem)));

        // Compare against the header
        ibl.append(push_(reg::rax));
        ibl.append(mov_imm_(reg::rax, int64_(-basic_block_info::HEADER)));
        ibl.append(lea_(reg_compare_addr, reg_compare_addr + reg::rax));
        ibl.append(pop_(reg::rax));

        instruction slow(ibl.last());
        instruction fast(ibl.append(label_()));
        slow = ibl.insert_after(slow, jecxz_(instr_(fast)));

        // slow path: return address is not in code cache; go off to the IBL.
        IF_IBL_PREDICT( slow = ibl.insert_after(slow,
            mov_imm_(reg_predict_table_ptr, int64_(0))); )
        ibl.insert_after(slow, jmp_(pc_(ibl_entry_routine(target_policy))));

        // fast path: they are equal; in the kernel this requires restoring the
        // return address, in user space this isn't necessary because the we
        // overlap the redzone and return address in the case of the slow path.
#   if GRANARY_IN_KERNEL
        ibl.append(mov_ld_(reg_compare_addr, reg::rsp[IF_IBL_PREDICT_ELSE(16, 8)]));
        ibl.append(mov_st_(reg::rsp[IF_IBL_PREDICT_ELSE(16, 8)], reg_target_addr));
        ibl.append(mov_st_(reg_target_addr, reg_compare_addr));
        ibl.append(pop_(reg_compare_addr));
        IF_IBL_PREDICT( ibl.append(pop_(reg_predict_table_ptr)); )
        ibl.append(ret_());

#   else
        ibl.append(pop_(reg_compare_addr));
        IF_IBL_PREDICT( ibl.append(pop_(reg_predict_table_ptr)); )
        ibl.append(pop_(reg_target_addr));
        IF_USER( ibl.append(lea_(reg::rsp, reg::rsp[REDZONE_SIZE - 8])); )
        ibl.append(ret_());
#   endif

        // quick double check ;-)
        if(routine[target_policy_bits]) {
            return routine[target_policy_bits];
        }

        IF_PERF( perf::visit_rbl(ibl); )

        // encode
        app_pc temp(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>( ibl.encoded_size()));
        ibl.encode(temp);
        routine[target_policy_bits] = temp;

        return temp;
    }
#endif /* !CONFIG_ENABLE_DIRECT_RETURN */


    /// Address of the CPU-private code cache lookup function.
    static app_pc cpu_private_code_cache_find(nullptr);


    /// Address of the global code cache lookup function.
    static app_pc global_code_cache_find(nullptr);


    STATIC_INITIALISE_ID(code_cache_functions, {
        cpu_private_code_cache_find = unsafe_cast<app_pc>(
            (app_pc (*)(
                mangled_address,
                prediction_table **
            )) code_cache::find_on_cpu
        );

        global_code_cache_find = unsafe_cast<app_pc>(
            (app_pc (*)(mangled_address)) code_cache::find);
    })


    /// Return the IBL entry routine. The IBL entry routine is responsible
    /// for looking to see if an address (stored in reg::arg1) is located
    /// in the CPU-private code cache or in the global code cache. If the
    /// address is in the CPU-private code cache, and if IBL prediction is
    /// enabled, then the CPU-private lookup function might add a prediction
    /// entry to the CTI.
    app_pc instruction_list_mangler::ibl_entry_routine(
        instrumentation_policy target_policy
    ) throw() {
        static volatile app_pc routine[MAX_NUM_POLICIES] = {nullptr};
        const unsigned target_policy_bits(target_policy.encode());
        if(routine[target_policy_bits]) {
            return routine[target_policy_bits];
        }

        // on the stack:
        //      redzone
        //      reg_target_addr         (saved: arg1)
        //      reg_predict_table_ptr   (cond. saved: arg2)
        //      reg_compare_addr        (saved: rcx)

        instruction_list ibl;

        ibl.append(push_(reg::rax));
        insert_save_flags_after(ibl, ibl.last(), REG_AH_IS_DEAD);

        // mangle the target address with the policy. This corresponds to the
        // `mangled_address` argument to the `code_cache::find*` functions.
        ibl.append(shl_(reg_target_addr, int8_(mangled_address::NUM_MANGLED_BITS)));
        ibl.append(add_(reg_target_addr, int32_(target_policy_bits)));

        // save all registers for the IBL.
        register_manager all_regs;
        all_regs.kill_all();
        all_regs.revive(reg_target_addr);
        all_regs.revive(reg_compare_addr);
        all_regs.revive(reg::rax);
        IF_IBL_PREDICT( all_regs.revive(reg_predict_table_ptr); )

        // create a "safe" region of code around which all registers are saved
        // and restored.
        instruction safe(
            save_and_restore_registers(all_regs, ibl, ibl.last()));

#if !GRANARY_IN_KERNEL
        if(target_policy.is_in_xmm_context()) {
            safe = save_and_restore_xmm_registers(
                all_regs, ibl, safe, XMM_SAVE_UNALIGNED);

        // only save %xmm0 and %xmm1, because the ABI allows these registers
        // to be used for return values.
        //
        // TODO: should this be done in the kernel?
        } else {
            all_regs.revive_all_xmm();
            all_regs.kill(dynamorio::DR_REG_XMM0);
            all_regs.kill(dynamorio::DR_REG_XMM1);
            safe = save_and_restore_xmm_registers(
                all_regs, ibl, safe, XMM_SAVE_UNALIGNED);
        }
#endif

        // save the target on the stack so that if the register is clobbered by
        // the CPU-private lookup function then we can recall it for the global
        // lookup.
        safe = ibl.insert_after(safe, push_(reg_target_addr));

        // align the stack to a 16 byte boundary before the call.
        safe = insert_align_stack_after(ibl, safe);

        // fast path: try to find the address in the CPU-private code cache.
        safe = insert_cti_after(
            ibl, safe, cpu_private_code_cache_find,
            CTI_STEAL_REGISTER, reg::rax,
            CTI_CALL);
        safe = insert_restore_old_stack_alignment_after(ibl, safe);
        safe = ibl.insert_after(safe, pop_(reg_target_addr));
        safe = ibl.insert_after(safe, test_(reg::ret, reg::ret));

        instruction safe_fast(ibl.insert_after(safe, label_()));

        safe = ibl.insert_after(safe, jnz_(instr_(safe_fast)));

        // slow path: do a global code cache lookup
        safe = insert_align_stack_after(ibl, safe);
        safe = insert_cti_after(
            ibl, safe, global_code_cache_find,
            CTI_STEAL_REGISTER, reg::rax,
            CTI_CALL);
        safe = insert_restore_old_stack_alignment_after(ibl, safe);

        // fast path, and fall-through of slow path: move the returned target
        // address into `reg_target_addr` (`reg::arg1`)
        ibl.insert_after(safe_fast, mov_ld_(reg_target_addr, reg::ret));

        insert_restore_flags_after(ibl, ibl.last(), REG_AH_IS_DEAD);
        ibl.append(pop_(reg::rax));

        // jump to the target. The target in this case is an IBL exit routine,
        // which is responsible for cleaning up the stack.
        ibl.append(jmp_ind_(reg_target_addr));

        // encode
        app_pc temp(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>( ibl.encoded_size()));
        ibl.encode(temp);

        IF_PERF( perf::visit_ibl(ibl); )

        routine[target_policy_bits] = temp;
        return temp;
    }


    static void ibl_exit_stub(
        instruction_list &ibl,
        app_pc target_pc
    ) throw() {
        // on the stack:
        //      redzone
        //      reg_target_addr         (saved: arg1)
        //      reg_predict_table_ptr   (cond. saved: arg2)
        //      reg_compare_addr        (saved: rcx)

        ibl.append(pop_(reg_compare_addr));
        IF_IBL_PREDICT( ibl.append(pop_(reg_predict_table_ptr)); )
        ibl.append(pop_(reg_target_addr));
        IF_USER( ibl.append(lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )

        if(target_pc) {
            insert_cti_after(
                ibl, ibl.last(), target_pc,
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_JMP);
        }
    }


    /// Return or generate the IBL exit routine for a particular jump target.
    /// The target can either be code cache or native code.
    app_pc instruction_list_mangler::ibl_exit_routine(
        app_pc target_pc
    ) {
        instruction_list ibl;
        ibl_exit_stub(ibl, target_pc);
        app_pc routine(global_state::FRAGMENT_ALLOCATOR->allocate_array<uint8_t>(
            ibl.encoded_size()));

        IF_PERF( perf::visit_ibl_exit(ibl); )

        ibl.encode(routine);
        return routine;
    }


#if CONFIG_ENABLE_IBL_PREDICTION_STUBS
    /// Return the IBL entry routine that first looks in a prediction table
    /// for its target address. On failing this, it falls over to the last
    /// entry in the table, whose destination (`dest`) field is the address
    /// of the corresponding IBL entry routine, or a specialized entry
    /// routine.
    app_pc instruction_list_mangler::ibl_predict_entry_routine(void) throw() {

        static app_pc routine(nullptr);
        if(routine) {
            return routine;
        }

        instruction_list ibl;

        ibl.append(push_(reg_compare_addr));

        // on the stack:
        //      redzone
        //      reg_target_addr         (saved: arg1)
        //      reg_predict_table_ptr   (saved: arg2)
        //      reg_compare_addr        (saved: rcx)

        ibl.append(push_(reg_predict_ptr));
        ibl.append(mov_ld_(reg_predict_ptr, *reg_predict_table_ptr));

        // increment the access count field
#if 1
        operand access_count_field(
            reg_predict_ptr[prediction_table::NUM_READS_OFFSET]);
        access_count_field.size = dynamorio::OPSZ_4;
        ibl.append(mov_ld_(reg_compare_addr_32, access_count_field));
        ibl.append(lea_(reg_compare_addr, reg_compare_addr + 1));
        ibl.append(mov_st_(access_count_field, reg_compare_addr_32));
#endif

        instruction test(ibl.append(label_()));
        instruction miss(ibl.append(label_()));
        instruction hit_or_fail(ibl.append(label_()));

        // on the first iteration, the LEA will skip the prediction table
        // header and go right for the first slot; subsequent iterations
        // will skip over individual slots.
        //
        // Note: predict entries are stored as (-source, dest), so
        //       lea cmp, 0(cmp, target, 1) is equivalent to
        //       sub target, cmp.
        ibl.insert_before(miss, lea_(reg_predict_ptr, reg_predict_ptr[16]));
        ibl.insert_before(miss, mov_ld_(reg_compare_addr, *reg_predict_ptr));
        ibl.insert_before(miss, jrcxz_(instr_(miss)));
        ibl.insert_before(miss, lea_(reg_compare_addr, reg_compare_addr + reg_target_addr));
        ibl.insert_before(miss, jrcxz_(instr_(miss)));
        ibl.insert_before(miss, jmp_(instr_(test)));

        // Note: in both the hit and fail cases, we want to maintain the values
        //       of `reg_target_addr` and `reg_predict_table_ptr` so that we
        //       can do race-prone modifications of the lookup table that still
        //       maintain semantic correctness.

        // fall through: found a match. This will jmp to the IBL exit routine,
        // leaving the stack as the IBL predict routine received it.
        ibl.insert_before(hit_or_fail, mov_ld_(reg_compare_addr, reg_predict_ptr[8]));
        ibl.insert_before(hit_or_fail, pop_(reg_predict_ptr));
        ibl.insert_before(hit_or_fail, jmp_ind_(reg_compare_addr));

        // encode
        app_pc temp(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>( ibl.encoded_size()));
        ibl.encode(temp);
        routine = temp;
        return temp;
    }
#endif


    /// Injects N bytes of NOPs into an instuction list after a specific
    /// instruction.
    ///
    /// Note: this will propagate delay regions on NOPs to ensure that an
    ///       hot-patchable regions are never interrupted.
    void instruction_list_mangler::inject_mangled_nops(
        instruction_list &ls,
        instruction in,
        unsigned num_nops
    ) throw() {
        instruction original(in);
        instruction invalid;

        for(; num_nops >= 3; num_nops -= 3) {
            IF_PERF( perf::visit_align_nop(); )
            in = ls.insert_after(in, nop3byte_());
            propagate_delay_region(original, invalid, in);
        }

        if(2 == num_nops) {
            IF_PERF( perf::visit_align_nop(); )
            in = ls.insert_after(in, nop2byte_());
            propagate_delay_region(original, invalid, in);

        } else if(num_nops) {
            IF_PERF( perf::visit_align_nop(); )
            ls.insert_after(in, nop1byte_());
            propagate_delay_region(original, invalid, in);
        }
    }


    /// Stage an 8-byte hot patch. This will encode the instruction `in` into
    /// the `stage` location (as if it were going to be placed at the `dest`
    /// location, and then encodes however many NOPs are needed to fill in 8
    /// bytes. If offset is non-zero, then `offset` number of NOP bytes will be
    /// encoded before the instruction `in`.
    void instruction_list_mangler::stage_8byte_hot_patch(
        instruction in,
        app_pc stage,
        app_pc dest,
        unsigned offset
    ) throw() {
        instruction_list ls;

        if(offset) {
            inject_mangled_nops(ls, ls.first(), offset);
        }

        ls.append(in);

        const unsigned size(in.encoded_size());
        if((size + offset) < 8) {
            inject_mangled_nops(ls, ls.first(), 8U - (size + offset));
        }

        ls.stage_encode(stage, dest);
    }


    /// Patch the code by regenerating the original instruction.
    ///
    /// Note: in kernel mode, this function executes with interrupts
    ///       disabled.
    ///
    /// Note: this function alters a return address in `context` so that
    ///       when the corresponding assembly patch function returns, it
    ///       will return to the instruction just patched.
    GRANARY_ENTRYPOINT
    template <instruction (*make_opcode)(dynamorio::opnd_t)>
    static void
    find_and_patch_direct_cti(direct_cti_patch_mcontext *context) throw() {

        IF_KERNEL( kernel_preempt_disable(); )

        // Notify Granary that we're entering!
        cpu_state_handle cpu;
        granary::enter(cpu);

        // Get an address into the target basic block using two stage lookup.
        app_pc target_pc(
            cpu->code_cache.find(context->target_address.as_address));

        if(!target_pc) {
            target_pc = code_cache::find(cpu, context->target_address);
        }

        ASSERT(is_code_cache_address(context->return_address_into_patch_tail));

        // Determine the address to patch; this decodes the *tail* of the patch
        // code in the basic block and looks for a CTI (assumed jmp) and takes
        // its target to be the instruction that must be patched.
        app_pc patch_address(nullptr);
        for(app_pc ret_pc(context->return_address_into_patch_tail); ;) {
            instruction maybe_jmp(instruction::decode(&ret_pc));
            if(maybe_jmp.is_cti()) {
                ASSERT(dynamorio::OP_jmp == maybe_jmp.op_code());
                patch_address = maybe_jmp.cti_target().value.pc;
                ASSERT(0 == (reinterpret_cast<uint64_t>(patch_address) % 8));
                break;
            }
        }

        // create the patch code
        uint64_t staged_code_(0ULL);
        app_pc staged_code(reinterpret_cast<app_pc>(&staged_code_));

        // make the cti instruction, and try to widen it if necessary.
        instruction cti(make_opcode(pc_(target_pc)));
        cti.widen_if_cti();

        unsigned offset(0);
        if(cti.is_call()) {
            ASSERT(cti.encoded_size() <= RETURN_ADDRESS_OFFSET);
            offset = RETURN_ADDRESS_OFFSET - cti.encoded_size();
        }

        instruction_list_mangler::stage_8byte_hot_patch(
            cti, staged_code, patch_address, offset);

        // apply the patch
        granary_atomic_write8(
            staged_code_,
            reinterpret_cast<uint64_t *>(patch_address));

        IF_KERNEL( kernel_preempt_enable(); )
    }


    /// Make a direct patch function that is specific to a particular opcode.
    template <instruction (*make_opcode)(dynamorio::opnd_t)>
    static app_pc make_direct_cti_patch_func(
        void (*template_func)(void)
    ) throw() {

        instruction_list ls;
        app_pc start_pc(unsafe_cast<app_pc>(template_func));

        for(;;) {
            instruction in(instruction::decode(&start_pc));
            if(in.is_call()) {
                ls.append(mov_imm_(reg::rax, int64_(reinterpret_cast<int64_t>(
                    find_and_patch_direct_cti<make_opcode>))));
            }

            ls.append(in);
            if(dynamorio::OP_ret == in.op_code()) {
                break;
            }
        }

        app_pc dest_pc(global_state::FRAGMENT_ALLOCATOR->allocate_array<uint8_t>(
            ls.encoded_size()));

        IF_PERF( perf::visit_dbl_patch(ls); )

        ls.encode(dest_pc);
        return dest_pc;
    }


    /// Look up and return the assembly patch (see asm/direct_branch.asm)
    /// function needed to patch an instruction that originally had opcode as
    /// `opcode`.
#if CONFIG_TRACK_XMM_REGS || GRANARY_IN_KERNEL
    static app_pc get_direct_cti_patch_func(int opcode) throw() {
        switch(opcode) {
        CASE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(CASE_DIRECT_JUMP_MANGLER);
        default: return nullptr;
        }
    }
#endif


#if !GRANARY_IN_KERNEL
    /// Look up and return the assembly patch (see asm/direct_branch.asm)
    /// function needed to patch an instruction that originally had opcode as
    /// `opcode`. These patch functions will save/restore all %xmm registers.
    static app_pc get_xmm_safe_direct_cti_patch_func(int opcode) throw() {
        switch(opcode) {
        CASE_XMM_SAFE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(CASE_XMM_SAFE_DIRECT_JUMP_MANGLER);
        default: return nullptr;
        }
    }
#endif


    /// Get or build the direct branch lookup (DBL) routine for some jump/call
    /// target.
    app_pc instruction_list_mangler::dbl_entry_routine(
        instrumentation_policy target_policy,
        instruction in,
        mangled_address am
    ) throw() {

        /// Nice names for register(s) used by the DBL.
        operand reg_mangled_addr(reg::rax);

        // add in the patch code, change the initial behaviour of the
        // instruction, and mark it has hot patchable so it is nicely aligned.
#if GRANARY_IN_KERNEL
        app_pc patcher_for_opcode(get_direct_cti_patch_func(in.op_code()));
#elif CONFIG_TRACK_XMM_REGS
        app_pc patcher_for_opcode(nullptr);
        if(target_policy.is_in_xmm_context()) {
            patcher_for_opcode = get_xmm_safe_direct_cti_patch_func(in.op_code());
        } else {
            patcher_for_opcode = get_direct_cti_patch_func(in.op_code());
        }
#else
        app_pc patcher_for_opcode(get_xmm_safe_direct_cti_patch_func(
            in.op_code()));
#endif

        (void) target_policy;

        instruction_list dbl;

        // TODO: these patch stubs can be reference counted so that they
        //       can be reclaimed (especially since every patch stub will
        //       have the same size!).

        // TODO: these patch stubs represent a big memory leak!

        // Store the policy-mangled target on the stack.
        dbl.append(lea_(reg::rsp, reg::rsp[-8]));
        dbl.append(push_(reg_mangled_addr));
        dbl.append(mov_imm_(reg_mangled_addr, int64_(am.as_int)));
        dbl.append(mov_st_(reg::rsp[8], reg_mangled_addr));
        dbl.append(pop_(reg_mangled_addr)); // restore

        // tail-call out to the patcher/mangler.
        dbl.append(mangled(jmp_(pc_(patcher_for_opcode))));

        app_pc routine(cpu->fragment_allocator.allocate_array<uint8_t>(
            dbl.encoded_size()));

        dbl.encode(routine);

        IF_PERF( perf::visit_dbl(dbl); )

        return routine;
    }


    /// Make a direct CTI patch stub. This is used both for mangling direct CTIs
    /// and for emulating policy inheritance/scope when transparent return
    /// addresses are used.
    void instruction_list_mangler::dbl_entry_stub(
        instruction_list &patch_ls,
        instruction patch,
        instruction patched_in,
        app_pc dbl_routine
    ) throw() {
        IF_PERF( const unsigned old_num_ins(patch_ls.length()); )

        int redzone_size(patched_in.is_call() ? 0 : REDZONE_SIZE);

        // We add REDZONE_SIZE + 8 because we make space for the policy-mangled
        // address. The
        if(redzone_size) {
            patch = patch_ls.insert_after(patch,
                lea_(reg::rsp, reg::rsp[-redzone_size]));
        }

        patch = patch_ls.insert_after(patch,
            mangled(call_(pc_(dbl_routine))));

        if(redzone_size) {
            patch = patch_ls.insert_after(patch,
                lea_(reg::rsp, reg::rsp[redzone_size]));
        }

        // the address to be mangled is implicitly encoded in the target of this
        // jmp instruction, which will later be decoded by the direct cti
        // patch function. There are two reasons for this approach of jumping
        // around:
        //      i)  Doesn't screw around with the return address predictor.
        //      ii) Works with user space red zones.
        patch_ls.insert_after(patch,
            mangled(jmp_(instr_(mangled(patched_in)))));

        IF_PERF( perf::visit_dbl_stub(patch_ls.length() - old_num_ins); )
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

        // If we already know the target, then forgo a stub.
        detach_target_pc = cpu->code_cache.find(am.as_address);

#if GRANARY_IN_KERNEL
        if(!detach_target_pc
        && (is_code_cache_address(target_pc) || is_wrapper_address(target_pc))) {
            detach_target_pc = target_pc;
        }
#endif

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
        if(!detach_target_pc){
            detach_target_pc = find_detach_target(
                target_pc, target_policy.context());
        }

        // If this is a detach point then replace the target address with the
        // detach address. This can be tricky because the instruction might not
        // be a call/jmp (i.e. it might be a conditional branch)
        if(detach_target_pc) {

            if(is_far_away(estimator_pc, detach_target_pc)) {

                // TODO: convert to an alternative form in the case of a
                //       conditional branch.
                ASSERT(in.is_call() || in.is_jump());

                app_pc *slot = cpu->fragment_allocator.allocate<app_pc>();
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

#if !CONFIG_ENABLE_DIRECT_RETURN
            if(!in.next().is_valid() || in.is_call()) {
                in.set_patchable();
            }
#endif

            return;
        }

        const unsigned old_size(in.encoded_size());

        // Set the policy-fied target.
        instruction stub(ls->prepend(label_()));
        dbl_entry_stub(
            *ls,                                     // list to patch
            stub,                                    // patch label
            in,                                      // patched instruction
            dbl_entry_routine(target_policy, in, am) // target of stub
        );

        in.replace_with(patchable(mangled(jmp_(instr_(stub)))));

        const unsigned new_size(in.encoded_size());

        IF_DEBUG(old_size > 8, FAULT)
        IF_DEBUG(new_size > 8, FAULT)

        (void) new_size;
    }


    /// Mangle an indirect control transfer instruction.
    void instruction_list_mangler::mangle_indirect_cti(
        instruction in,
        operand target,
        instrumentation_policy target_policy
    ) throw() {

        // mark indirect calls as hot-patchable (even though they won't be
        // patched) so that they are automatically aligned to an 8-byte boundary
        // with associated alignment nops after them. This is so that the RBL
        // can treat direct and indirect calls uniformly.
        //
        // TODO: in future, it might be worth hot-patching the call if we
        //       can make good predictions.
        if(in.is_call()) {

            IF_PERF( perf::visit_mangle_indirect_call(); )

            instruction start_of_stub(ls->prepend(label_()));
            ibl_entry_stub(
                *ls,
                start_of_stub,
                target_policy,
                target,
                IBL_ENTRY_CALL
            );
            in.replace_with(patchable(mangled(call_(instr_(start_of_stub)))));

        } else if(in.is_return()) {

            IF_PERF( perf::visit_mangle_return(); )

#if !CONFIG_ENABLE_DIRECT_RETURN
            // TODO: handle RETn/RETf with a byte count.
            in.replace_with(
                mangled(jmp_(pc_(rbl_entry_routine(target_policy)))));
#endif
        } else {

            IF_PERF( perf::visit_mangle_indirect_jmp(); )

            in.replace_with(mangled(jmp_(instr_(ibl_entry_stub(
                *ls,
                ls->prepend(label_()),
                target_policy,
                target,
                IBL_ENTRY_JMP
            )))));
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

        // Inherit properties that aren't defined by this CTI, e.g. XMM register
        // usage.
        target_policy.inherit_properties(policy);

        if(dynamorio::OP_iret == in.op_code()) {
            // TODO?
            return;

        } else if(in.is_return()) {
            target_policy.return_target(true);
            target_policy.indirect_cti_target(false);
            target_policy.in_host_context(false);

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


    void instruction_list_mangler::mangle_cli(instruction in) throw() {
        (void) in;
    }


    void instruction_list_mangler::mangle_sti(instruction in) throw() {

        (void) in;
    }


#if CONFIG_TRANSLATE_FAR_ADDRESSES
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
#endif


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


#if CONFIG_TRANSLATE_FAR_ADDRESSES


    /// Find a far memory operand and its size. If we've already found one in
    /// this instruction then don't repeat the check.
    static void find_far_operand(
        const operand_ref op,
        app_pc &estimator_pc,
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
            first_in = ls->insert_before(in, mov_imm_(reg_addr, int64_(addr)));
            in.replace_with(push_(*reg_addr));

        } else {
            const operand reg_addr(spill_reg_id);
            const operand reg_value(spill_reg_id);
            first_in = ls->insert_before(in, lea_(reg::rsp, reg::rsp[-8]));
            ls->insert_before(in, push_(reg_addr));
            ls->insert_before(in, mov_imm_(reg_addr, int64_(addr)));
            ls->insert_before(in, mov_ld_(reg_value, *reg_addr));

            in.replace_with(mov_st_(reg::rsp[8], reg_value));

            last_in = ls->insert_after(in, pop_(reg_addr));
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

            first_in = ls->insert_before(in, pop_(reg_value));
            ls->insert_before(in, push_(reg_addr));
            ls->insert_before(in, mov_imm_(reg_addr, int64_(addr)));

            in.replace_with(mov_st_(*reg_addr, reg_value));

            last_in = ls->insert_after(in, pop_(reg_addr));

        } else {
            const operand reg_value(dead_reg_id);
            const operand reg_addr(spill_reg_id);

            first_in = ls->insert_before(in, push_(reg_value));
            ls->insert_before(in, push_(reg_addr));
            ls->insert_before(in, mov_imm_(reg_addr, int64_(addr)));
            ls->insert_before(in, mov_ld_(reg_value, reg::rsp[16]));

            in.replace_with(mov_st_(*reg_addr, reg_value));

            ls->insert_after(in, pop_(reg_addr));
            ls->insert_after(in, pop_(reg_value));
            last_in = ls->insert_after(in, lea_(reg::rsp, reg::rsp[8]));
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
            first_in = ls->insert_before(in, mov_imm_(used_reg, int64_(addr)));

        // spill a register, then use that register to load the value from
        // memory. Note: the ordering of managing `first_in` is intentional and
        // done for delay propagation.
        } else {

            used_reg = spill_reg_id;
            first_in = ls->insert_before(in, push_(used_reg));
            IF_USER( first_in = ls->insert_before(first_in,
                lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )
            ls->insert_before(in, mov_imm_(used_reg, int64_(addr)));
            last_in = ls->insert_after(in, pop_(used_reg));
            IF_USER( last_in = ls->insert_after(last_in,
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
#endif


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

        in = ls->insert_after(in, push_(undefined_source_64));
        in = ls->insert_after(in, mov_imm_(undefined_source, undefined_value));
        in = ls->insert_after(in,
            cmovcc_(dynamorio::OP_cmovz, dest_op, undefined_source));
        ls->insert_after(in, pop_(undefined_source_64));
    }


    /// Convert non-instrumented instructions that change control-flow into
    /// mangled instructions.
    void instruction_list_mangler::mangle(instruction_list &ls_) throw() {
        instruction_list *prev_ls(ls);

        ls = &ls_;

        instruction in(ls->first());
        instruction next_in;

        // go mangle instructions; note: indirect CTI mangling happens here.
        for(; in.is_valid(); in = next_in) {
            const bool is_mangled(in.is_mangled());
            const bool can_skip(nullptr == in.pc() || is_mangled);
            next_in = in.next();

            // native instruction, we might need to mangle it.
            if(in.is_cti()) {
                if(!is_mangled) {
                    mangle_cti(in);
                }

            // clear interrupt
            } else if(dynamorio::OP_cli == in.op_code()) {
                if(can_skip) {
                    continue;
                }
                mangle_cli(in);

            // restore interrupt
            } else if(dynamorio::OP_sti == in.op_code()) {
                if(can_skip) {
                    continue;
                }
                mangle_sti(in);

#if CONFIG_TRANSLATE_FAR_ADDRESSES
            // Look for cases where an `LEA` loads from a memory address that is
            // too far away and fix it.
            } else if(dynamorio::OP_lea == in.op_code()) {

                IF_PERF( const unsigned old_num_ins(ls->length()); )
                mangle_lea(in);
                IF_PERF( perf::visit_mem_ref(ls->length() - old_num_ins); )

            // Look for uses of relative addresses in operands that are no
            // longer reachable with %rip-relative encoding, and convert to a
            // use of an absolute address.
            } else {

#if 0
                if(dynamorio::OP_bsr == in.op_code()
                || dynamorio::OP_bsf == in.op_code()) {
                    mangle_bit_scan(in);
                }
#endif
                IF_PERF( const unsigned old_num_ins(ls->length()); )
                mangle_far_memory_refs(in);
                IF_PERF( perf::visit_mem_ref(ls->length() - old_num_ins); )
#endif
            }
        }


        // do a second-pass over all instructions, looking for any hot-patchable
        // instructions, and aligning them nicely.
        //
        // Extra alignment/etc needs to be done here instead of in encoding
        // because of how basic block allocation works.
        unsigned align(0);
        instruction prev_in;
        in = ls->first();

        for(; in.is_valid(); in = next_in) {

            next_in = in.next();
            const bool is_hot_patchable(in.is_patchable());
            const unsigned in_size(in.encoded_size());

            // x86-64 guaranteed quadword atomic writes so long as the memory
            // location is aligned on an 8-byte boundary; we will assume that
            // we are never patching an instruction longer than 8 bytes
            if(is_hot_patchable) {
                uint64_t forward_align(ALIGN_TO(align, 8));

                // This will make sure that even indirect calls have their
                // return addresses aligned at `RETURN_ADDRESS_OFFSET`.
                if(in.is_call() && RETURN_ADDRESS_OFFSET > in_size) {
                    forward_align += RETURN_ADDRESS_OFFSET - in_size;
                }

                inject_mangled_nops(*ls, prev_in, forward_align);
                align += forward_align;
            }

            prev_in = in;
            align += in_size;

            // make sure that the instruction is the only "useful" one in it's
            // 8-byte block
            if(is_hot_patchable) {
                uint64_t forward_align(ALIGN_TO(align, 8));
                inject_mangled_nops(ls_, prev_in, forward_align);
                align += forward_align;
            }
        }

        ls = prev_ls;
    }


    /// Constructor
    instruction_list_mangler::instruction_list_mangler(
        cpu_state_handle cpu_,
        basic_block_state *bb_,
        instrumentation_policy &policy_
    ) throw()
        : cpu(cpu_)
        , bb(bb_)
        , policy(policy_)
        , ls(nullptr)
        , estimator_pc(cpu->fragment_allocator.allocate_staged<uint8_t>())
    { }

}

