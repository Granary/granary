/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>
#include <cstdlib>

#include "granary/instruction.h"

void break_on_instruction(decltype(granary::instruction_list().first()) in) {
    (void) in;
}

uint8_t *buff;

int main(int argc, const char **argv) throw() {
    granary::instruction_list ls;

    for(auto pc = main; ;) {
        granary::instruction in(granary::instruction::decode(&pc));
        if(in.is_cti()) {
            break;
        }

        ls.insert_before(ls.first(), in);
    }

    printf("insert before first\n");
    auto item = ls.first();
    for(unsigned len(0); len < ls.length(); ++len) {
        printf("%p\n", item->pc());
        item = item.next();
    }

    ls.clear_for_reuse();

    for(auto pc = main; ;) {
        granary::instruction in(granary::instruction::decode(&pc));
        if(in.is_cti()) {
            break;
        }

        ls.prepend(in);
    }

    printf("prepend\n");
    item = ls.first();
    for(unsigned len(0); len < ls.length(); ++len) {
        printf("%p\n", item->pc());
        item = item.next();
    }

    ls.clear();

    (void) argc;
    (void) argv;

    return 0;
}

