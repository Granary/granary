/*
 * init.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/code_cache.h"

namespace granary {


    IF_KERNEL(extern void init_kernel(void) throw();)
    IF_USER(extern void init_user(void) throw();)


    /// List of static initialisers to be run at granary::init. The separation
    /// between normal static initialisation and granary initialisation is
    /// useful for debugging, especially in user space where there might be
    /// bugs that can't usefully be caught *before* any signal handlers can be
    /// attached, and that don't show up in debuggers.
    static static_init_list STATIC_INIT_LIST_HEAD;


    /// Add an entry onto the static initialiser list.
    void static_init_list::append(static_init_list &entry) throw() {
        entry.next = STATIC_INIT_LIST_HEAD.next;
        STATIC_INIT_LIST_HEAD.next = &entry;
    }


    /// Initialise granary.
    void init(void) throw() {

        // run all static initialisers.
        static_init_list *init(STATIC_INIT_LIST_HEAD.next);
        for(; init; init = init->next) {
            if(init->exec) {
                init->exec();
            }
        }

        // initialise the ibl entry routine.
        code_cache::init_ibl();

        // initialise for kernel or user space.
        IF_KERNEL(init_kernel());
        IF_USER(init_user());
    }

}

extern "C" {
    void __cxa_pure_virtual(void) {
        granary_break_on_fault();
        granary_fault();
    }
}

