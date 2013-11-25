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

#include "granary/basic_block.h"
#include "granary/basic_block_info.h"

namespace granary {


    /// Data structure that tracks direct control flow instructions that must
    /// be patched, and how to patch them.
    ///
    /// TODO: Find a way do to garbage collection on these. Alternatively, keep
    ///       them around as a way of re-wiring the code cache, although this
    ///       is redundant because there is already enough information contained
    ///       in `basic_block_info` to effectively re-wire things.
    struct direct_branch_patch_info {

        /// Always the same; the function that actually performs the patch.
        app_pc patcher_func;

        /// This is pretty evil: We use a non CPU-private allocated instruction,
        /// and put it into the instruction stream directly. This gives us
        /// access to the instruction's location after it has been patched.
        dynamorio::instr_t in_to_patch;

        /// The target of the instruction to patch.
        mangled_address target_address;

        /// The resolved target of the patch.
        app_pc translated_target_address;

        /// Lock on if this is owned.
        spin_lock lock;

        enum {
            DBL_CONDITIONAL,
            DBL_FALL_THROUGH,
            DBL_UNCONDITIONAL
        } kind;
    };


    enum {
        CALL_INDIRECT_ADDRESS_SIZE = 6 // 1-byte opcode + mod/rm + rel32
    };


    /// Patch a direct control-flow instruction.
    GRANARY_ENTRYPOINT
    static void patch_instruction(app_pc *ret_address_addr) throw() {

        // Notify Granary that we're entering!
        cpu_state_handle cpu;
        granary::enter(cpu);

        app_pc indirect_call(*ret_address_addr - CALL_INDIRECT_ADDRESS_SIZE);

        // Make sure we're coming from the right place.
        ASSERT(is_gencode_address(indirect_call));

        // The indirect call that brought us here goes through the actual
        // `direct_branch_patch_info` structure.
        instruction call_ind(instruction::decode(&indirect_call));
        direct_branch_patch_info *patch(unsafe_cast<direct_branch_patch_info *>(
            call_ind.cti_target().value.addr));

        // Start by specifying the return address as the instruction that
        // brought us into here, i.e. infinite loop!
        *ret_address_addr = call_ind.pc_or_raw_bytes();

        // If we can't get mutual exclusion over the locking process then we'll
        // give up and go right on back. This might re-enter again, which is
        // fine.
        if(!patch->lock.try_acquire()) {
            if(patch->translated_target_address) {
                *ret_address_addr = patch->translated_target_address;
            }
            return;
        }

        // We got ownership of the lock, but we've just realized that the
        // instruction has already been patched!
        //
        // TODO: Garbage collection of patch info structures.
        if(patch->translated_target_address) {
            patch->lock.release();
            *ret_address_addr = patch->translated_target_address;
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

        // Get the original CTI that we're going to patch.
        app_pc patch_address(patch->in_to_patch.translation);
        ASSERT(is_code_cache_address(patch_address));

#if CONFIG_ENABLE_TRACE_ALLOCATOR
        // Propagate the allocator through direct control flow instructions.
        const basic_block_info * const source_bb_info(
            find_basic_block_info(patch_address));
        cpu->current_fragment_allocator = source_bb_info->allocator;
#endif

        app_pc target_pc(code_cache::find(cpu, patch->target_address));

        // Tell concurrent patchers that the patch is done, even before it is!
        // This is fine because they will redirect to the destination, not back
        // to the instruction being patched.
        std::atomic_thread_fence(std::memory_order_acquire);
        patch->translated_target_address = target_pc;
        std::atomic_thread_fence(std::memory_order_release);

        // Make sure we return to the destination of the instruction we're
        // patching, rather than re-executing the original instruction.
        *ret_address_addr = patch->translated_target_address;

        uint64_t staged_code(0);
        app_pc staged_data(reinterpret_cast<app_pc>(&staged_code));

        app_pc decode_address(patch_address);
        instruction new_cti(instruction::decode(&decode_address));

        IF_TEST( const unsigned old_cti_len(new_cti.encoded_size()); )

        new_cti.set_cti_target(pc_(target_pc));
        new_cti.stage_encode(staged_data, patch_address);
        const unsigned new_cti_len(new_cti.encoded_size());

        ASSERT(old_cti_len == new_cti_len);

        const unsigned rel32_offset(new_cti_len - sizeof(uint32_t));
        const uint32_t new_rel32(
            *unsafe_cast<uint32_t *>(&(staged_data[rel32_offset])));
        uint32_t *old_rel32(
            unsafe_cast<uint32_t *>(&(patch_address[rel32_offset])));

        std::atomic_thread_fence(std::memory_order_acquire);
        *old_rel32 = new_rel32;
        std::atomic_thread_fence(std::memory_order_release);

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
        IF_NOT_TEST(
            rm.revive(reg::rbx);
            rm.revive(reg::rbp);
            rm.revive(reg::r12);
            rm.revive(reg::r13);
            rm.revive(reg::r14);
            rm.revive(reg::r15);
        )

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

        // TODO: If the basic block is not committed then this is a memory leak.
        // TODO: These are already a memory leak! Find a way to reclaim them in
        //       the common case.
        direct_branch_patch_info *patch(
            allocate_memory<direct_branch_patch_info>());

        // Copy the patch instruction verbatim. At patch time, the actual
        // sources and destination operands are invalid, so MUST not be
        // accessed.
        memcpy(&(patch->in_to_patch), cti.instr, sizeof *(cti.instr));
        patch->in_to_patch.next = nullptr;
        patch->in_to_patch.prev = nullptr;

        // Unconditional CTIs are always followed, and they all occupy 5 bytes,
        // so we always replace them with 5-byte JMPs to stubs, which are
        // always followed.
        if(cti.is_unconditional_cti()) {

            // Fall-through JMP.
            if(cti.has_flag(instruction::COND_CTI_FALL_THROUGH)) {
                patch->kind = direct_branch_patch_info::DBL_FALL_THROUGH;
                IF_PERF( perf::visit_fall_through_dbl(); )

            // Direct JMP, direct CALL.
            } else {
                patch->kind = direct_branch_patch_info::DBL_UNCONDITIONAL;
            }

        // Conditional CTIs (Jcc) occupy 6 bytes, but aren't always followed,
        // so we keep them as the original Jcc to the stub so that they occupy
        // the correct amount of space and only trigger translation of the
        // destination block if its taken.
        } else {
            patch->kind = direct_branch_patch_info::DBL_CONDITIONAL;
            IF_PERF( perf::visit_conditional_dbl(); )
        }

        // Modify the instruction to patch in place.
        instruction patch_cti(&(patch->in_to_patch));
        patch_cti.set_cti_target(instr_(stub_ls.append(label_())));
        patch_cti.set_mangled();
        patch_cti.set_patchable();

        IF_USER( stub_ls.append(lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )

        instruction patch_entry(call_ind_(mem_pc_(&(patch->patcher_func))));
        stub_ls.append(patch_entry);

        ASSERT(CALL_INDIRECT_ADDRESS_SIZE == patch_entry.encoded_size());

        // Redzone is unshifted in the RET instruction of the patcher
        // in the case of user space.

        // Fill in the rest of the information.
        patch->target_address = target_address;
        patch->patcher_func = PATCH_INSTRUCTION;

        // Replace the CTI.
        ls.insert_before(cti, instruction(&(patch->in_to_patch)));
    }
}


