/*
 * dlmain.cc
 *
 *  Created on: 2013-02-13
 *      Author: pag
 *     Version: $Id$
 */
#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif
#include <dlfcn.h>

#include "granary/globals.h"
#include "granary/policy.h"
#include "granary/code_cache.h"
#include "granary/basic_block.h"
#include "granary/emit_utils.h"

#include "clients/instrument.h"

#include <unistd.h>
#include <signal.h>

void granary_signal_handler(int) {
    printf("Run `sudo gdb attach %d`\n", getpid());
    for(;;) { /* loop until we manually attach gdb */ }
}

extern "C" {

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
        //void *handle(dlopen("/lib/libc.so.6", RTLD_LAZY | RTLD_GLOBAL));
        granary::app_pc start(reinterpret_cast<granary::app_pc>(dlsym(
            RTLD_NEXT, "__libc_start_main")));

        printf("their __libc_start_main = %p\n", start);

        signal(SIGSEGV, granary_signal_handler);
        signal(SIGILL, granary_signal_handler);

        granary::init();

        granary::basic_block bb(granary::code_cache::find(
            start, GRANARY_INIT_POLICY));

        granary::printf("in __libc_start_main\n");

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

} /* extern C */


