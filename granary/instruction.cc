/*
 * instruction.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstring>

#include "granary/instruction.h"

namespace granary {

    // used frequently in instruction functions
    dynamorio::dcontext_t *instruction::DCONTEXT = \
        dynamorio::get_thread_private_dcontext();

    /// constructor
    instruction::instruction(void) throw() {
        memset(this, 0, sizeof *this);
        dynamorio::instr_set_x86_mode(this, true);
    }

}

