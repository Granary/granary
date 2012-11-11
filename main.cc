/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>
#include <cstdlib>

#include "granary/instruction.h"

void break_on_instruction(uint8_t *out) {
    (void) out;
}

uint8_t *buff;

int main(int argc, const char **argv) throw() {
    granary::instruction_list ls;

    for(auto pc = main; ;) {
        granary::instruction in(granary::instruction::decode(&pc));
        if(in.is_cti()) {
            break;
        }

        ls.append(in);
    }

    (void) argc;
    (void) argv;

    return 0;
}

