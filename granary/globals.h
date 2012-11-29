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
#include "granary/pp.h"

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
    extern void granary_break_on_fault(void);
    extern int granary_test_return_true(void);
    extern int granary_test_return_false(void);
    extern int granary_asm_apic_id(void);
    extern void granary_atomic_write8(uint64_t, uint64_t *);
}

namespace granary {

    /// This is a sort of hack to ensure that static initializers are
    /// compiled to execute.
    struct static_init_list {
        static_init_list *next;
    };

    extern static_init_list STATIC_LIST_HEAD;

    /// Another static init hack to add test cases to be run automatically.
    struct static_test_list {
        void (*func)(void);
        const char *desc;
        static_test_list *next;
    };

    extern static_test_list STATIC_TEST_LIST_HEAD;
    extern void run_tests(void) throw();
}


#include "granary/utils.h"
#include "granary/type_traits.h"
#include "granary/allocator.h"

#endif /* granary_GLOBALS_H_ */
