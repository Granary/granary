/*
 * init.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "globals.h"

namespace granary {


    IF_KERNEL(extern void init_kernel(void) throw();)
    IF_USER(extern void init_user(void) throw();)


    /// List of static initializers to be run at granary::init. The separation
    /// between normal static initialization and granary initialization is
    /// useful for debugging, especially in user space where there might be
    /// bugs that can't usefully be caught *before* any signal handlers can be
    /// attached, and that don't show up in debuggers.
    static static_init_list STATIC_INIT_LIST_HEAD;


    /// Add an entry onto the static initializer list.
    void static_init_list::append(static_init_list &entry) throw() {
        entry.next = STATIC_INIT_LIST_HEAD.next;
        STATIC_INIT_LIST_HEAD.next = &entry;
    }


    /// Initialize granary.
    void init(void) throw() {
        static_init_list *init(STATIC_INIT_LIST_HEAD.next);
        for(; init; init = init->next) {
            if(init->exec) {
                init->exec();
            }
        }

        IF_KERNEL(init_kernel());
        IF_USER(init_user());
    }

}

