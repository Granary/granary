/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * syscall.cc
 *
 *  Created on: 2013-11-06
 *          Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/detach.h"
#include "granary/state.h"
#include "granary/policy.h"
#include "granary/code_cache.h"
#include "granary/basic_block.h"
#include "granary/emit_utils.h"


#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"


#if CONFIG_ENABLE_TRACE_ALLOCATOR && CONFIG_TRACE_ALLOCATE_ENTRY_SYSCALL
#   define IF_CUSTOM_ALLOCATOR(...) __VA_ARGS__
#else
#   define IF_CUSTOM_ALLOCATOR(...)
#endif


namespace granary {

#if defined(DETACH_ADDR_sys_call_table) \
&& DETACH_LENGTH_sys_call_table \
&& CONFIG_FEATURE_INSTRUMENT_HOST

    enum {
        NUM_ENTRIES = DETACH_LENGTH_sys_call_table / sizeof(void *)
    };


    /// Actual shadow syscall table.
    app_pc SYSCALL_TABLE[NUM_ENTRIES] = {nullptr};


    // Native syscall table in the kernel.
    extern "C" {
        app_pc *NATIVE_SYSCALL_TABLE = \
            reinterpret_cast<app_pc *>(DETACH_ADDR_sys_call_table);


        intptr_t SHADOW_SYSCALL_TABLE = reinterpret_cast<uintptr_t>(
            &(SYSCALL_TABLE[0]));
    }


    GRANARY_ENTRYPOINT
    static void takeover_syscall(unsigned entry) throw() {

        cpu_state_handle cpu;
        enter(cpu);

        // Starting policy.
        instrumentation_policy policy(START_POLICY);
        policy.begins_functional_unit(true);
        policy.in_host_context(true);
        policy.return_address_in_code_cache(true);

        instrumentation_policy base_policy(policy.base_policy());
        mangled_address am(NATIVE_SYSCALL_TABLE[entry], policy);

        // If we've already translated this syscall then don't needlessly
        // create another allocator.
        app_pc target_addr(code_cache::lookup(am.as_address));
        if(!target_addr) {
            mangled_address base_am(NATIVE_SYSCALL_TABLE[entry], base_policy);
            target_addr = code_cache::lookup(base_am.as_address);
        }
        if(target_addr) {
            SYSCALL_TABLE[entry] = target_addr;
            return;
        }

        // If we're using the trace allocator then add an allocator in one
        // of two cases:
        //      1) We're tracing system call entrypoints (default trace
        //         allocator strategy).
        //      2) We're tracing functional units.
        IF_CUSTOM_ALLOCATOR( cpu->current_fragment_allocator = \
            allocate_memory<generic_fragment_allocator>(); )

        target_addr = code_cache::find(cpu, am);

        // Overwrite what's there (the lazy takeover routine) with the fully
        // resolved routine.
        SYSCALL_TABLE[entry] = target_addr;
    }


    extern "C" {

        /// Make a routine that will lazily take over a system call entrypoint.
        ///
        /// Lazy takeover allows us to defer slab allocator for unused syscalls.
        GRANARY_ENTRYPOINT
        app_pc granary_syscall_takeover_routine(unsigned entry) {

            cpu_state_handle cpu;
            enter(cpu);

            instruction_list ls;
            register_manager rm;
            rm.kill_all();

            // Get ready to switch stacks and call out to the handler.
            instruction in(save_and_restore_registers(rm, ls, ls.last()));

            // Switch to the private stack.
            in = insert_cti_after(
                ls, in,
                unsafe_cast<app_pc>(granary_enter_private_stack),
                CTI_STEAL_REGISTER, reg::ret,
                CTI_CALL);

            // Call the function to take over this particular syscall.
            in = ls.insert_after(in, mov_imm_(reg::arg1_32, int32_(entry)));
            in = insert_cti_after(
                ls, in, // instruction
                unsafe_cast<app_pc>(takeover_syscall), // target
                CTI_STEAL_REGISTER, reg::ret, // clobber reg
                CTI_CALL);

            // Switch back to original stack.
            in = insert_cti_after(
                ls, in, unsafe_cast<app_pc>(granary_exit_private_stack),
                CTI_STEAL_REGISTER, reg::ret,
                CTI_CALL);

            // Hack-ish!
            instruction target_label(label_());
            target_label.instr->translation = unsafe_cast<app_pc>(
                &(SYSCALL_TABLE[entry]));
            target_label.instr->bytes = target_label.instr->translation;
            target_label.instr->note = target_label.instr->translation;

            // Jump to the newly created syscall entrypoint.
            ls.append(jmp_ind_(mem_instr_(target_label)));

            // Encode.
            const unsigned size(ls.encoded_size());
            app_pc routine(reinterpret_cast<app_pc>(
                global_state::FRAGMENT_ALLOCATOR-> \
                    allocate_untyped(CACHE_LINE_SIZE, size)));

            ls.encode(routine, size);

            // Duplicate here so that this works for both delayed and normal
            // syscall takeover.
            SYSCALL_TABLE[entry] = routine;

            return routine;
        }


        bool granary_takeover_next_syscall_entry(void) {
#if !CONFIG_DELAYED_TAKEOVER || !CONFIG_FEATURE_INSTRUMENT_HOST
            return false;
#else
            static unsigned next_entry(0);
            if(next_entry >= NUM_ENTRIES) {
                return false;
            }

            eflags flags = granary_disable_interrupts();
            ASM(
                "movq %0, %%" TO_STRING(ARG1) ";"
                TO_STRING(PUSHA_ASM_ARG)
                "callq " TO_STRING(SHARED_SYMBOL(granary_enter_private_stack)) ";"
                "callq " TO_STRING(SHARED_SYMBOL(granary_syscall_takeover_routine)) ";"
                "callq " TO_STRING(SHARED_SYMBOL(granary_exit_private_stack)) ";"
                TO_STRING(POPA_ASM_ARG)
                :
                : "m"(next_entry)
                : "%" TO_STRING(ARG1)
            );
            granary_store_flags(flags);

            ++next_entry;
            return true;
#endif
        }
    }


#if CONFIG_DELAYED_TAKEOVER
#   define IF_NOT_DELAYED_TAKEOVER(...)
#else
#   define IF_NOT_DELAYED_TAKEOVER(...) __VA_ARGS__
#endif


    /// Creates a shadow system call table.
    STATIC_INITIALISE_ID(duplicate_syscall_table, {
        for(unsigned i(0); i < NUM_ENTRIES; ++i) {
            SYSCALL_TABLE[i] = NATIVE_SYSCALL_TABLE[i];
            IF_NOT_DELAYED_TAKEOVER(
                SYSCALL_TABLE[i] = granary_syscall_takeover_routine(i); )
        }
    })

#else
    extern "C" {
        intptr_t NATIVE_SYSCALL_TABLE = 0;
        intptr_t SHADOW_SYSCALL_TABLE = 0;
    }
#endif
}


