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

extern "C" {

    __attribute__((constructor))
    static void granary_begin_program(void) {

        printf("Process ID for attaching GDB: %d\n", (int) getpid());
        printf("Press enter to continue.\n");
        getchar();

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


