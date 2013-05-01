/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * printf.cc
 *
 *  Created on: 2013-02-13
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/printf.h"

#define LOG 1

#if LOG
#   include <cstdio>
#   include <cstdarg>
#endif

namespace granary {
#if LOG
    static FILE *granary_out(fopen("granary.log", "w"));
#endif

    int printf(const char *format, ...) throw() {
#if !LOG
        (void) format;
        return 0;
#else
        va_list args;
        va_start(args, format);
        int ret = vfprintf(granary_out, format, args);
        va_end(args);
        fflush(granary_out);
        return ret;
#endif
    }
}

