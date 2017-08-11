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
#include "clients/report.h"
#include <ucontext.h>

#define DEBUG_GDB_ATTACH_AT_INIT 1
#define DEBUG_GDB_ATTACH_AT_SIGSEGV 0

#if DEBUG_GDB_ATTACH_AT_SIGSEGV
#   include <csignal>
#endif


#if DEBUG_GDB_ATTACH_AT_INIT || DEBUG_GDB_ATTACH_AT_SIGSEGV
#   include <unistd.h>
#endif

using namespace granary;

extern "C" {

#if DEBUG_GDB_ATTACH_AT_SIGSEGV
    /// Handle a segfault by trying to attach instrumentation to native code.
    static void handle_fault(int, siginfo_t *, void *) {
        detach();
        granary::printf("Process ID for attaching GDB: %d\n", (int) getpid());
        granary::printf("Press enter to continue.\n");
        char buff[1];
        read(0, buff, 1);
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

#if DEBUG_GDB_ATTACH_AT_INIT
    static unsigned buffer_pid(char *buff, uint64_t data) {
        if(!data) {
            *buff++ = '0';
            *buff++ = '\0';
            return 2;
        }

        uint64_t max_base(10);
        unsigned num_chars(1);
        for(; data / max_base; max_base *= 10) { }
        for(max_base /= 10; max_base; max_base /= 10) {
            const uint64_t digit(data / max_base);
            *buff++ = digit + '0';
            data -= digit * max_base;
            ++num_chars;
        }
        *buff++ = '\0';
        return num_chars;
    }
#endif

    __attribute__((constructor))
    static void granary_begin_program(void) {

#if DEBUG_GDB_ATTACH_AT_INIT
        char buff[20];
        write(2, "Process ID for attaching GDB: ", 31);
        write(2, buff, buffer_pid(buff, getpid()));
        write(2, "\nPress enter to continue.\n", 27);
        read(0, buff, 1);
#endif

        granary::init();
        granary::attach(granary::START_POLICY);
    }

    __attribute__((destructor, noinline, used))
    void granary_end_program(void) {
#ifdef CLIENT_report
        client::report();
#endif
#if CONFIG_DEBUG_PERF_COUNTS
        granary::perf::report();
#endif
    }

} /* extern C */


