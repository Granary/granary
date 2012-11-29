/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>
#include <cstdlib>

#include "granary/globals.h"
#include "granary/instruction.h"
#include "granary/basic_block.h"
#include "granary/state.h"
#include "granary/init.h"


int main(int argc, const char **argv) throw() {

    (void) argc;
    (void) argv;

    granary::run_tests();

    return 0;
}

