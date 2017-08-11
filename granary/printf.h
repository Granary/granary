/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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

#if CONFIG_ENV_KERNEL
    extern int (*printf)(const char *, ...);
#else
    int printf(const char *format, ...) ;
#endif

}

#endif /* Granary_PRINTF_H_ */
