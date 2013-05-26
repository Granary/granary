/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * init.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/init.h"
#include "granary/globals.h"
#include "granary/state.h"
#include "granary/detach.h"

extern "C" {
    extern void kernel_run_on_each_cpu(void (*func)(void *), void *thunk);
}


namespace granary {

    /// Replace the interrupt descriptor tables.
    static void init_idt(system_table_register_t *idt) throw() {
        set_idtr(idt);
    }


    /// Initialise Granary in kernel space.
    void init_kernel(void) throw() {

        cpu_state_handle::init();
        system_table_register_t idt(create_idt());
        kernel_run_on_each_cpu(
            unsafe_cast<void (*)(void *)>(init_idt),
            reinterpret_cast<void *>(&idt));

        app_pc detach_app = find_detach_target(
            (app_pc) 0xffffffff811becb0, RUNNING_AS_APP);
        app_pc detach_host = find_detach_target(
            (app_pc) 0xffffffff811becb0, RUNNING_AS_HOST);

        printf("__bread detach app: %p\n", detach_app);
        printf("__bread detach host: %p\n", detach_host);
    }

}

