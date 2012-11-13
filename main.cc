/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>
#include <cstdlib>

#include "granary/instruction.h"
#include "granary/gen/instruction.h"

void break_on_instruction(decltype(granary::instruction_list().first()) in) {
    (void) in;
}

uint8_t *buff;

namespace granary {
    typedef void (*adder_type)(int);

    void make_adder(int a) throw() {
        granary::instruction_list ls;
        ls.append(mov_ld_(reg::rax, int32_(a)));
        ls.append(add_(reg::rax, reg::arg1));
        ls.append(ret_());
    }
}

int main(int argc, const char **argv) throw() {

    granary::make_adder(1);

    (void) argc;
    (void) argv;

    return 0;
}

