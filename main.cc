/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>
#include <cstdlib>

#include "granary/instruction.h"
#include "granary/basic_block.h"

void break_on_instruction(uint8_t *in, granary::basic_block *bb) {
    (void) in;
    (void) bb;
}

uint8_t *buff;

struct foo {
    char bar[13];
};

namespace granary {
    typedef void (*adder_type)(int);

    void make_adder(int a) throw() {
        uint8_t *data = (uint8_t *) heap_alloc(nullptr, 100);
        uint8_t *next_data = data;

        granary::instruction_list ls;
        auto restart = ls.label();

        ls.append(restart);
        ls.append(mov_imm_(reg::ret, int32_(a)));
        ls.append(add_(*reg::ret, reg::arg1));
        ls.append(lea_(reg::ret, reg::arg1 + reg::arg2 * 2 + sizeof(foo)));
        ls.append(ret_());
        ls.append(jcc_(dynamorio::OP_jge, instr_(restart)));

        basic_block bb(basic_block::emit(BB_TRANSLATED_FRAGMENT, ls, nullptr, &next_data));
        break_on_instruction(data, &bb);

        printf("bb size = %u\n", basic_block::size(ls));
        printf("emitted bb size = %u\n", bb.size());

        heap_free(nullptr, data, 100);
    }
}

int main(int argc, const char **argv) throw() {

    granary::make_adder(0xBEEF);

    (void) argc;
    (void) argv;

    return 0;
}

