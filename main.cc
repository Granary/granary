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

void break_on_instruction(uint8_t *in) {
    (void) in;
}

uint8_t *buff;

namespace granary {
    typedef void (*adder_type)(int);

    void make_adder(int a) throw() {
        uint8_t *data = (uint8_t *) heap_alloc(nullptr, 100);
        uint8_t *pc = data;
        granary::instruction_list ls;
        auto restart = ls.label();

        ls.append(restart);
        ls.append(mov_imm_(reg::ret, int32_(a)));
        ls.append(add_(reg::ret, reg::arg1));
        ls.append(ret_());
        ls.append(jcc_short_(dynamorio::OP_jge, instr_(restart)));

        auto in = ls.first();
        for(unsigned i = 0; i < ls.length(); ++i) {
            pc = in->encode(pc);
            in = in.next();
        }

        break_on_instruction(data);

        heap_free(nullptr, data, 100);
    }
}

int main(int argc, const char **argv) throw() {

    granary::make_adder(0xBEEF);

    (void) argc;
    (void) argv;

    return 0;
}

