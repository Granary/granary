/*
 * test_pthreads.cc
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */


#include "granary/test.h"

#if CONFIG_RUN_TEST_CASES

extern "C" {
#   include <pthread.h>
#   include <sys/time.h>
#   include <unistd.h>
}

namespace test {

/*

    static void test_dining_philosophers(void) {
        granary::app_pc func((granary::app_pc) dinner_table);
        granary::basic_block bb_func(granary::code_cache::find(
            func, granary::policy_for<granary::test_policy>()));
        bb_func.call<void>();
    }


    ADD_TEST(test_dining_philosophers,
        "Test that the dining philosophers problem runs correctly under pthreads.")
*/
}

#endif /* CONFIG_RUN_TEST_CASES */
