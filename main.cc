/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>
#include <cstdlib>
#include "granary/globals.h"

#define ENABLE_SEGFAULT_HANDLER 1

#if ENABLE_SEGFAULT_HANDLER
#include <unistd.h>
#include <signal.h>


void segfault_handler(int) {
    printf("Run `gdb attach %d`\n", getpid());
    for(;;) { /* loop until we manually attach gdb */ }
}

#endif

int main(int argc, const char **argv) throw() {
#if ENABLE_SEGFAULT_HANDLER
    signal(SIGSEGV, segfault_handler);
#endif
    (void) argc;
    (void) argv;

    granary::init();

#if CONFIG_RUN_TEST_CASES
    granary::run_tests();
#endif

    IF_PERF( granary::perf::report(); )

    return 0;
}

