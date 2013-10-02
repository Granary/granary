/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * init.cc
 *
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/code_cache.h"

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
    void static_init_list::append(static_init_list &entry) throw() {
        if(!STATIC_INIT_LIST_TAIL) {
            STATIC_INIT_LIST_HEAD.next = &entry;
        } else {
            STATIC_INIT_LIST_TAIL->next = &entry;
        }

        STATIC_INIT_LIST_TAIL = &entry;
    }


#if GRANARY_IN_KERNEL
    /// List of static initialisers to be run at granary's kernel initialiser
    /// within a `stop_machine` call.
    static static_init_list STATIC_INIT_LIST_SYNC_HEAD;
    static static_init_list *STATIC_INIT_LIST_SYNC_TAIL(nullptr);

    /// Add an entry onto the static initialiser list.
    void static_init_list::append_sync(static_init_list &entry) throw() {
        if(!STATIC_INIT_LIST_SYNC_TAIL) {
            STATIC_INIT_LIST_SYNC_HEAD.next = &entry;
        } else {
            STATIC_INIT_LIST_SYNC_TAIL->next = &entry;
        }

        STATIC_INIT_LIST_SYNC_TAIL = &entry;
    }
#endif


    /// Initialise granary.
    void init(void) throw() {

        IF_KERNEL( cpu_state::init_early(); )

        // Run all static initialiser functions.
        static_init_list *init(STATIC_INIT_LIST_HEAD.next);
        for(; init; init = init->next) {
            if(init->exec) {
                IF_KERNEL( eflags flags = granary_disable_interrupts(); )

                cpu_state_handle cpu;
                cpu.free_transient_allocators();

                init->exec();
                IF_KERNEL( granary_store_flags(flags); )
            }
        }

        IF_KERNEL( cpu_state::init_late(); )

#ifdef CLIENT_init
        client::init();
#endif
    }


#if GRANARY_IN_KERNEL

    /// Returns true iff there is anything to run in a synchronised way.
    bool should_init_sync(void) throw() {
        return nullptr != STATIC_INIT_LIST_SYNC_HEAD.next;
    }


    /// Initialise the synchronised static initialisers.
    void init_sync(void) throw() {

        // Run all static initialiser functions.
        static_init_list *init(STATIC_INIT_LIST_SYNC_HEAD.next);

        for(; init; init = init->next) {
            if(init->exec) {

                // Free up temporary resources before executing a CPU-specific
                // Thing.
                IF_KERNEL( eflags flags = granary_disable_interrupts(); )
                cpu_state_handle cpu;
                cpu.free_transient_allocators();

                init->exec();

                IF_KERNEL( granary_store_flags(flags); )
            }
        }
    }
#endif /* GRANARY_IN_KERNEL */
}

