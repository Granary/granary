/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-10-17
 *      Author: Peter Goodman
 */

#include <atomic>

#include "clients/watchpoints/clients/everything_watched_aug/instrument.h"

using namespace granary;

namespace client {


    IF_PERF( extern std::atomic<unsigned> NUM_AUGMENT_FAULTS; )
    IF_PERF( extern std::atomic<unsigned> NUM_AUGMENT_FAULTS_MISS; )
    IF_PERF( extern std::atomic<unsigned> NUM_AUGMENT_FAULTS_DUPLICATES; )
    IF_PERF( extern std::atomic<unsigned> NUM_NULL_BBS; )
    IF_PERF( extern std::atomic<unsigned> NUM_NATIVE_FAULTS; )

    /// Instruction a basic block.
    granary::instrumentation_policy watchpoint_null_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        instruction_list &ls
    ) throw() {

        IF_PERF( NUM_NULL_BBS.fetch_add(1); )

        instruction first(ls.first());
        instruction patch_target(ls.prepend(label_()));
        ls.prepend(mangled(patchable(jmp_short_(instr_(patch_target)))));

        return policy_for<watchpoint_null_policy>();
    }


    /// Instruction a basic block.
    granary::instrumentation_policy watchpoint_null_policy::visit_host_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        instruction_list &ls
    ) throw() {

        IF_PERF( NUM_NULL_BBS.fetch_add(1); )

        instruction first(ls.first());
        instruction patch_target(ls.prepend(label_()));
        ls.prepend(mangled(patchable(jmp_short_(instr_(patch_target)))));

        return policy_for<watchpoint_null_policy>();
    }

    namespace wp {
        void watched_policy::visit_read(
            granary::basic_block_state &,
            instruction_list &,
            watchpoint_tracker &,
            unsigned
        ) throw() { }


        void watched_policy::visit_write(
            granary::basic_block_state &,
            instruction_list &,
            watchpoint_tracker &,
            unsigned
        ) throw() { }

#if CONFIG_CLIENT_HANDLE_INTERRUPT
        interrupt_handled_state watched_policy::handle_interrupt(
            cpu_state_handle,
            thread_state_handle,
            granary::basic_block_state &,
            interrupt_stack_frame &,
            interrupt_vector
        ) throw() {
            return INTERRUPT_DEFER;
        }
#endif
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT


    /// Re-encode some of the instructions in the faulting basic block, up to
    /// the first CTI, which will either be a CTI into a stub or a CTI to a
    /// wrapped address.
    ///
    /// This is used as part of the basic block upgrade process. A rcu_null
    /// basic block is upgraded to an rcu_watched basic block, and then control
    /// is returned to this "watched tail" of the faulting basic block.
    ///
    /// TODO: Handle case of indirect CTIs through watched memory address.
    app_pc instrument_watched_tail(
        cpu_state_handle cpu,
        granary::basic_block_state &bb,
        app_pc cache_pc,
        instrumentation_policy policy
    ) throw() {

        app_pc start_pc(cache_pc);

        // Decode a suffix of the basic block up to the next CTI.
        instruction_list ls;
        for(;;) {
            const app_pc in_pc(start_pc);
            instruction in(instruction::decode(&start_pc));

            if(dynamorio::instr_is_nop(in)) {
                continue;
            }

            if(in.is_cti()) {
                ls.append(mangled(jmp_(pc_(in_pc))));
                break;
            }

            ls.append(in);
        }

        // Run the watchpoints instrumentation on the tail.
        policy.instrument(cpu, bb, ls);

        // Emit as gencode.
        const unsigned size(ls.encoded_size());
        app_pc tail_pc(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(size));
        ls.encode(tail_pc, size);

        return tail_pc;
    }


    /// Handle an interrupt in module code. Returns true iff the client
    /// handles the interrupt.
    granary::interrupt_handled_state watchpoint_null_policy::handle_interrupt(
        granary::cpu_state_handle cpu,
        granary::thread_state_handle,
        granary::basic_block_state &bb,
        granary::interrupt_stack_frame &isf,
        granary::interrupt_vector vector
    ) throw() {
        if(likely(VECTOR_GENERAL_PROTECTION != vector || isf.error_code)) {
            return granary::INTERRUPT_DEFER;
        }

        IF_PERF( NUM_AUGMENT_FAULTS.fetch_add(1); )

        // Sequence of NOPs (0x90).
        uint64_t patch(0x9090909090909090ULL);

        // Re-encode the basic block using the rcu_watched policy and hot-
        // patch the first instruction to redirect execution.
        basic_block faulting_bb(isf.instruction_pointer);
        instrumentation_policy policy = policy_for<watchpoint_watched_policy>();
        policy.force_attach(true);
        policy.access_user_data(faulting_bb.policy.accesses_user_data());
        policy.in_host_context(faulting_bb.policy.is_in_host_context());
        policy.in_xmm_context(faulting_bb.policy.is_in_xmm_context());

        // Figure out the location of the hot-patchable entry-point. If the
        // trace logger is being used then we need to shift by 8 bytes.
        // Double check that we find a short/near JMP instruction.
        app_pc patch_pc(faulting_bb.cache_pc_start);
#if CONFIG_DEBUG_TRACE_EXECUTION
        patch_pc += 5; // Size of a `CALL`.
        patch_pc += ALIGN_TO(reinterpret_cast<uintptr_t>(patch_pc), 8);
#endif

        // Check that the patch point looks like a jump. This will be true
        // both before and after patching.
        ASSERT(0xEB == *patch_pc || 0xE9 == *patch_pc);

        mangled_address am(
            reinterpret_cast<app_pc>(faulting_bb.info->generating_pc),
            policy);

        // Stage a patched JMP to the upgraded basic block.
        app_pc upgraded_bb_pc(nullptr);
        if(!cpu->code_cache.load(am.as_address, upgraded_bb_pc)) {
            upgraded_bb_pc = code_cache::find(cpu, am);
        }

        instruction patch_jmp(jmp_(pc_(upgraded_bb_pc)));
        patch_jmp.stage_encode(unsafe_cast<app_pc>(&patch), patch_pc);

        // Apply the patch if we haven't already.
        uint64_t *patch_target(reinterpret_cast<uint64_t *>(patch_pc));
        std::atomic_thread_fence(std::memory_order_acquire);
        if(patch != *patch_target) {
            *patch_target = patch;
            std::atomic_thread_fence(std::memory_order_release);
        } else {
            IF_PERF( NUM_AUGMENT_FAULTS_MISS.fetch_add(1); )
        }

        // Build the tail if we didn't fault at the first instruction.
        // Redirect the interrupt to resume where watchpoints are tested
        // for.
        const app_pc first_ins(patch_pc + 8);
        if(first_ins == isf.instruction_pointer) {
            isf.instruction_pointer = upgraded_bb_pc;
        } else {
            app_pc faulting_pc(isf.instruction_pointer);
            app_pc target_addr(nullptr);

            if(!cpu->code_cache.load(faulting_pc, target_addr)) {
                target_addr = code_cache::lookup(faulting_pc);
            }

            if(!target_addr) {
                target_addr = instrument_watched_tail(
                    cpu, bb, faulting_pc, policy);

                code_cache::add(faulting_pc, target_addr);
                cpu->code_cache.store(faulting_pc, target_addr);
            } else {
                IF_PERF( NUM_AUGMENT_FAULTS_DUPLICATES.fetch_add(1); )
            }

            isf.instruction_pointer = target_addr;
        }

        return INTERRUPT_IRET;
    }


    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    interrupt_handled_state handle_kernel_interrupt(
        cpu_state_handle cpu,
        thread_state_handle,
        interrupt_stack_frame &isf,
        interrupt_vector vector
    ) throw() {
        if(likely(VECTOR_GENERAL_PROTECTION != vector || isf.error_code)) {
            return INTERRUPT_DEFER;
        }

        IF_PERF( NUM_NATIVE_FAULTS.fetch_add(1); )

        instrumentation_policy policy = policy_for<watchpoint_watched_policy>();
        policy.in_host_context(true);
        policy.force_attach(true);
        policy.access_user_data(true); // Can't know for sure, so assume it.
        // TODO: Can't know xmm for sure.

        mangled_address target(isf.instruction_pointer, policy);
        app_pc translated_target(nullptr);
        if(!cpu->code_cache.load(target.as_address, translated_target)) {
            translated_target = code_cache::find(cpu, target);
        }

        isf.instruction_pointer = translated_target;
        return INTERRUPT_IRET;
    }
#endif
}
