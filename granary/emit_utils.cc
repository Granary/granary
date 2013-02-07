/*
 * emit_utils.cc
 *
 *  Created on: 2013-02-06
 *      Author: pag
 */

#include "granary/emit_utils.h"
#include "granary/hash_table.h"
#include "granary/list.h"

namespace granary {


    /// Traverse through the instruction control-flow graph and look for used
    /// registers.
    register_manager find_used_regs_in_func(app_pc func) throw() {
        register_manager used_regs;
        list<app_pc> process_bbs;
        list<app_pc>::handle_type next;
        hash_set<app_pc> seen;

        used_regs.revive_all();
        process_bbs.append(func);

        // traverse instructions and find all used registers.
        for(unsigned i(0); i < process_bbs.length(); ++i) {

            if(!next.is_valid()) {
                next = process_bbs.first();
            } else {
                next = next.next();
            }

            app_pc bb(*next);
            if(seen.contains(bb)) {
                break;
            }

            seen.add(bb);

            for(; bb; ) {
                instruction in;

                in = instruction::decode(&bb);
                used_regs.kill(in);

                // done processing this basic block
                if(dynamorio::OP_ret == in.op_code()) {
                    break;
                }

                // this is a cti; add the destination instruction as
                if(in.is_cti()) {

                    operand target(in.cti_target());
                    if(!dynamorio::opnd_is_pc(target)) {
                        used_regs.kill_all();
                        return used_regs;
                    }

                    process_bbs.append(dynamorio::opnd_get_pc(target));
                }
            }
        }

        return used_regs;
    }


    /// Save all dead registers within a particular register manager. This is
    /// useful for saving/restoring only those registers used by a function.
    ///
    /// Given an instruction `in`, this function adds all instructions to
    /// save and restore the undead registers of `regs` after `in`, and returns
    /// the handle of the last pushed register, so that further instructions
    /// can be placed in-between the saving/restoring instructions.
    instruction_list_handle save_and_restore_registers(
        register_manager &regs,
        instruction_list &ls,
        instruction_list_handle in
    ) throw() {
        for(;;) {
            dynamorio::reg_id_t reg_id(regs.get_zombie());
            if(!reg_id) {
                break;
            }

            operand reg(reg_id);

            in = ls.insert_after(in, push_(reg));
            ls.insert_after(in, pop_(reg));
        }

        return in;
    }


    /// Save all dead xmm registers within a particular register manager. This
    /// is analogous to `save_and_restore_registers`.
    instruction_list_handle save_and_restore_xmm_registers(
        register_manager &regs,
        instruction_list &ls,
        instruction_list_handle in,
        xmm_save_constraint is_aligned
    ) throw() {
        instruction (*mov_xmm)(dynamorio::opnd_t, dynamorio::opnd_t) = (
            XMM_SAVE_ALIGNED == is_aligned ? movaps_ : movups_);

        in = ls.insert_after(in, label_());
        instruction_list_handle window_top(in);
        instruction_list_handle window_bottom(in);

        int disp(0);
        for(; ; disp += 16) {
            dynamorio::reg_id_t reg_id(regs.get_xmm_zombie());
            if(!reg_id) {
                break;
            }

            operand reg_xmm(reg_id);

            window_top = ls.insert_before(window_top,
                mov_xmm(reg::rsp[disp], reg_xmm));

            window_bottom = ls.insert_after(window_bottom,
                mov_xmm(reg_xmm, reg::rsp[disp]));
        }

        if(disp) {
            ls.insert_before(window_top, lea_(reg::rsp, reg::rsp[-disp]));
            ls.insert_after(window_bottom, lea_(reg::rsp, reg::rsp[disp]));
        }

        return in;
    }
}

