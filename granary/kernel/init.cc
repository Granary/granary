/*
 * init.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/init.h"
#include "granary/globals.h"
#include "granary/state.h"
#include "granary/types.h"

namespace granary {

    void init_kernel(void) throw() {
        cpu_state_handle::init();
    }

}

