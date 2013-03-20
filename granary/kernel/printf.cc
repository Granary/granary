/*
 * printf.cc
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/kernel/printf.h"
#include "granary/types.h"

#define LOG 1

namespace granary {
#if LOG
    int (*printf)(const char *, ...) = types::printk;
#else
    static int sink_printf(const char *, ...) throw() { return 0; }
    int (*printf)(const char *, ...) = sink_printf;
#endif
}

