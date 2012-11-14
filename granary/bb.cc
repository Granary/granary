/*
 * bb.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/utils.h"
#include "granary/bb.h"

namespace granary {

    /// Return the meta information for the current basic block, given some
    /// pointer into the instructions of the basic block.
    basic_block::basic_block(app_pc current_pc_) throw()
        : current_pc(current_pc_)
    {

        // round up to the next 64-bit aligned
        uint64_t addr(reinterpret_cast<uint64_t>(current_pc_));
        addr += (addr % sizeof(uint64_t));

        uint32_t *ints(reinterpret_cast<uint8_t *>(addr));
        for(; (BB_MAGIC_MASK & *ints) != BB_MAGIC; ints += 2) {
            // keep scanning
        }

        // we've found the basic block info
        info = unsafe_cast<basic_block_info *>(ints);
    }

}


