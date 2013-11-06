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
#include "granary/ibl.h"

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

/// Used to forward-declare the assembly function patches. These patch functions
/// eventually call the templates.
#define CASE_DIRECT_JUMP_MANGLER(opcode, size) \
    case dynamorio::OP_ ## opcode: \
        if(!(direct_branch_ ## opcode)) { \
            DEFINE_DIRECT_JUMP_RESOLVER(opcode, size) \
        } \
        return direct_branch_ ## opcode;

/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define DECLARE_DIRECT_JUMP_MANGLER(opcode, size) \
    static app_pc direct_branch_ ## opcode = nullptr;


extern "C" {
    extern uint16_t granary_bswap16(uint16_t);
    extern void granary_asm_direct_branch_template(void);
}


namespace granary {

    namespace {
        /// Specific instruction manglers used for branch lookup.
        DECLARE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(DECLARE_DIRECT_JUMP_MANGLER)
    }


    enum {
        MAX_NUM_POLICIES = 1 << mangled_address::NUM_MANGLED_BITS,
        HOTPATCH_ALIGN = 8
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


    struct ibl_stub_key {
        uint16_t policy_bits;
        operand target_operand;

        bool operator==(const ibl_stub_key &that) const throw() {
            return policy_bits == that.policy_bits
                && 0 == memcmp(&target_operand, &(that.target_operand), sizeof target_operand);
        }
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
        ibl_entry_kind ibl_kind
        _IF_PROFILE_IBL( app_pc cti_addr )
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
        if(IBL_ENTRY_CALL == ibl_kind || IBL_ENTRY_JMP == ibl_kind) {
            instrumentation_policy tail_policy(policy);

            // Kill all flags so that the instrumentation can use them if
            // possible.
            if(IBL_ENTRY_CALL == ibl_kind) {
                tail_bb.append(mangled(popf_()));
            }

            // Make sure all other registers appear live.
            tail_bb.append(mangled(jmp_(instr_(tail_bb_end))));

            tail_policy.instrument(cpu, bb, tail_bb);

            instruction_list_mangler sub_mangler(
                cpu, bb, tail_bb, stub_ls, policy);
            sub_mangler.mangle();
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
            ibl.insert_before(in, tail_in);
        }

        // Extend the basic block with the IBL lookup stub.
        ibl_lookup_stub(ibl, in, target_policy _IF_PROFILE_IBL(cti_addr));
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

        // Notify Granary that we're entering!
        cpu_state_handle cpu;
        granary::enter(cpu);

        ASSERT(is_valid_address(context->target_address.unmangled_address()));
        ASSERT(is_valid_address(context->return_address_into_patch_tail));
        ASSERT(is_gencode_address(context->return_address_into_patch_tail));

        // Get an address into the target basic block using two stage lookup.
        app_pc ret_pc(context->return_address_into_patch_tail);
        app_pc target_pc(code_cache::find(cpu, context->target_address));

        // Determine the address to patch; this decodes the *tail* of the patch
        // code in the basic block and looks for a CTI (assumed jmp) and takes
        // its target to be the instruction that must be patched.
        app_pc patch_address(nullptr);
        for(int max_tries(0); max_tries < 8; ++max_tries) {
            instruction maybe_jmp(instruction::decode(&ret_pc));
            if(maybe_jmp.is_cti()) {
                ASSERT(dynamorio::OP_jmp == maybe_jmp.op_code());
                patch_address = maybe_jmp.cti_target().value.pc;
                goto ready_to_patch;
            }
        }
        USED(context);

        ASSERT(false);

    ready_to_patch:

#if CONFIG_ENABLE_TRACE_ALLOCATOR
        // Inherit this basic block's allocator from our predecessor basic
        // block.
        basic_block source_bb(patch_address);
        cpu->current_fragment_allocator = source_bb.info->allocator;
#endif

        // Make sure that the patch target will fit on one cache line. We will
        // use `5` as the magic number for instruction size, as that's typical
        // for a RIP-relative CTI.

        // Get the original code. Note: We get the code before we decode the
        // instruction, which means that the decoded instruction *could* see
        // a newer version of the code. We accept this as it will allow us to
        // detect whether or not the code was patched (because the decoded old
        // CTI will no longer point into gencode).
        uint64_t *code_to_patch(reinterpret_cast<uint64_t *>(patch_address));
        uint64_t original_code(*code_to_patch);
        uint64_t staged_code(original_code);
        app_pc staged_data(reinterpret_cast<app_pc>(&staged_code));

        // The old instruction that we think is there.
        app_pc decode_address(patch_address);
        instruction old_cti(instruction::decode(&decode_address));
        unsigned old_cti_len(old_cti.encoded_size());

        // Make the new CTI instruction, widen it if possible.
        instruction new_cti(make_opcode(pc_(target_pc)));
        new_cti.widen_if_cti();

        // This is kind of a hack; see later in this file. `Jcc rel32` takes
        // 6 bytes, whereas `JMP rel32` and `CALL rel32` take 5 bytes.
        if(!new_cti.is_unconditional_cti()) {
            old_cti_len += 1;
        }

        ASSERT(old_cti_len <= sizeof(uintptr_t));
        ASSERT(new_cti.encoded_size() <= old_cti_len);

        // Double check that the instruction to patch transfers control into
        // gencode. If not then it's already been patched.
        ASSERT(old_cti.is_cti());
        operand old_cti_target(old_cti.cti_target());
        ASSERT(dynamorio::opnd_is_pc(old_cti_target));
        if(!is_gencode_address(old_cti_target.value.pc)) {
            return;
        }

        // Create a bitmask that will allow us to diagnose why a compare&swap
        // might fail. Under normal circumstances (e.g. null tool), we wouldn't
        // actually need to check whether the compare&swap succeeds because
        // control cannot jump over a patch. However, it's entirely possible for
        // another tool to place two hot-patchable CTIs side-by-side, and
        // conditionally jump to one or another.
        uint64_t code_mask(0ULL);
        memset(&code_mask, 0xFF, old_cti_len);
        const uint64_t masked_head(original_code & code_mask);

        IF_TEST( int i(0); )
        for(; IF_TEST(i < 2); IF_TEST(++i)) {

            // Fill the staged code with NOPs where the old CTI was.
            memset(staged_data, 0x90, old_cti_len);
            new_cti.stage_encode(staged_data, patch_address);

            // Apply the patch.
            const bool applied_patch(__sync_bool_compare_and_swap(
                code_to_patch, original_code, staged_code));

            if(applied_patch) {
                break;
            }

            // Get the new version of the code.
            original_code = *code_to_patch;
            staged_code = original_code;

            // Ignorable failure; the instruction was concurrently patched by
            // another thread/core.
            if(masked_head != (original_code & code_mask)) {
                break;
            }
        }

        ASSERT(2 > i);
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
                // Leave any direct calls as is. For example, the private stack
                // entry/exit functions.
                if (in.is_direct_call()) {
                    insert_cti_after(
                        ls, ls.last(), in.cti_target().value.pc,
                        CTI_STEAL_REGISTER, reg::ret,
                        CTI_CALL);

                // The indirect call targeting %rax is meant to be replaced with
                // the call to our patch function.
                } else {
                    ASSERT(dynamorio::REG_kind == in.cti_target().kind);
                    ASSERT(dynamorio::DR_REG_RAX == in.cti_target().value.reg);
                    insert_cti_after(
                        ls, ls.last(),
                        unsafe_cast<app_pc>(
                            find_and_patch_direct_cti<make_opcode>),
                        CTI_STEAL_REGISTER, reg::rax,
                        CTI_CALL);

                }
            } else {
                ls.append(in);
            }

            //ls.append(in);
            if(dynamorio::OP_ret == in.op_code()) {
                break;
            }
        }

        const unsigned size(ls.encoded_size());
        app_pc dest_pc(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(size));
        ls.encode(dest_pc, size);

        IF_PERF( perf::visit_dbl_patch(ls); )

        return dest_pc;
    }


    /// Look up and return the assembly patch (see asm/direct_branch.asm)
    /// function needed to patch an instruction that originally had opcode as
    /// `opcode`.
    static app_pc get_direct_cti_patch_func(int opcode) throw() {
        switch(opcode) {
        CASE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(CASE_DIRECT_JUMP_MANGLER);
        default: return nullptr;
        }
    }


    /// Get or build the direct branch lookup (DBL) routine for some jump/call
    /// target.
    app_pc instruction_list_mangler::dbl_entry_routine(
        instruction in,
        mangled_address am
    ) throw() {

        /// Nice names for register(s) used by the DBL.
        operand reg_mangled_addr(reg::rax);

        // Add in the patch code, change the initial behaviour of the
        // instruction, and mark it has hot patchable so it is nicely aligned.
        app_pc patcher_for_opcode(get_direct_cti_patch_func(in.op_code()));
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

        const unsigned size(dbl.encoded_size());
        app_pc routine(global_state::FRAGMENT_ALLOCATOR->allocate_array<uint8_t>(
            size));
        dbl.encode(routine, size);

        IF_PERF( perf::visit_dbl(dbl); )

        return routine;
    }


    /// Make a direct CTI patch stub. This is used both for mangling direct CTIs
    /// and for emulating policy inheritance/scope when transparent return
    /// addresses are used.
    instruction instruction_list_mangler::dbl_entry_stub(
        instruction patched_in,
        app_pc dbl_routine
    ) throw() {
        instruction ret(stub_ls.append(label_()));

        IF_PERF( const unsigned old_num_ins(stub_ls.length()); )

        int redzone_size(patched_in.is_call() ? 0 : REDZONE_SIZE);

        // We add REDZONE_SIZE + 8 because we make space for the policy-mangled
        // address. The
        if(redzone_size) {
            stub_ls.append(lea_(reg::rsp, reg::rsp[-redzone_size]));
        }

        stub_ls.append(mangled(call_(pc_(dbl_routine))));

        if(redzone_size) {
            stub_ls.append(lea_(reg::rsp, reg::rsp[redzone_size]));
        }

        // The address to be mangled is implicitly encoded in the target of
        // this JMP instruction, which will later be decoded by the direct cti
        // patch function. There are two reasons for this approach of jumping
        // around:
        //      i)  Doesn't screw around with the return address predictor.
        //      ii) Works with user space red zones.
        stub_ls.append(mangled(jmp_(instr_(mangled(patched_in)))));

        IF_PERF( perf::visit_dbl_stub(stub_ls.length() - old_num_ins); )

        return ret;
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

            return;
        }

        IF_TEST( const unsigned old_size(in.encoded_size()); )

        // Set the policy-fied target.
        instruction stub(dbl_entry_stub(
            in,                         // patched instruction
            dbl_entry_routine(in, am)   // target of stub
        ));

        const bool was_unconditional(in.is_unconditional_cti());
        in.replace_with(patchable(mangled(jmp_(instr_(stub)))));

        if(!was_unconditional) {
            in.instr->granary_flags |= instruction::COND_CTI_PLACEHOLDER;
        }

        IF_TEST( const unsigned new_size(in.encoded_size()); )
        ASSERT(old_size <= 8);
        ASSERT(new_size <= 8);
    }


    /// Mangle an indirect control transfer instruction.
    void instruction_list_mangler::mangle_indirect_cti(
        instruction in,
        operand target,
        instrumentation_policy target_policy
    ) throw() {
        if(in.is_call()) {

            instruction call_target(stub_ls.append(label_()));
            instruction insert_point(stub_ls.append(label_()));

            IF_PERF( perf::visit_mangle_indirect_call(); )
            mangle_ibl_lookup(
                stub_ls, insert_point, target_policy, target,
                IBL_ENTRY_CALL _IF_PROFILE_IBL(in.pc()));

            ls.insert_before(in, mangled(call_(instr_(call_target))));
            ls.remove(in);

        } else if(in.is_return()) {
            IF_PERF( perf::visit_mangle_return(); )
#if !CONFIG_ENABLE_DIRECT_RETURN
            if(!policy.return_address_is_in_code_cache()) {

                // TODO: handle RETn/RETf with a byte count.
                ASSERT(dynamorio::IMMED_INTEGER_kind != in.instr->u.o.src0.kind);

                mangle_ibl_lookup(
                    ls, in, target_policy, target,
                    IBL_ENTRY_RETURN _IF_PROFILE_IBL(in.pc()));
                ls.remove(in);
            }
#endif
        } else {
            IF_PERF( perf::visit_mangle_indirect_jmp(); )
            mangle_ibl_lookup(
                ls, in, target_policy, target,
                IBL_ENTRY_JMP _IF_PROFILE_IBL(in.pc()));
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
            } else {
                target_policy.inherit_properties(policy, INHERIT_JMP);
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
    unsigned instruction_list_mangler::align(unsigned curr_align) throw() {
        instruction in;
        instruction next_in;
        unsigned size(0);

        for(in = ls.first(); in.is_valid(); in = next_in) {

            next_in = in.next();
            const bool is_hot_patchable(in.is_patchable());
            unsigned in_size(in.encoded_size());

            // This is a hack: `Jcc rel32` takes 6 bytes to encode, whereas
            // `CALL rel32` and `JMP rel32` take only 5 bytes.
            //
            // We add the NOP here so that we can take it into account when
            // figuring out the forward alignment requirements of the patchable
            // instruction.
            if(is_hot_patchable
            && (in.instr->granary_flags & instruction::COND_CTI_PLACEHOLDER)) {
                in.instr->granary_flags &= ~instruction::COND_CTI_PLACEHOLDER;
                ls.insert_after(in, nop1byte_());
                in_size += 1;
            }

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
        instrumentation_policy policy_
    ) throw()
        : cpu(cpu_)
        , bb(bb_)
        , policy(policy_)
        , ls(ls_)
        , stub_ls(stub_ls_)
        , estimator_pc(cpu->fragment_allocator.allocate_staged<uint8_t>())
    { }

}

