/*
 * printf.cc
 *
 *  Created on: 2013-02-13
 *      Author: pag
 *     Version: $Id$
 */

#include <cstdio>
#include <cstdarg>

#include "granary/printf.h"

namespace granary {
    static FILE *granary_out(fopen("granary.log", "w"));

    int printf(const char *format, ...) throw() {
        va_list args;
        va_start(args, format);
        int ret = vfprintf(granary_out, format, args);
        va_end(args);
        fflush(granary_out);
        return ret;
    }
}

