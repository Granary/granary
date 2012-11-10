/*
 * module.cc
 *
 *  Created on: 2012-11-08
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/utils.h"
#include "granary/module.h"
#include "granary/printf.h"

#include "granary/types/kernel.h"
#include "granary/instruction.h"

extern "C" {
    void notify_module_state_change(struct kernel_module *module) {
        if(module->is_granary) {
            return;
        }

        kernel::module *kernel_module(granary::unsafe_cast<kernel::module *>(module->address));

        granary::printf("Notified about module (%s) state change!!\n", kernel_module->name);

        (void) kernel_module;
    }
}
