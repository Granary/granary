/*
 * globals.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_GLOBALS_H_
#define granary_GLOBALS_H_

#include <cstring>
#include <cstddef>
#include <stdint.h>

#include "granary/types/dynamorio.h"

namespace granary {

    /// Program counter type.
    typedef typename dynamorio::app_pc app_pc;


    enum {

        /// Size in bytes of each memory page
        PAGE_SIZE = 4096U,


        /// some non-zero positive multiple of the cache line size;
        /// used to pad some st
        CACHE_LINE_SIZE = 64U,


        /// number of processor cores
        NUM_CPUS = 8,


        /// Number of per-thread direct branch lookup slots to use.
        NUM_DIRECT_BRANCH_SLOTS = 16ULL
    };

    enum {

        /// Bounds on where kernel module code is placed
        KERNEL_MODULE_START = 0xffffffffa0000000ULL,
        KERNEL_MODULE_END = 0xfffffffffff00000ULL
    };
}


extern "C" {
    extern void break_before_fault(void);
}


#include "granary/pp.h"
#include "granary/utils.h"
#include "granary/type_traits.h"
#include "granary/allocator.h"

#endif /* granary_GLOBALS_H_ */
