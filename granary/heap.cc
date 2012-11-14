/*
 * heap.cc
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/heap.h"

extern "C" {

#if GRANARY_IN_KERNEL
#   include "granary/kernel/heap.cc"
#else
#   include "granary/user/heap.cc"
#endif

}

