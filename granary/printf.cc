/*
 * printf.cc
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/printf.h"

#ifdef __cplusplus
extern "C" {
#endif

int (*printf)(const char *, ...);

#ifdef __cplusplus
}
#endif

