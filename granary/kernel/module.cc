/*
 * module.cc
 *
 *  Created on: 2012-11-08
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/globals.h"
#include "granary/instruction.h"

#include "granary/kernel/module.h"
#include "granary/kernel/printf.h"

#ifdef GRANARY_DONT_INCLUDE_CSTDLIB
#   undef GRANARY_DONT_INCLUDE_CSTDLIB
#endif

#include "granary/types.h"

extern "C" {
    void notify_module_state_change(struct kernel_module *module) {
        using namespace granary;

        if(module->is_granary) {
            return;
        }

        types::module *mod(unsafe_cast<types::module *>(module->address));

        granary::printf("Notified about module (%s) state change!!\n", mod->name);

        (void) mod;
    }
}
