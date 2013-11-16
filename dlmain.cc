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

#include <unistd.h>
#include <cstdio>
#include <csignal>
#include <ucontext.h>

#define DEBUG_GDB_ATTACH_AT_INIT 1
#define DEBUG_GDB_ATTACH_AT_SIGSEGV 0

using namespace granary;

extern "C" {

#if DEBUG_GDB_ATTACH_AT_SIGSEGV
    /// Handle a segfault by trying to attach instrumentation to native code.
    static void handle_fault(int, siginfo_t *, void *) throw() {
        detach();
        granary::printf("Process ID for attaching GDB: %d\n", (int) getpid());
        granary::printf("Press enter to continue.\n");
        getchar();
    }


    /// Attach the signal handler when the program starts up.
    STATIC_INITIALISE({
        static struct sigaction sa;

        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = &handle_fault;

        sigaction(SIGSEGV, &sa, nullptr);
    })
#endif

    __attribute__((constructor))
    static void granary_begin_program(void) {

#if DEBUG_GDB_ATTACH_AT_INIT
        granary::printf("Process ID for attaching GDB: %d\n", (int) getpid());
        granary::printf("Press enter to continue.\n");
        getchar();
#endif

        granary::init();
        granary::attach(granary::START_POLICY);
    }


#if CONFIG_DEBUG_PERF_COUNTS

    __attribute__((destructor))
    static void granary_end_program(void) {
        granary::perf::report();
    }

    GRANARY_DETACH_POINT(granary_end_program)
#endif

} /* extern C */


