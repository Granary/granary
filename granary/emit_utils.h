/*
 * emit_utils.h
 *
 *  Created on: 2013-02-06
 *      Author: pag
 */

#ifndef EMIT_UTILS_H_
#define EMIT_UTILS_H_

#include "granary/instruction.h"
#include "granary/register.h"

namespace granary {


    /// Returns true iff the two values are very far (>= 4GB) away.
    template <typename P1, typename P2>
    inline bool is_far_away(P1 p1, P2 p2) throw() {

        enum {
            _4GB = 4294967296LL
        };

        int64_t diff(
            reinterpret_cast<uint64_t>(p1) - reinterpret_cast<uint64_t>(p2));
        if(diff < 0) {
            diff = -diff;
        }
        return diff >= _4GB;
    }


    /// Returns true iff some address can sign-extend from 32 bits to 64 bits.
    template <typename P>
    inline bool addr_is_32bit(P addr_) throw() {
        const uint64_t addr(reinterpret_cast<uint64_t>(addr_));
        const uint64_t addr_sign(addr >> 31);
        const uint32_t addr_high(addr_sign >> 1);
        if(1 & addr_sign) {
            return ~0U == addr_high;
        } else {
            return 0U == addr_high;
        }
    }


    /// Traverse through the instruction control-flow graph and look for used
    /// registers.
    register_manager find_used_regs_in_func(app_pc func) throw();


    /// Save all dead 64-bit registers within a particular register manager.
    /// This is useful for saving/restoring only those registers used by a
    /// function.
    instruction_list_handle save_and_restore_registers(
        register_manager &regs,
        instruction_list &ls,
        instruction_list_handle in
    ) throw();


    enum xmm_save_constraint {
        XMM_SAVE_ALIGNED,
        XMM_SAVE_UNALIGNED
    };


    /// Save all dead xmm registers within a particular register manager.
    /// This is useful for saving/restoring only those registers used by a
    /// function.
    instruction_list_handle save_and_restore_xmm_registers(
        register_manager &regs,
        instruction_list &ls,
        instruction_list_handle in,
        xmm_save_constraint
    ) throw();
}


#endif /* EMIT_UTILS_H_ */
