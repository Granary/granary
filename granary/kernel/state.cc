/*
 * state.cc
 *
 *  Created on: 2012-11-19
 *      Author: pag
 *     Version: $Id$
 */

#include <atomic>

#include "granary/globals.h"

namespace granary {

    namespace {

        /// CPU private info is maintained manually instead of
        /// using the automatic segmentation system. This is
        /// less efficient, but slightly more portable.
        static cache_aligned<cpu_state> CPU_STATES[NUM_CPUS];

        /// The current CPU id
        std::atomic_int_fast32_t NEXT_CPU_ID(0);


        /// maps APIC IDs to cpus
        int APIC_ID_MAP[256] = {-1};
    }


    /// Gets a handle to one of the CPU states. This is a prime
    /// place for optimisation, e.g. by using teh %gs register /
    /// proper CPU-private info.
    cpu_state_handle::cpu_state_handle(void) throw() {
        const int apic_id(granary_asm_apic_id());

        if(-1 == APIC_ID_MAP[apic_id]) {
            APIC_ID_MAP[apic_id] = NEXT_CPU_ID.fetch_add(1);
        }

        state = &(CPU_STATES[APIC_ID_MAP[apic_id]].val);
    }

}
