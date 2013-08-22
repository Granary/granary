/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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
    ///
    /// Note: This will recursively follow through direct function calls.
    register_manager find_used_regs_in_func(
        app_pc func,
        instruction_traversal_constraint constraint
    ) throw() {
        register_manager used_regs;
        list<app_pc> process_bbs;
        list<app_pc>::handle_type next;
        hash_set<app_pc> seen;
        instruction in;

        used_regs.revive_all();
        process_bbs.append(func);

        // Traverse instructions and find all used registers.
        while(process_bbs.length()) {

            next = process_bbs.first();
            app_pc bb(*next);
            process_bbs.remove(next);

            if(seen.contains(bb)) {
                continue;
            }

            seen.add(bb);
            for(; bb; ) {

                in.decode_update(&bb);
                used_regs.kill_dests(in);

                // Done processing this basic block.
                if(dynamorio::OP_ret == in.op_code()
                || dynamorio::OP_iret == in.op_code()
                || dynamorio::OP_sysret == in.op_code()) {
                    break;
                }

                // Try to follow CTIs.
                if(in.is_cti()) {
                    const bool is_call(in.is_call());

                    // TODO: Too conservative; fall back on ABI.
                    if(is_call && USED_REGS_IGNORE_CALLS == constraint) {
                        continue;
                    }

                    operand target(in.cti_target());
                    if(!dynamorio::opnd_is_pc(target)) {
                        used_regs.kill_all();
                        return used_regs;
                    }

                    app_pc target_pc(dynamorio::opnd_get_pc(target));
                    if(!seen.contains(bb)) {
                        process_bbs.append(target_pc);
                    }

                    if(in.is_unconditional_cti()) {

                        // Done processing this basic block.
                        if(!is_call) {
                            break;
                        }

                    // Done processing this basic block.
                    } else {
                        if(!seen.contains(bb)) {
                            process_bbs.append(bb);
                        }
                        break;
                    }
                }
            }
        }

        return used_regs;
    }


    /// Push all registers that are dead in a register manager.
    instruction save_registers(
        register_manager regs,
        instruction_list &ls,
        instruction in
    ) throw() {
        for(;;) {
            dynamorio::reg_id_t reg_id(regs.get_zombie());
            if(!reg_id) {
                break;
            }

            in = ls.insert_after(in, push_(operand(reg_id)));
        }

        return in;
    }


    /// Pop all registers that are dead in a register manager.
    instruction restore_registers(
        register_manager regs,
        instruction_list &ls,
        instruction in
    ) throw() {
        instruction last(ls.insert_after(in, label_()));
        in = last;
        for(;;) {
            dynamorio::reg_id_t reg_id(regs.get_zombie());
            if(!reg_id) {
                break;
            }

            in = ls.insert_before(in, pop_(operand(reg_id)));
        }

        return last;
    }


    /// Save all dead registers within a particular register manager. This is
    /// useful for saving/restoring only those registers used by a function.
    ///
    /// Given an instruction `in`, this function adds all instructions to
    /// save and restore the undead registers of `regs` after `in`, and returns
    /// the handle of the last pushed register, so that further instructions
    /// can be placed in-between the saving/restoring instructions.
    ///
    /// Note: This
    instruction save_and_restore_registers(
        register_manager regs,
        instruction_list &ls,
        instruction in
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
    instruction save_and_restore_xmm_registers(
        register_manager regs,
        instruction_list &ls,
        instruction in,
        xmm_save_constraint is_aligned
    ) throw() {
        instruction (*mov_xmm)(dynamorio::opnd_t, dynamorio::opnd_t) = (
            XMM_SAVE_ALIGNED == is_aligned ? movaps_ : movups_);

        in = ls.insert_after(in, label_());
        instruction window_top(in);
        instruction window_bottom(in);

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
    instruction insert_cti_after(
        instruction_list &ls,
        instruction in,
        app_pc target,
        cti_register_steal_constraint steal_constraint,
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

        // Start with a staged allocation that should be close enough.
        app_pc staged_loc(
            global_state::FRAGMENT_ALLOCATOR->allocate_staged<uint8_t>());

        // Add in the indirect call to the find on CPU function.
        if(is_far_away(staged_loc, target)) {

            if(CTI_STEAL_REGISTER == steal_constraint) {
                ASSERT(dynamorio::REG_kind == clobber_reg.kind);
                ASSERT(dynamorio::DR_REG_NULL != clobber_reg.value.reg);
                ASSERT(
                    register_manager::scale(clobber_reg.value.reg, REG_64)
                 == clobber_reg.value.reg);

                in = ls.insert_after(in,
                    mov_imm_(clobber_reg,
                        int64_(unsafe_cast<int64_t>(target))));
                in = ls.insert_after(in, cti_ind_(clobber_reg));
            } else {
                app_pc *slot(global_state::FRAGMENT_ALLOCATOR->\
                    allocate<app_pc>());
                *slot = target;
                in = ls.insert_after(in,
                    cti_ind_(absmem_(slot, dynamorio::OPSZ_8)));
            }

        // Add in a direct, pc relative call.
        } else {
            in = ls.insert_after(in, cti_(pc_(target)));
        }

        return in;
    }


    /// Add the instructions to save the flags onto the top of the stack.
    instruction insert_save_flags_after(
        instruction_list &ls,
        instruction in,
        flag_save_constraint constraint
    ) throw() {
        (void) constraint;
#if GRANARY_IN_KERNEL
        in = ls.insert_after(in, pushf_());
        in = ls.insert_after(in, cli_());
#elif CONFIG_IBL_SAVE_ALL_FLAGS
        in = ls.insert_after(in, pushf_());
#else
        if(REG_AH_IS_LIVE == constraint) {
            in = ls.insert_after(in, push_(reg::rax));
            in = ls.insert_after(in, push_(reg::rax));
            in = ls.insert_after(in, lahf_());
            in = ls.insert_after(in, mov_st_(reg::rsp[8], reg::rax));
            in = ls.insert_after(in, pop_(reg::rax));
        } else {
            in = ls.insert_after(in, lahf_());
            in = ls.insert_after(in, push_(reg::rax));
        }
#endif
        return in;
    }


    /// Add the instructions to restore the flags from the top of the stack.
    instruction insert_restore_flags_after(
        instruction_list &ls,
        instruction in,
        flag_save_constraint constraint
    ) throw() {
        (void) constraint;
#if GRANARY_IN_KERNEL
        in = ls.insert_after(in, popf_());
#elif CONFIG_IBL_SAVE_ALL_FLAGS
        in = ls.insert_after(in, popf_());
#else
        if(REG_AH_IS_LIVE == constraint) {
            in = ls.insert_after(in, push_(reg::rax));
            in = ls.insert_after(in, mov_ld_(reg::rax, reg::rsp[8]));
            in = ls.insert_after(in, sahf_());
            in = ls.insert_after(in, pop_(reg::rax));
            in = ls.insert_after(in, lea_(reg::rsp, reg::rsp[8]));
        } else {
            in = ls.insert_after(in, pop_(reg::rax));
            in = ls.insert_after(in, sahf_());
        }
#endif
        return in;
    }


    /// Add instructions to align the stack (to the top of the stack) to a 16
    /// byte boundary.
    instruction insert_align_stack_after(
        instruction_list &ls,
        instruction in
    ) throw() {
        in = ls.insert_after(in, push_(reg::rsp));
        in = ls.insert_after(in, push_(reg::rsp[0]));
        in = ls.insert_after(in, and_(reg::rsp, int8_(-16)));
        return in;
    }


    /// Add instructions to restore the stack's previous alignment.
    instruction insert_restore_old_stack_alignment_after(
        instruction_list &ls,
        instruction in
    ) throw() {
        in = ls.insert_after(in, mov_ld_(reg::rsp, reg::rsp[8]));
        return in;
    }


    /// Argument registers.
    operand ARGUMENT_REGISTERS[5];


    STATIC_INITIALISE_ID(argument_registers, {
        ARGUMENT_REGISTERS[0] = reg::arg1;
        ARGUMENT_REGISTERS[1] = reg::arg2;
        ARGUMENT_REGISTERS[2] = reg::arg3;
        ARGUMENT_REGISTERS[3] = reg::arg4;
        ARGUMENT_REGISTERS[4] = reg::arg5;
    });

    namespace detail {

        /// Generate a clean way of exiting the code cache through a CALL that
        /// will correctly save/restore registers and align the stack.
        ///
        /// Note: This will assume that whoever invokes this exit point is
        ///       responsible for the argument registers.
        app_pc generate_clean_callable_address(
            app_pc func_pc,
            unsigned num_args,
            register_exit_constaint constraint
        ) throw() {

            // How should we look for used registers?
            instruction_traversal_constraint used_reg_constraint;
            if(EXIT_REGS_ABI_COMPATIBLE == constraint) {
                used_reg_constraint = USED_REGS_IGNORE_CALLS;
            } else {
                used_reg_constraint = USED_REGS_VISIT_ALL_INSTRUCTIONS;
            }

            register_manager dead_regs(find_used_regs_in_func(
                func_pc, used_reg_constraint));

            // Assume that the caller does not care about saving argument
            // registers which it likely clobbered (and had to save on its
            // own).
            for(unsigned i(num_args); i--; ) {
                dead_regs.revive(ARGUMENT_REGISTERS[i]);
            }

            // Don't bother saving callee-saved registers if the callee is
            // following the ABI.
            if(EXIT_REGS_ABI_COMPATIBLE == constraint) {

                // Callee-saved.
                dead_regs.revive(dynamorio::DR_REG_RBX);
                dead_regs.revive(dynamorio::DR_REG_RBP);
                dead_regs.revive(dynamorio::DR_REG_R12);
                dead_regs.revive(dynamorio::DR_REG_R13);
                dead_regs.revive(dynamorio::DR_REG_R14);
                dead_regs.revive(dynamorio::DR_REG_R15);

                // Some of the scratch registers.
                dead_regs.kill(dynamorio::DR_REG_R8);
                dead_regs.kill(dynamorio::DR_REG_R9);
                dead_regs.kill(dynamorio::DR_REG_R10);
                dead_regs.kill(dynamorio::DR_REG_R11);
            }

            instruction_list ls;
            instruction in(ls.append(pushf_()));

            IF_KERNEL( in = ls.append(cli_()); )

            in = save_and_restore_registers(dead_regs, ls, in);

            if(EXIT_REGS_ABI_COMPATIBLE == constraint) {
                in = insert_align_stack_after(ls, in);
            }

#if !GRANARY_IN_KERNEL
            instruction after_xmm(ls.insert_after(in, label_()));
            in = save_and_restore_xmm_registers(
                dead_regs, ls, in,
                (EXIT_REGS_ABI_COMPATIBLE == constraint)
                    ? XMM_SAVE_ALIGNED
                    : XMM_SAVE_UNALIGNED
            );
#endif

            in = insert_cti_after(
                ls, in,
                func_pc,
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL);
            in.set_mangled();

            if(EXIT_REGS_ABI_COMPATIBLE == constraint) {
                in = insert_restore_old_stack_alignment_after(
                    ls, IF_USER_ELSE(after_xmm, in));
            }

            ls.append(popf_());
            ls.append(ret_());

            app_pc entry_point_pc(global_state::FRAGMENT_ALLOCATOR->\
                allocate_array<uint8_t>(ls.encoded_size()));

            ls.encode(entry_point_pc);

            return entry_point_pc;
        }
    }
}

