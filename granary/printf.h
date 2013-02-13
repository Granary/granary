/*
 * printf.h
 *
 *  Created on: 2013-02-13
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_PRINTF_H_
#define Granary_PRINTF_H_

#include "globals.h"

namespace granary {

#if GRANARY_IN_KERNEL
    extern int (*printf)(const char *, ...);
#else
    int printf(const char *format, ...) throw();
#endif

}

#endif /* Granary_PRINTF_H_ */
