/*
 * dlmain.cc
 *
 *  Created on: 2013-02-13
 *      Author: pag
 *     Version: $Id$
 */

#include <cstdio>

#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif
#include <dlfcn.h>

#include "granary/globals.h"
#include "granary/policy.h"
#include "granary/code_cache.h"
#include "granary/basic_block.h"
#include "granary/attach.h"
#include "clients/instrument.h"

#include <unistd.h>
#include <signal.h>

void granary_signal_handler(int) {
    printf("Run `sudo gdb attach %d`\n", getpid());
    for(;;) { /* loop until we manually attach gdb */ }
}

extern "C" {

//#ifdef __APPLE__
#if 1
    __attribute__((noinline, constructor, optimize("O0")))
    static void begin_program(void) {
        auto policy(GRANARY_INIT_POLICY);
        signal(SIGSEGV, granary_signal_handler);
        signal(SIGILL, granary_signal_handler);
        granary::init();
        granary::attach(granary::policy_for<decltype(policy)>());
    }
#else

    typedef int (__libc_start_main_type)(
        int (*)(int, char **, char **),
        int,
        char **,
        void (*)(void),
        void (*)(void),
        void (*)(void),
        void *
    );

    int __libc_start_main(
        int (*main)(int, char **, char **),
        int argc,
        char **ubp_av,
        void (*init)(void),
        void (*fini)(void),
        void (*rtld_fini)(void),
        void *stack_end
    ) {
        auto policy(GRANARY_INIT_POLICY);
        //void *handle(dlopen("/lib/libc.so.6", RTLD_LAZY | RTLD_GLOBAL));
        granary::app_pc start(reinterpret_cast<granary::app_pc>(dlsym(
            RTLD_NEXT, "__libc_start_main")));

        //printf("their __libc_start_main = %p\n", start);

        signal(SIGSEGV, granary_signal_handler);
        signal(SIGILL, granary_signal_handler);

        granary::init();

        granary::basic_block bb(granary::code_cache::find(
            start, granary::policy_for<decltype(policy)>()));

        //granary::printf("in __libc_start_main\n");

        //return ((__libc_start_main_type *) start)(
        //    main, argc, ubp_av, init, fini, rtld_fini, stack_end);

        return bb.call<
            int,
            int (*)(int, char **, char **),
            int,
            char **,
            void (*)(void),
            void (*)(void),
            void (*)(void),
            void *
        >(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
    }
#endif
} /* extern C */


