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


/// Set to 1 iff jumps that keep control within the same basic block should be
/// patched to jump directly back into the same basic block instead of being
/// turned into slot-based direct jump lookups.
#define CONFIG_BB_PATCH_LOCAL_BRANCHES 1


/// Set to 1 iff basic blocks should contain the instructions immediately
/// following a conditional branch. If enabled, basic blocks will be bigger.
#define CONFIG_BB_EXTEND_BBS_PAST_CBRS 0


/// Upper bound on the number of processor cores.
#ifndef CONFIG_MAX_NUM_CPUS
#   define CONFIG_MAX_NUM_CPUS 8
#endif


/// Lower bound on the cache line size.
#ifndef CONFIG_MIN_CACHE_LINE_SIZE
#   define CONFIG_MIN_CACHE_LINE_SIZE 64
#endif


/// Exact size of memory pages.
#ifndef CONFIG_MEMORY_PAGE_SIZE
#   define CONFIG_MEMORY_PAGE_SIZE 4096
#endif


namespace granary {

    /// Program counter type.
    typedef dynamorio::app_pc app_pc;


    enum {

        /// Size in bytes of each memory page.
        PAGE_SIZE = CONFIG_MEMORY_PAGE_SIZE,


        /// Some non-zero positive multiple of the cache line size.
        CACHE_LINE_SIZE = CONFIG_MIN_CACHE_LINE_SIZE,


        /// number of processor cores.
        NUM_CPUS = CONFIG_MAX_NUM_CPUS
    };

    enum {

        /// Bounds on where kernel module code is placed
        KERNEL_MODULE_START = 0xffffffffa0000000ULL,
        KERNEL_MODULE_END = 0xfffffffffff00000ULL
    };
}


extern "C" {
#if !GRANARY_IN_KERNEL
    extern void granary_break_on_fault(void);
    extern void granary_break_on_encode(dynamorio::app_pc pc,
                                        dynamorio::instr_t *instr);
    extern void granary_break_on_allocate(void *ptr);
    extern int granary_test_return_true(void);
    extern int granary_test_return_false(void);
#endif
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

        static_test_list(void) throw();
    };

    extern static_test_list STATIC_TEST_LIST_HEAD;
    extern void run_tests(void) throw();
}


#include "granary/utils.h"
#include "granary/type_traits.h"
#include "granary/allocator.h"

#endif /* granary_GLOBALS_H_ */
