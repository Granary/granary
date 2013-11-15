/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * dbl.cc
 *
 *  Created on: 2013-11-13
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/code_cache.h"

#include "granary/instruction.h"
#include "granary/spin_lock.h"
#include "granary/emit_utils.h"

namespace granary {


    typedef instruction (instr_func_t)(dynamorio::opnd_t);


    /// Data structure that tracks direct control flow instructions that must
    /// be patched, and how to patch them.
    struct direct_branch_patch_info {

        /// Always the same; the function that actually performs the patch.
        app_pc patcher_func;

        /// This is pretty evil: We use a non CPU-private allocated instruction,
        /// and put it into the instruction stream directly. This gives us
        /// access to the instruction's location after it has been patched.
        dynamorio::instr_t in_to_patch;

        /// Function to create the patch instruction.
        instr_func_t *make_patched_in;

        /// The target of the instruction to patch.
        mangled_address target_address;

        /// Lock on if this is owned.
        spin_lock lock;

        /// Have we finished this patch?
        bool patch_complete;

        enum {
            DBL_CONDITIONAL,
            DBL_FALL_THROUGH,
            DBL_UNCONDITIONAL
        } kind;
    };


    static instruction bad_instr_(dynamorio::opnd_t) throw() {
        ASSERT(false);
        return instruction();
    }


#define INSTR_CASE(instr, size) \
    case dynamorio::CAT(OP_, instr): return &CAT(instr, _);


    static instr_func_t *get_instr_func_for_instr(int opcode) throw() {
        switch(opcode) {
        INSTR_CASE(call, 5)
        FOR_EACH_DIRECT_JUMP(INSTR_CASE)
        default: break;
        }

        return &bad_instr_;
    }


    /// Patch a direct control-flow instruction.
    GRANARY_ENTRYPOINT
    static void patch_instruction(app_pc *ret_address_addr) throw() {

        // Notify Granary that we're entering!
        cpu_state_handle cpu;
        granary::enter(cpu);

        enum {
            CALL_INDIRECT_ADDRESS_SIZE = 6 // 1-byte opcode + mod/rm + rel32
        };

        app_pc indirect_call(*ret_address_addr - CALL_INDIRECT_ADDRESS_SIZE);

        // Make sure we're coming from the right place.
        ASSERT(is_code_cache_address(indirect_call)
            || is_gencode_address(indirect_call));

        // The indirect call that brought us here goes through the actual
        // `direct_branch_patch_info` structure.
        instruction call_ind(instruction::decode(&indirect_call));
        direct_branch_patch_info *patch(unsafe_cast<direct_branch_patch_info *>(
            call_ind.cti_target().value.addr));

        instruction in_to_patch(&(patch->in_to_patch));
        app_pc patch_address(in_to_patch.pc_or_raw_bytes());

        ASSERT(is_code_cache_address(patch_address));

        // Redirect the return address.
        *ret_address_addr = patch_address;

        // If we can't get mutual exclusion over the locking process then we'll
        // give up and go right on back. This might re-enter again, which is
        // fine.
        if(!patch->lock.try_acquire()) {
            return;
        }

        // We got ownership of the lock, but we've just realized that the
        // instruction has already been patched! Leave it locked; we can't
        // (easily) prove that no one else is looking at this data.
        //
        // TODO: Garbage collection of patch info structures.
        if(patch->patch_complete) {
            return;
        }

        // Record performance counts that can help us evaluate tracing
        // strategies.
        IF_PERF( perf::visit_patched_dbl(); )
        switch(patch->kind) {
        case direct_branch_patch_info::DBL_CONDITIONAL:
            IF_PERF( perf::visit_patched_conditional_dbl(); )
            break;
        case direct_branch_patch_info::DBL_FALL_THROUGH:
            IF_PERF( perf::visit_patched_fall_through_dbl(); )
            break;
        default: break;
        }

        app_pc target_pc(code_cache::find(cpu, patch->target_address));

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

        // Make the new CTI instruction, widen it if possible.
        instruction new_cti(patch->make_patched_in(pc_(target_pc)));
        new_cti.widen_if_cti();

        const unsigned cti_len(new_cti.encoded_size());

        // Create a bitmask that will allow us to diagnose why a compare&swap
        // might fail. Under normal circumstances (e.g. null tool), we wouldn't
        // actually need to check whether the compare&swap succeeds because
        // control cannot jump over a patch. However, it's entirely possible for
        // another tool to place two hot-patchable CTIs side-by-side, and
        // conditionally jump to one or another.
        uint64_t code_mask(0ULL);
        memset(&code_mask, 0xFF, cti_len);
        const uint64_t masked_head(original_code & code_mask);

        IF_TEST( int i(0); )
        for(; IF_TEST(i < 2); IF_TEST(++i)) {

            // Fill the staged code with NOPs where the old CTI was.
            memset(staged_data, 0x90, cti_len);
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

        patch->patch_complete = true;
        patch->lock.release();
    }


    static app_pc PATCH_INSTRUCTION = nullptr;


    STATIC_INITIALISE_ID(patch_instruction_entrypoint, {

        instruction_list ls(INSTRUCTION_LIST_GENCODE);

        // In kernel space.
        register_manager rm;
        rm.kill_all();
        rm.revive(reg::arg1);
        rm.revive(reg::arg2);
        rm.revive(reg::ret);

        // Restore callee-saved registers, because `handle_interrupt` will
        // save them for us (because it respects the ABI).
#if !CONFIG_ENABLE_ASSERTIONS
        rm.revive(reg::rbx);
        rm.revive(reg::rbp);
        rm.revive(reg::r12);
        rm.revive(reg::r13);
        rm.revive(reg::r14);
        rm.revive(reg::r15);
#endif

        // Move the address of the return address into ARG1.
        ls.append(push_(reg::arg1));
        ls.append(lea_(reg::arg1, reg::rsp[8]));
        ls.append(pushf_());
        IF_KERNEL( ls.append(cli_()); )
        ls.append(push_(reg::arg2));
        ls.append(push_(reg::ret));

        // Switch to the private stack (we might be on the private stack if this
        // is a nested interrupt).
        IF_KERNEL( insert_cti_after(
            ls, ls.last(),
            unsafe_cast<app_pc>(granary_enter_private_stack),
            CTI_STEAL_REGISTER, reg::ret,
            CTI_CALL); )

        // Save the remaining registers after switching out stack.
        instruction enter_granary(
            save_and_restore_registers(rm, ls, ls.append(label_())));

        // Call handler.
        insert_cti_after(
            ls, enter_granary, // instruction
            unsafe_cast<app_pc>(patch_instruction), // target
            CTI_STEAL_REGISTER, reg::ret, // clobber reg
            CTI_CALL);

        // Switch back to original stack.
        IF_KERNEL( insert_cti_after(
            ls, ls.last(), unsafe_cast<app_pc>(granary_exit_private_stack),
            CTI_STEAL_REGISTER, reg::ret,
            CTI_CALL); )

        ls.append(pop_(reg::ret));
        ls.append(pop_(reg::arg2));
        ls.append(popf_());
        ls.append(pop_(reg::arg1));

        IF_KERNEL( ls.append(ret_()); )
        IF_USER( ls.append(ret_imm_(int16_(REDZONE_SIZE))); )

        // Encode.
        const unsigned size(ls.encoded_size());
        PATCH_INSTRUCTION = reinterpret_cast<app_pc>(
            global_state::FRAGMENT_ALLOCATOR-> \
                allocate_untyped(CACHE_LINE_SIZE, size));

        ls.encode(PATCH_INSTRUCTION, size);
    });


    /// Replace `cti` with a new instruction that jumps to the DBL entry routine
    /// for instruction patching and replacing.
    void insert_dbl_lookup_stub(
        instruction_list &ls,
        instruction_list &stub_ls,
        instruction cti,
        mangled_address target_address
    ) throw() {
        IF_PERF( perf::visit_dbl_stub(); )

        direct_branch_patch_info *patch(
            allocate_memory<direct_branch_patch_info>());

        patch->in_to_patch.num_srcs = 1;
        patch->in_to_patch.num_dsts = 0;

        // Unconditional CTIs are always followed, and they all occupy 5 bytes,
        // so we always replace them with 5-byte JMPs to stubs, which are
        // always followed.
        if(cti.is_unconditional_cti()) {
            patch->in_to_patch.opcode = dynamorio::OP_jmp;

            if(cti.has_flag(instruction::COND_CTI_FALL_THROUGH)) {
                patch->kind = direct_branch_patch_info::DBL_FALL_THROUGH;
                IF_PERF( perf::visit_fall_through_dbl(); )
            } else {
                patch->kind = direct_branch_patch_info::DBL_UNCONDITIONAL;
            }

        // Conditional CTIs (Jcc) occupy 6 bytes, but aren't always followed,
        // so we keep them as the original Jcc to the stub so that they occupy
        // the correct amount of space and only trigger translation of the
        // destination block if its taken.
        } else {
            patch->in_to_patch.opcode = cti.op_code();
            patch->kind = direct_branch_patch_info::DBL_CONDITIONAL;
            IF_PERF( perf::visit_conditional_dbl(); )
        }

        dynamorio::instr_set_src(
            &(patch->in_to_patch), 0, instr_(stub_ls.append(label_())));

        IF_USER( stub_ls.append(lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )
        stub_ls.append(call_ind_(mem_pc_(&(patch->patcher_func))));

        // Redzone is unshifted in the RET instruction of the patcher
        // in the case of user space.

        // Make sure the mangler treats this instruction as mangled and
        // hot-patchable.
        patch->in_to_patch.granary_flags |= instruction::DONT_MANGLE;
        patch->in_to_patch.granary_flags |= instruction::HOT_PATCHABLE;

        // Fill in the rest of the information.
        patch->make_patched_in = get_instr_func_for_instr(cti.op_code());
        patch->target_address = target_address;
        patch->patcher_func = PATCH_INSTRUCTION;

        // Replace the CTI.
        ls.insert_before(cti, instruction(&(patch->in_to_patch)));
        ls.remove(cti);
    }
}


