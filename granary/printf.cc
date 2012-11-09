/*
 * printf.cc
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/printf.h"

namespace granary {
    int (*printf)(const char *, ...);
}

extern "C" {
    int (**kernel_printf)(const char *, ...) = &(granary::printf);
}

