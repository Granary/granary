/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <signal.h>

#include "granary/globals.h"
#include "granary/instruction.h"
#include "granary/basic_block.h"
#include "granary/state.h"
#include "granary/init.h"

void segfault_handler(int) {
    printf("Run `gdb attach %d`\n", getpid());
    for(;;) { /* loop until we manually attach gdb */ }
}


int main(int argc, const char **argv) throw() {
    printf("attaching segfault handler\n");
    signal(SIGSEGV, segfault_handler);

    (void) argc;
    (void) argv;

    granary::init();
    granary::run_tests();

    return 0;
}

