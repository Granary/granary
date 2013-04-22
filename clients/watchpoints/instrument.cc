/*
 * instrument.cc
 *
 *  Created on: 2013-04-20
 *      Author: pag
 */


#include "clients/watchpoints/instrument.h"

using namespace granary;

namespace clients { namespace wp {

    /// Find memory operands that might need to be checked for watchpoints.
    /// If one is found, then num_ops is incremented, and the operand
    /// reference is stored in the passed array.
    void find_memory_operand(
        const operand_ref &op,
        operand_ref *&ops,
        unsigned &num_ops
    ) throw() {

        if(dynamorio::BASE_DISP_kind != op->kind) {
            return;
        }

        register_manager rm;
        rm.kill(*op);

        // make sure we've got at least one general purpose register
        dynamorio::reg_id_t reg(rm.get_zombie());
        if(!reg) {
            return;
        }

        do {
            if(dynamorio::DR_REG_RSP == reg
            IF_WP_IGNORE_FRAME_POINTER( || dynamorio::DR_REG_RBP == reg )) {
                return;
            }
            reg = rm.get_zombie();

        } while(reg);

        ops[num_ops] = const_cast<operand_ref>(op);
        ++num_ops;
    }

}}
