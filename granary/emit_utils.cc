/*
 * emit_utils.cc
 *
 *  Created on: 2013-02-06
 *      Author: pag
 */

#include "granary/emit_utils.h"
#include "granary/hash_table.h"
#include "granary/list.h"
#include "granary/state.h"

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


    /// Add a call to a known function after a particular instruction in the
    /// instruction stream. If the location of the function to be called is
    /// too far away then a specified register is clobbered with the target pc,
    /// and an indirect jump is performed. If no clobber register is available
    /// then an indirect jump slot is allocated and used.
    instruction_list_handle insert_cti_after(
        instruction_list &ls,
        instruction_list_handle in,
        app_pc target,
        bool has_clobber_reg,
        operand clobber_reg,
        cti_kind kind
    ) throw() {

        instruction (*cti_)(dynamorio::opnd_t);
        instruction (*cti_ind_)(dynamorio::opnd_t);

        if(CTI_CALL == kind) {
            cti_ = call_;
            cti_ind_ = call_ind_;
        } else {
            cti_ = jmp_;
            cti_ind_ = jmp_ind_;
        }

        // start with a staged allocation that should be close enough
        app_pc staged_loc(
            global_state::FRAGMENT_ALLOCATOR->allocate_staged<uint8_t>());

        // add in the indirect call to the find on CPU function
        if(is_far_away(staged_loc, target)) {

            if(has_clobber_reg) {
                in = ls.insert_after(in,
                    mov_imm_(clobber_reg,
                        int64_(unsafe_cast<int64_t>(target))));
                in = ls.insert_after(in, cti_ind_(clobber_reg));
            } else {
                app_pc *slot(global_state::FRAGMENT_ALLOCATOR->allocate<app_pc>());
                *slot = target;
                in = ls.insert_after(in, cti_ind_(absmem_(slot, dynamorio::OPSZ_8)));
            }

        // add in a direct, pc relative call.
        } else {
            in = ls.insert_after(in, cti_(pc_(target)));
        }

        return in;
    }


    /// Add the instructions to save the flags onto the top of the stack.
    instruction_list_handle insert_save_flags_after(
        instruction_list &ls,
        instruction_list_handle in
    ) throw() {
#if GRANARY_IN_KERNEL
        in = ls.insert_after(in, pushf_());
        in = ls.insert_after(in, cli_());
#elif CONFIG_IBL_SAVE_ALL_FLAGS
        in = ls.insert_after(in, pushf_());
#else
        in = ls.insert_after(in, push_(reg::rax));
        in = ls.insert_after(in, lahf_());
        in = ls.insert_after(in, xchg_(*reg::rsp, reg::rax));
#endif
        return in;
    }


    /// Add the instructions to restore the flags from the top of the stack.
    instruction_list_handle insert_restore_flags_after(
        instruction_list &ls,
        instruction_list_handle in
    ) throw() {
#if GRANARY_IN_KERNEL
        in = ls.insert_after(in, popf_());
#elif CONFIG_IBL_SAVE_ALL_FLAGS
        in = ls.insert_after(in, popf_());
#else
        in = ls.insert_after(in, xchg_(*reg::rsp, reg::rax));
        in = ls.insert_after(in, sahf_());
        in = ls.insert_after(in, pop_(reg::rax));
#endif
        return in;
    }


    /// Add instructions to align the stack (to the top of the stack) to a 16
    /// byte boundary.
    instruction_list_handle insert_align_stack_after(
        instruction_list &ls,
        instruction_list_handle in
    ) throw() {
        in = ls.insert_after(in, push_(reg::rsp));
        in = ls.insert_after(in, push_(reg::rsp[0]));
        in = ls.insert_after(in, and_(reg::rsp, int8_(-16)));
        return in;
    }


    /// Add instructions to restore the stack's previous alignment.
    instruction_list_handle insert_restore_old_stack_alignment_after(
        instruction_list &ls,
        instruction_list_handle in
    ) throw() {
        in = ls.insert_after(in, mov_ld_(reg::rsp, reg::rsp[8]));
        return in;
    }
}

