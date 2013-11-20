/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * printf.cc
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/globals.h"
#include "granary/kernel/printf.h"
#include "granary/types.h"

#define LOG !CONFIG_FEATURE_INSTRUMENT_HOST

namespace granary {
#if LOG
    int (*printf)(const char *, ...) = types::printk;
#else
    static int sink_printf(const char *, ...) throw() { return 0; }
    int (*printf)(const char *, ...) = sink_printf;
#endif
}

