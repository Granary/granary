/*
 * printf.cc
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/kernel/printf.h"
#include "granary/types.h"

namespace granary {
    int (*printf)(const char *, ...) = types::printk;
}

