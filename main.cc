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

void break_on_instruction(uint8_t *in, granary::basic_block *bb) {
    (void) in;
    (void) bb;
}

typedef int (*adder_type)(int);

namespace granary {

    adder_type make_adder(int a) throw() {
        uint8_t *data = (uint8_t *) allocate_executable((cpu_info *) nullptr, 100);
        uint8_t *next_data = data;

        granary::instruction_list ls;
        ls.append(mov_imm_(reg::ret, int32_(a)));
        ls.append(add_(reg::ret, reg::arg1));
        ls.append(ret_());

        basic_block bb(basic_block::emit(
            basic_block_kind::BB_TRANSLATED_FRAGMENT, ls, nullptr, &next_data));

        break_on_instruction(data, &bb);

        return (adder_type) data;
    }
}

int main(int argc, const char **argv) throw() {

    adder_type add1 = granary::make_adder(1);

    printf("1 + 2 = %d\n", add1(2));
    printf("1 + 5 = %d\n", add1(5));

    (void) argc;
    (void) argv;

    return 0;
}

