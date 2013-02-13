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

    int printf(const char *format, ...) throw() {
        va_list args;
        va_start(args, format);
        int ret = vprintf(format, args);
        va_end(args);
        return ret;
    }

}

