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

void break_on_instruction(granary::basic_block *bb) {
    //(void) in;
    (void) bb;
}

extern "C" {
    void break_before_fault(void) {

    }
}

typedef int (*adder_type)(int);

int main(int argc, const char **argv) throw();

namespace granary {
#if 0
    adder_type make_adder(int a) throw() {

        cpu_state_handle cpu;
        uint8_t *data(cpu->fragment_allocator.allocate_array<uint8_t>(100));
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
#endif


    void decode(app_pc func) {
        cpu_state_handle cpu;
        thread_state_handle thread;
        basic_block bb(basic_block::translate(cpu, thread, &func));

        break_on_instruction(&bb);
    }
}

void break_on_compare(granary::instruction *manual, granary::instruction *decoded) {
    (void) manual;
    (void) decoded;
}

int main(int argc, const char **argv) throw() {

    for(int i(0); i < argc; ++i) {
        printf("%s\n", argv[i]);
    }

    granary::init();
    granary::decode((granary::app_pc) main);

    printf("hello world!\n");

    unsigned char buff[100];
foo:
    ASM("mov 0x3e(%rcx), %ax;");
    granary::app_pc pc((granary::app_pc) &&foo);
    granary::instruction manual(granary::mov_ld_(granary::reg::ax, granary::reg::rcx[0x3e]));
    granary::instruction decoded(granary::instruction::decode(&pc));

    break_on_compare(&manual, &decoded);

    manual.encode(buff);

    (void) argc;
    (void) argv;

    return 0;
}

