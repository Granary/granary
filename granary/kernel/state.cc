/*
 * state.cc
 *
 *  Created on: 2012-11-19
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/state.h"
#include "granary/kernel/linux/per_cpu.h"

namespace granary {

    namespace {
        cpu_state CPU_PRIV_STATE __attribute__((section (".percpu")));
    }


    cpu_state_handle::cpu_state_handle(void) throw()
        : state(&CPU_PRIV_STATE)
    { }
}
