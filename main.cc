/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>

#include "granary/instruction.h"

void break_on_instruction(granary::instruction *in, uint8_t *out) {
    (void) in;
    (void) out;
}

int main(int argc, const char **argv) throw() {
    (void) argc;
    (void) argv;

    uint8_t instr_buffer[100] = {0x90};

    auto pc = main;
    granary::instruction in(granary::instruction::decode(&pc));
    in.encode(instr_buffer);
    break_on_instruction(&in, instr_buffer);
    return 0;
}

