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

    /// Allocates memory for an interrupt descriptor table.
    extern granary::detail::interrupt_descriptor_table *
    granary_allocate_idt(void);
}


namespace granary {


    static void *CPU_STATES;


    /// Gets a handle to the current CPU state.
    cpu_state_handle::cpu_state_handle(void) throw()
        : state(get_percpu_state(CPU_STATES))
    { }


    /// Initialise the per-cpu state, including a new interrupt descriptor
    /// table.
    void init_cpu_state(app_pc common_vector_handler) throw() {

        cpu_state *state(new (get_percpu_state(CPU_STATES)) cpu_state);

        system_table_register_t native;
        system_table_register_t instrumented;
        detail::interrupt_descriptor_table *idt(granary_allocate_idt());

        get_idtr(&native);

        state->granary_idt = idt;
        state->kernel_idt = unsafe_cast<detail::interrupt_descriptor_table *>(
            native.base);

        memcpy(state->granary_idt, state->kernel_idt, sizeof *idt);

        for(unsigned i(0); i < NUM_INTERRUPT_VECTORS; ++i) {
            descriptor_t *i_vec(&(idt->vectors[i * 2]));
            descriptor_t *n_vec(&(native.base[i * 2]));

            // update the gate
            if(GATE_DESCRIPTOR == get_descriptor_kind(i_vec)) {
                app_pc native_handler(get_gate_target_offset(&(n_vec->gate)));

                if(!is_host_address(native_handler)) {
                    continue;
                }

                set_gate_target_offset(
                    &(i_vec->gate),
                    emit_interrupt_routine(
                        i,
                        native_handler,
                        common_vector_handler));
            }
        }

        instrumented.base = &(idt->vectors[0]);
        instrumented.limit = (sizeof *idt) - 1;

        set_idtr(&instrumented);
    }


    /// Initialise the state.
    void cpu_state_handle::init(void) throw() {

        CPU_STATES = types::__alloc_percpu(
            sizeof(cpu_state), alignof(cpu_state));

        types::on_each_cpu(
            unsafe_cast<void (*)(void *)>(init_cpu_state),
            emit_common_interrupt_routine(),
            1);
    }


    /// Gets a handle to the current thread state.
    thread_state_handle::thread_state_handle(void) throw()
        : state(nullptr)
    { }
}
