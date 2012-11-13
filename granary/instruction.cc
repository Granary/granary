/*
 * instruction.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstring>

#include "granary/instruction.h"

namespace granary {

#define MAKE_REG(name, upper_name) dynamorio::opnd_t name = dynamorio::opnd_create_reg(dynamorio::DR_ ## upper_name);
    namespace reg {
#include "inc/registers.h"
    }
#undef MAKE_REG

    // used frequently in instruction functions
    dynamorio::dcontext_t *instruction::DCONTEXT = \
        dynamorio::get_thread_private_dcontext();

    /// constructor
    instruction::instruction(void) throw() {
        memset(this, 0, sizeof *this);
        dynamorio::instr_set_x86_mode(this, true);
    }

}

