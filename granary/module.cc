/*
 * module.cc
 *
 *  Created on: 2012-11-08
 *      Author: pag
 *     Version: $Id$
 */

#include "module.h"
#include "printf.h"

extern "C" {
    void notify_module_state_change(struct kernel_module *) {
        granary::printf("Notified about module state change!!\n");
    }
}
