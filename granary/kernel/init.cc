/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * init.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/globals.h"
#include "granary/state.h"


extern "C" {
    extern void kernel_run_on_each_cpu(void (*func)(void *), void *thunk);

    /// Call a function where all CPUs are synchronised.
    void kernel_run_synchronised(void (*func)(void));
}


namespace granary {

    /// Replace the interrupt descriptor tables.
    static void init_idt(system_table_register_t *idt) throw() {
#if 0
        UNUSED(idt);
#else
        set_idtr(idt);
#endif
    }


    /// Initialise Granary in kernel space.
    void init_kernel(void) throw() {

        cpu_state_handle::init();
        system_table_register_t idt(create_idt());
        kernel_run_on_each_cpu(
            unsafe_cast<void (*)(void *)>(init_idt),
            reinterpret_cast<void *>(&idt));

        if(should_init_sync()) {
            kernel_run_synchronised(&init_sync);
        }
    }

}

