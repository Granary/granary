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

    /// Run a function on each CPU.
    void kernel_run_on_each_cpu(void (*func)(void));


    /// Call a function where all CPUs are synchronised.
    void kernel_run_synchronised(void (*func)(void));
}


namespace granary {

    /// Replace the interrupt descriptor tables.
    static void init_cpu(void) throw() {
        cpu_state_handle::init();

#if CONFIG_HANDLE_INTERRUPTS || CONFIG_INSTRUMENT_HOST
        system_table_register_t idt(create_idt());
        set_idtr(&idt);
#endif

#if CONFIG_INSTRUMENT_HOST
        app_pc syscall_pc(create_syscall_entrypoint());
        set_msr(MSR_LSTAR, reinterpret_cast<uint64_t>(syscall_pc));
#endif
    }


    /// Initialise Granary in kernel space.
    void init_kernel(void) throw() {

        kernel_run_on_each_cpu(init_cpu);

        if(should_init_sync()) {
            kernel_run_synchronised(&init_sync);
        }
    }

}

