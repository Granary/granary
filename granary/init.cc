/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * init.cc
 *
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/code_cache.h"

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"

#include "clients/init.h"

namespace granary {


    /// List of static initialisers to be run at granary::init. The separation
    /// between normal static initialisation and granary initialisation is
    /// useful for debugging, especially in user space where there might be
    /// bugs that can't usefully be caught *before* any signal handlers can be
    /// attached, and that don't show up in debuggers.
    static static_init_list STATIC_INIT_LIST_HEAD;
    static static_init_list *STATIC_INIT_LIST_TAIL(nullptr);

    /// Add an entry onto the static initialiser list.
    void static_init_list::append(static_init_list &entry) {
        if(!STATIC_INIT_LIST_TAIL) {
            STATIC_INIT_LIST_HEAD.next = &entry;
        } else {
            STATIC_INIT_LIST_TAIL->next = &entry;
        }

        STATIC_INIT_LIST_TAIL = &entry;
    }


#if CONFIG_ENV_KERNEL
    /// List of static initialisers to be run at granary's kernel initialiser
    /// within a `stop_machine` call.
    static static_init_list STATIC_INIT_LIST_SYNC_HEAD;
    static static_init_list *STATIC_INIT_LIST_SYNC_TAIL(nullptr);

    /// Add an entry onto the static initialiser list.
    void static_init_list::append_sync(static_init_list &entry) {
        if(!STATIC_INIT_LIST_SYNC_TAIL) {
            STATIC_INIT_LIST_SYNC_HEAD.next = &entry;
        } else {
            STATIC_INIT_LIST_SYNC_TAIL->next = &entry;
        }

        STATIC_INIT_LIST_SYNC_TAIL = &entry;
    }
#endif /* CONFIG_ENV_KERNEL */


    extern "C" {

        void granary_do_init_on_private_stack(static_init_list *init) {
            init->exec();
        }


        extern void granary_enter_private_stack(void);
        extern void granary_exit_private_stack(void);
    }


    namespace detail {
        extern void init_code_cache(void) ;
    }


    /// Initialise granary.
    void init(void) {

        detail::init_code_cache();

        IF_KERNEL( cpu_state::init_early(); )

        // Run all static initialiser functions.
        static_init_list *init(STATIC_INIT_LIST_HEAD.next);
        for(; init; init = init->next) {
            if(!init->exec) {
                continue;
            }

            IF_KERNEL( eflags flags = granary_disable_interrupts(); )
            cpu_state_handle cpu;

            IF_TEST( cpu->in_granary = false; )
            cpu.free_transient_allocators();

            ASM(
                "movq %0, %%" TO_STRING(ARG1) ";"
                TO_STRING(PUSHA_ASM_ARG)
                "callq " TO_STRING(SHARED_SYMBOL(granary_enter_private_stack)) ";"
                "callq " TO_STRING(SHARED_SYMBOL(granary_do_init_on_private_stack)) ";"
                "callq " TO_STRING(SHARED_SYMBOL(granary_exit_private_stack)) ";"
                TO_STRING(POPA_ASM_ARG)
                :
                : "m"(init)
                : "%" TO_STRING(ARG1)
            );

            IF_KERNEL( granary_store_flags(flags); )
        }

        IF_KERNEL( cpu_state::init_late(); )

#ifdef CLIENT_init
        client::init();
#endif
    }


#if CONFIG_ENV_KERNEL

    /// Returns true iff there is anything to run in a synchronised way.
    bool should_init_sync(void) {
        return nullptr != STATIC_INIT_LIST_SYNC_HEAD.next;
    }


    /// Initialise the synchronised static initialisers.
    void init_sync(void) {

        // Run all static initialiser functions.
        static_init_list *init(STATIC_INIT_LIST_SYNC_HEAD.next);

        for(; init; init = init->next) {
            if(!init->exec) {
                continue;
            }

            // Free up temporary resources before executing a CPU-specific
            // Thing.
            IF_KERNEL( eflags flags = granary_disable_interrupts(); )
            cpu_state_handle cpu;
            IF_TEST( cpu->in_granary = false; )
            cpu.free_transient_allocators();

            ASM(
                "movq %0, %%rdi;"
                "callq " TO_STRING(SHARED_SYMBOL(granary_enter_private_stack)) ";"
                "callq " TO_STRING(SHARED_SYMBOL(granary_do_init_on_private_stack)) ";"
                "callq " TO_STRING(SHARED_SYMBOL(granary_exit_private_stack)) ";"
                :
                : "m"(init)
                : "%rdi"
            );

            IF_KERNEL( granary_store_flags(flags); )
        }
    }
#endif /* CONFIG_ENV_KERNEL */
}

