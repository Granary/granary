/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>
#include <cstdlib>
#include "granary/globals.h"
#include "granary/test.h"

int main(void) throw() {

    using namespace granary;

    init();

#if CONFIG_DEBUG_RUN_TEST_CASES
    run_tests();
#endif

    IF_PERF( perf::report(); )
    return 0;
}

