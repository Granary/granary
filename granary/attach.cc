/*
 * attach.cc
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/attach.h"
#include "granary/globals.h"
#include "granary/code_cache.h"
#include "granary/basic_block.h"
#include "clients/instrument.h"

namespace granary {

    /// Called (through a tail call) by `granary::attach`, which is defined in
    /// `granary/x86/attach.asm`.
    void do_attach(instrumentation_policy policy, app_pc *ret_addr) throw() {
        granary::basic_block bb(granary::code_cache::find(*ret_addr, policy));
        *ret_addr = bb.cache_pc_start;
    }

}

