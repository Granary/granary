/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * dlmain.cc
 *
 *  Created on: 2013-02-13
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/globals.h"
#include "granary/policy.h"
#include "granary/code_cache.h"
#include "granary/basic_block.h"
#include "granary/attach.h"
#include "granary/detach.h"
#include "granary/perf.h"

#define CATCH_SIGNALS 0

#if CATCH_SIGNALS
#   include <unistd.h>
#   include <signal.h>
#   include <cstdio>
void granary_signal_handler(int) {
    std::fprintf(stderr, "Run `sudo gdb attach %d`\n", getpid());
    for(;;) { /* loop until we manually attach gdb */ }
}
#endif

extern "C" {

    __attribute__((constructor))
    static void granary_begin_program(void) {

#if CATCH_SIGNALS
        signal(SIGSEGV, granary_signal_handler);
        signal(SIGABRT, granary_signal_handler);
        signal(SIGILL, granary_signal_handler);
#endif

        granary::init();
        granary::attach(granary::START_POLICY);
    }


#if CONFIG_ENABLE_PERF_COUNTS

    __attribute__((destructor))
    static void granary_end_program(void) {
        granary::perf::report();
    }

    GRANARY_DETACH_POINT(granary_end_program)
#endif

} /* extern C */


