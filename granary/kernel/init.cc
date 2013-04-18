/*
 * init.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/init.h"
#include "granary/globals.h"
#include "granary/state.h"

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
    }

}

