/*
 * state.cc
 *
 *  Created on: 2012-11-19
 *      Author: pag
 *     Version: $Id$
 */

#include <atomic>

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/types.h"


extern "C" {
    extern granary::cpu_state *get_percpu_state(void *);
}


namespace granary {


    static void *CPU_STATES;


    /// Gets a handle to the current CPU state.
    cpu_state_handle::cpu_state_handle(void) throw()
        : state(get_percpu_state(CPU_STATES))
    { }


    /// Initialise the per-cpu state, including a new interrupt descriptor
    /// table.
    void init_cpu_state(system_table_register_t *idt) throw() {

        cpu_state *state(new (get_percpu_state(CPU_STATES)) cpu_state);

        app_pc handler(global_state::FRAGMENT_ALLOCATOR->\
            allocate_array<uint8_t>(INTERRUPT_DELAY_CODE_SIZE + 16));
        handler += ALIGN_TO(reinterpret_cast<uint64_t>(handler), 16);
        state->interrupt_delay_handler = handler;

        set_idtr(idt);
    }


    /// Initialise the state.
    void cpu_state_handle::init(void) throw() {

        CPU_STATES = types::__alloc_percpu(
            sizeof(cpu_state), alignof(cpu_state));

        system_table_register_t idt(emit_idt());

        types::on_each_cpu(
            unsafe_cast<void (*)(void *)>(init_cpu_state),
            &idt,
            1);
    }


    /// Gets a handle to the current thread state.
    thread_state_handle::thread_state_handle(void) throw()
        : state(nullptr)
    { }
}
