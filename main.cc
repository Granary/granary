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

int main(int argc, const char **argv) throw() {
    /*
    adder_type add1 = granary::make_adder(1);

    printf("1 + 2 = %d\n", add1(2));
    printf("1 + 5 = %d\n", add1(5));
    */

    granary::init();
    granary::decode((granary::app_pc) main);

    printf("hello world!\n");

    (void) argc;
    (void) argv;

    return 0;
}

