/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#include "clients/cfg/instrument.h"
#include "clients/cfg/events.h"

using namespace granary;


#if GRANARY_IN_KERNEL
#   include "granary/kernel/linux/module.h"
extern "C" {
    /// Returns the kernel module information for a given app address.
    extern const kernel_module *kernel_get_module(app_pc addr);
}
#endif


namespace client {


    enum {
        NUM_START_EDGES = 2
    };


    /// Used to link together all basic blocks.
    std::atomic<basic_block_state *> BASIC_BLOCKS = \
        ATOMIC_VAR_INIT(nullptr);


    /// Entry point for the on-enter-function event handler.
    static app_pc EVENT_ENTER_FUNCTION = nullptr;


    /// Exit point for the on-exit-function event handler.
    static app_pc EVENT_EXIT_FUNCTION = nullptr;


    /// Exit point for the on-exit-function event handler.
    static app_pc EVENT_AFTER_FUNCTION = nullptr;


    /// Entry point for the on-enter-bb event handler.
    static app_pc EVENT_ENTER_BASIC_BLOCK = nullptr;


    /// Unique IDs for basic blocks.
    static std::atomic<unsigned> BASIC_BLOCK_ID = ATOMIC_VAR_INIT(0);


    enum allocator_kind {
        MEMORY_ALLOCATOR,
        MEMORY_DEALLOCATOR,
        MEMORY_REALLOCATOR
    };


    /// Hashtable mapping memory allocators to de-allocators.
    static static_data<hash_table<app_pc, allocator_kind>> MEMORY_ALLOCATORS;


    /// Initialise the control-flow graph client.
    void init(void) throw() {
        cpu_state_handle cpu;
        cpu.free_transient_allocators();
        EVENT_ENTER_FUNCTION = generate_clean_callable_address(
            &event_enter_function);
        cpu.free_transient_allocators();
        EVENT_EXIT_FUNCTION = generate_clean_callable_address(
            &event_exit_function);
        cpu.free_transient_allocators();
        EVENT_AFTER_FUNCTION = generate_clean_callable_address(
            &event_after_function);
        cpu.free_transient_allocators();
        EVENT_ENTER_BASIC_BLOCK = generate_clean_callable_address(
            &event_enter_basic_block);
        cpu.free_transient_allocators();
        MEMORY_ALLOCATORS.construct();

#if GRANARY_IN_KERNEL

#   define CFG_MEMORY_ALLOCATOR(func) \
        MEMORY_ALLOCATORS->store( \
            unsafe_cast<app_pc>(CAT(DETACH_ADDR_, func)), MEMORY_ALLOCATOR);

#   define CFG_MEMORY_DEALLOCATOR(func) \
        MEMORY_ALLOCATORS->store( \
            unsafe_cast<app_pc>(CAT(DETACH_ADDR_, func)), MEMORY_DEALLOCATOR);

#   define CFG_MEMORY_REALLOCATOR(func) \
        MEMORY_ALLOCATORS->store( \
            unsafe_cast<app_pc>(CAT(DETACH_ADDR_, func)), MEMORY_REALLOCATOR);

#   include "clients/cfg/kernel/linux/allocators.h"
#   undef CFG_MEMORY_ALLOCATOR
#   undef CFG_MEMORY_DEALLOCATOR
#   undef CFG_MEMORY_REALLOCATOR

#endif /* GRANARY_IN_KERNEL */
    }


    /// Chain the basic block into the list of basic blocks.
    void commit_to_basic_block(basic_block_state &bb) throw() {
        basic_block_state *prev(nullptr);
        basic_block_state *curr(&bb);
        do {
            prev = BASIC_BLOCKS.load();
            curr->next = prev;
        } while(!BASIC_BLOCKS.compare_exchange_weak(prev, curr));
    }


    /// Invoked if/when Granary discards a basic block (e.g. due to a race
    /// condition when two cores compete to translate the same basic block).
    void discard_basic_block(basic_block_state &bb) throw() {
        free_memory<basic_block_edge>(bb.edges, bb.num_edges);
    }


    /// Initialise the basic block state and add in calls to event handlers.
    static void instrument_basic_block(
        granary::basic_block_state &bb,
        instruction_list &ls
    ) throw() {
        instruction in(ls.last());
        register_manager used_regs;
        register_manager entry_regs;

        bb.num_executions = 0;
        IF_KERNEL( bb.num_interrupts = 0; )

        used_regs.kill_all();
        entry_regs.kill_all();

        IF_KERNEL( app_pc start_pc(nullptr); )
        IF_KERNEL( app_pc end_pc(nullptr); )

        for(instruction prev_in; in.is_valid(); in = prev_in) {

            prev_in = in.prev();

            // Track the registers.
            used_regs.revive(in);
            entry_regs.visit(in);

#if GRANARY_IN_KERNEL
            // Track the translation so that we can find the offset.
            if(in.pc()) {
                app_pc curr_pc(in.pc());
                if(curr_pc) {
                    start_pc = curr_pc;
                    if(!end_pc) {
                        end_pc = curr_pc;
                    }
                }
            }

            // Is this a memory allocator?
            allocator_kind alloc_kind;
            if(MEMORY_ALLOCATORS->load(start_pc, alloc_kind)) {
                if(MEMORY_ALLOCATOR == alloc_kind) {
                    bb.is_allocator = true;
                } else if(MEMORY_DEALLOCATOR == alloc_kind) {
                    bb.is_deallocator = true;
                } else {
                    bb.is_allocator = true;
                    bb.is_deallocator = true;
                }
            }
#endif /* GRANARY_IN_KERNEL */

            // Call, switch into a the entry basic block policy.
            if(in.is_call()) {

                instrumentation_policy target_policy =
                    policy_for<cfg_entry_policy>();
                target_policy.force_attach(true);
                in.set_policy(target_policy);
                insert_clean_call_after(ls, in, EVENT_AFTER_FUNCTION, &bb);

            // Returning from a function.
            } else if(in.is_return()) {

                bb.is_function_exit = true;
                in = ls.insert_before(in, label_());
                insert_clean_call_after(ls, in, EVENT_EXIT_FUNCTION, &bb);

            // Non-call CTI.
            } else if(in.is_cti()) {
                bb.num_outgoing_jumps += 1;

                // Is this an indirect jump?
                operand target(in.cti_target());
                if(!dynamorio::opnd_is_pc(target)) {
                    bb.has_outgoing_indirect_jmp = true;
                }

                instrumentation_policy target_policy =
                    policy_for<cfg_exit_policy>();
                target_policy.force_attach(true);
                in.set_policy(target_policy);
            }
        }

        bb.block_id = BASIC_BLOCK_ID.fetch_add(1);
        bb.used_regs = used_regs.encode();
        bb.entry_regs = entry_regs.encode();

#if GRANARY_IN_KERNEL
        // Record the name and relative offset of the code within the
        // module. We can correlate this offset with an `objdump` or do
        // static instrumentation of the ELF/KO based on this offset.
        const kernel_module *module(kernel_get_module(start_pc));
        if(module) {
            const uintptr_t app_begin(
                reinterpret_cast<uintptr_t>(module->text_begin));

            const uintptr_t start_pc_uint(
                reinterpret_cast<uintptr_t>(start_pc));

            bb.app_offset_begin = start_pc_uint - app_begin;

            bb.num_bytes_in_block = reinterpret_cast<uintptr_t>(end_pc) \
                                  - start_pc_uint;

            bb.app_name = module->name;
        }
#endif /* GRANARY_IN_KERNEL */

        // Add the edges here because it's technically possible for a race to
        // occur where one core will observe the basic block once it's been
        // added to the code cache, but before it's been committed to.
        bb.num_edges = NUM_START_EDGES;
        bb.edges = allocate_memory<basic_block_edge>(NUM_START_EDGES);
    }


    granary::instrumentation_policy cfg_entry_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        instrument_basic_block(bb, ls);
        bb.is_function_entry = true;
        bb.is_app_code = true;
        bb.function_id = bb.block_id;

        // Add an event handler that executes before the basic block executes.
        instruction in(ls.insert_before(ls.first(), label_()));
        insert_clean_call_after(ls, in, EVENT_ENTER_FUNCTION, &bb);

        return policy_for<cfg_exit_policy>();
    }


    granary::instrumentation_policy cfg_entry_policy::visit_host_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        instrument_basic_block(bb, ls);
        bb.is_function_entry = true;
        bb.is_app_code = false;
        bb.function_id = bb.block_id;

        // Add an event handler that executes before the basic block executes.
        instruction in(ls.insert_before(ls.first(), label_()));
        insert_clean_call_after(ls, in, EVENT_ENTER_FUNCTION, &bb);

        return granary::policy_for<cfg_exit_policy>();
    }


    granary::instrumentation_policy cfg_root_policy::visit_app_instructions(
        granary::cpu_state_handle cpu,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        bb.is_root = true;
        return cfg_entry_policy::visit_app_instructions(cpu, bb, ls);
    }


    granary::instrumentation_policy cfg_root_policy::visit_host_instructions(
        granary::cpu_state_handle cpu,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        bb.is_root = true;
        return cfg_entry_policy::visit_host_instructions(cpu, bb, ls);
    }


    granary::instrumentation_policy cfg_exit_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        instrument_basic_block(bb, ls);
        bb.is_function_entry = false;
        bb.is_app_code = true;

        // Add an event handler that executes before the basic block executes.
        instruction in(ls.insert_before(ls.first(), label_()));
        insert_clean_call_after(ls, in, EVENT_ENTER_BASIC_BLOCK, &bb);

        return policy_for<cfg_exit_policy>();
    }


    granary::instrumentation_policy cfg_exit_policy::visit_host_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        instrument_basic_block(bb, ls);
        bb.is_function_entry = false;
        bb.is_app_code = false;

        // Add an event handler that executes before the basic block executes.
        instruction in(ls.insert_before(ls.first(), label_()));
        insert_clean_call_after(ls, in, EVENT_ENTER_BASIC_BLOCK, &bb);

        return granary::policy_for<cfg_exit_policy>();
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT

    granary::interrupt_handled_state cfg_entry_policy::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &bb,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        bb.num_interrupts.fetch_add(1);
        return granary::INTERRUPT_DEFER;
    }


    granary::interrupt_handled_state cfg_exit_policy::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &bb,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        bb.num_interrupts.fetch_add(1);
        return granary::INTERRUPT_DEFER;
    }


    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }

#endif

}

