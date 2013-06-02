/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-04-20
 *      Author: pag
 */


#include "clients/watchpoints/instrument.h"

using namespace granary;

namespace client { namespace wp {


    /// Tracks the next counter index to be allocated.
    static std::atomic<uintptr_t> NEXT_COUNTER_INDEX = ATOMIC_VAR_INIT(0);


    /// Return the next counter index.
    uintptr_t next_counter_index(void) throw() {
        return NEXT_COUNTER_INDEX.fetch_add(1);
    }


    /// Find memory operands that might need to be checked for watchpoints.
    /// If one is found, then num_ops is incremented, and the operand
    /// reference is stored in the passed array.
    void find_memory_operand(
        const operand_ref &op,
        watchpoint_tracker &tracker
    ) throw() {

        // Ignore non-address kinds.
        const operand ref_to_op(*op);
        if(dynamorio::BASE_DISP_kind != ref_to_op.kind) {
            return;
        }

        const dynamorio::reg_id_t base_reg(op->value.base_disp.base_reg);
        const dynamorio::reg_id_t index_reg(op->value.base_disp.index_reg);
        const dynamorio::reg_id_t base_reg_64(
            register_manager::scale(base_reg, REG_64));
        const dynamorio::reg_id_t index_reg_64(
            register_manager::scale(index_reg, REG_64));

        // Guard against RSP up here as it will never be returned by get_zombie
        // (as a preventive measure for preventing the stack pointer from being
        // clobbered). Also, treat all stack addresses as unwatched.
        if(dynamorio::DR_REG_RSP == base_reg_64
        || dynamorio::DR_REG_RSP == index_reg_64) {
            return;
        }

        // Make sure we've got at least one 64-bit register.
        if(base_reg) {
            if(base_reg != base_reg_64) {
                return;
            }
        } else {
            if(index_reg != index_reg_64) {
                return;
            }
        }

        register_manager rm;
        rm.kill(ref_to_op);

        // Make sure we've got at least one general purpose register
        dynamorio::reg_id_t regs[2];
        regs[0] = rm.get_zombie();
        regs[1] = rm.get_zombie();
        if(!regs[0]) {
            return;
        }

#if WP_IGNORE_FRAME_POINTER
        // If we consider RBP as a frame pointer then prevent it from being
        // clobbered as well. Also, treat all stack addresses as unwatched.
        if(dynamorio::DR_REG_RBP == regs[0]
        || dynamorio::DR_REG_RBP == regs[1]) {

            // We need to be careful about ignoring the frame pointer in the
            // case of leaf functions. We'll look to see if the base pointer
            // is an index register that is being multiplied by a non-1
            // constant. This doesn't get all cases, obviously.

            if(dynamorio::DR_REG_RBP != op->value.base_disp.index_reg
            || 1 == op->value.base_disp.scale) {
                return;
            }
        }
#endif /* WP_IGNORE_FRAME_POINTER */

        // Special case: XLAT uses an un-encodeable `(RBX, AL)` operand.
        if(dynamorio::OP_xlat == tracker.in.op_code()) {
            tracker.can_replace[tracker.num_ops] = false;

        // Two registers are used in the base/disp (base & index), thus this is
        // not an "implicit" operand to an instruction, and so we can replace
        // the operand.
        } else if(regs[1]) {
            tracker.can_replace[tracker.num_ops] = true;

        // This is a register that is not typically an implicitly used register
        // (one of the specialised general purpose regs). Note: this depends on
        // the ordering of regs in the enum. Specifically, this looks for
        // R8-R15 as being okay to alter.
        } else if(dynamorio::DR_REG_RDI < regs[0]) {
            tracker.can_replace[tracker.num_ops] = true;

        // If either of the scale or index are non-zero, then it's not an
        // implicit operand. The exception to this is base/disp using RSP as the
        // base reg (e.g. CALL, RET, PUSH, etc.); these operands are all avoided
        // using a different check.
        } else if(0 != op->value.base_disp.disp
               || 0 != op->value.base_disp.scale) {
            tracker.can_replace[tracker.num_ops] = true;

        // We need to leave this operand alone; which means we are required to
        // save the original regs, then modify them in place.
        } else {
            switch(tracker.in.op_code()) {
            // Optimisation for simple instructions that are typically used and
            // are known to have replaceable operands.
            case dynamorio::OP_mov_ld:
            case dynamorio::OP_mov_st:
            case dynamorio::OP_movzx:
            case dynamorio::OP_movsx:
            case dynamorio::OP_add:
            case dynamorio::OP_sub:
            case dynamorio::OP_inc:
            case dynamorio::OP_dec:
            case dynamorio::OP_and:
            case dynamorio::OP_or:
            case dynamorio::OP_xor:
            case dynamorio::OP_not:
            case dynamorio::OP_xadd:
            case dynamorio::OP_xchg:
            case dynamorio::OP_sar:
            case dynamorio::OP_shr:
            case dynamorio::OP_shl:
                tracker.can_replace[tracker.num_ops] = true;
                break;
            default:
                tracker.can_replace[tracker.num_ops] = false;
                break;
            }
        }

        // Try to combine this op with an existing one.
        for(unsigned i(0); i < tracker.num_ops; ++i) {
            if(tracker.ops[i].can_combine(op)) {
                tracker.ops[i].combine(op);
                tracker.can_replace[i] = (
                    tracker.can_replace[i]
                 && tracker.can_replace[tracker.num_ops]);
                return;
            }
        }

        tracker.ops[tracker.num_ops] = op;
        ++(tracker.num_ops);
    }


    /// Small state machine to track whether or not we can clobber the carry
    /// flag. The carry flag is relevant because we use the BT instruction to
    /// determine if the address is a watched address.
    void watchpoint_tracker::track_carry_flag(
        bool &next_reads_carry_flag
    ) throw() {
        const unsigned eflags(dynamorio::instr_get_eflags(in));

        // Assume flags do not propagate through RETs or CALLs.
        if(in.is_return() || in.is_call()) {
            next_reads_carry_flag = false;
            restore_carry_flag_before = false;
            restore_carry_flag_after = false;
            return;

        // For a specific propagation for CTIs.
        } else if(in.is_cti()) {
            next_reads_carry_flag = true;
            restore_carry_flag_before = true;
            restore_carry_flag_after = false;
            return;
        }

        // Read-after-write dependency.
        if(eflags & EFLAGS_READ_CF) {
            next_reads_carry_flag = true;
            restore_carry_flag_before = true;
            restore_carry_flag_after = false;

        // Output dependency.
        } else if(eflags & EFLAGS_WRITE_CF) {
            next_reads_carry_flag = false;
            restore_carry_flag_before = false;
            restore_carry_flag_after = false;

        // Inherit
        } else {
            restore_carry_flag_before = false;
            restore_carry_flag_after = next_reads_carry_flag;
        }
    }


    /// Perform watchpoint-specific mangling of an instruction.
    bool watchpoint_tracker::mangle(instruction_list &ls) throw() {
        instruction ret(in);
        bool mangled(true);
        dynamorio::reg_id_t spill_reg;
        operand op(*(ops[0]));

        // Mangle a push instruction.
        switch(in.op_code()) {

        case dynamorio::OP_push: {
            spill_reg = live_regs.get_zombie();

            // A dead register is available.
            if(spill_reg) {
                spill_regs.revive(spill_reg);

                const operand dead_reg(spill_reg);
                ret = ls.insert_before(in, mov_ld_(dead_reg, op));
                ls.insert_before(in, push_(dead_reg));

            // We need to spill a register to emulate the PUSH.
            } else {
                const operand dead_reg(spill_regs.get_zombie());

                // Don't need to protect from the userspace redzone on a push.
                ret = ls.insert_before(in, lea_(reg::rsp, reg::rsp[-8]));
                ret.set_pc(in.pc());
                ls.insert_before(in, push_(dead_reg));
                ret = ls.insert_before(in, mov_ld_(dead_reg, op));
                ls.insert_before(in, mov_st_(reg::rsp[8], dead_reg));
                ls.insert_before(in, pop_(dead_reg));
            }

            ls.remove(in);
            break;
        }

        case dynamorio::OP_pop: {
            spill_reg = live_regs.get_zombie();

            // A dead register is available.
            if(spill_reg) {
                spill_regs.revive(spill_reg);

                const operand dead_reg(spill_reg);
                ls.insert_before(in, pop_(dead_reg));
                ret = ls.insert_before(in, mov_st_(op, dead_reg));

            // We need to spill a register to emulate the POP.
            } else {
                const operand dead_reg(spill_regs.get_zombie());

                // Don't need to protect from the userspace redzone on a pop.
                ls.insert_before(in, push_(dead_reg));
                ls.insert_before(in, mov_ld_(dead_reg, reg::rsp[8]));
                ret = ls.insert_before(in, mov_st_(op, dead_reg));
                ls.insert_before(in, pop_(dead_reg));
                ls.insert_before(in, lea_(reg::rsp, reg::rsp[8]));
            }

            ls.remove(in);
            break;
        }

        // Emulate an XLAT so there are so many annoying special cases with it
        // in later instrumentation.
        case dynamorio::OP_xlat: {
            spill_reg = live_regs.get_zombie();

            if(spill_reg) {
                spill_regs.revive(spill_reg);

                const operand dead_reg(spill_reg);
                ls.insert_before(in, movzx_(dead_reg, reg::al));
                ls.insert_before(in, lea_(dead_reg, dead_reg + reg::rbx));
                ret = ls.insert_before(in, mov_ld_(reg::al, *dead_reg));
            } else {
                const operand dead_reg(spill_regs.get_zombie());
                ls.insert_before(in, push_(dead_reg));
                ls.insert_before(in, movzx_(dead_reg, reg::al));
                ls.insert_before(in, lea_(dead_reg, dead_reg + reg::rbx));
                ret = ls.insert_before(in, mov_ld_(reg::al, *dead_reg));
                ls.insert_before(in, pop_(dead_reg));
            }

            ls.remove(in);
            break;
        }

        default:
            mangled = false;
            break;
        }

        if(mangled) {
            ret.set_pc(in.pc());
            in = ret;
        }

        return mangled;
    }


#if 0
    /// Mangle an instruction that contains a memory reference using GS
    /// or FS.
    ///
    /// Note: This maintains the proper associations inside of
    ///       `tracker.ops`.
    void watchpoint_tracker::mangle_segment_mem_ops(
        instruction_list &ls
    ) throw() {
        // Try to mangle segmentation with GS and FS.
        if(1 != num_ops) {
            return;
        }

        printf("a");
        operand op(*(ops[0]));
        printf("b");

        if(dynamorio::DR_SEG_GS != op.seg.segment
        && dynamorio::DR_SEG_FS != op.seg.segment) {
            return;
        }

        dynamorio::reg_id_t spill_reg(live_regs.get_zombie());
        dynamorio::reg_id_t seg_reg(op.seg.segment);

        op.seg.segment = dynamorio::DR_REG_NULL;

        if(spill_reg) {
            operand spill(spill_reg);
            operand spill_seg(spill);
            spill_seg.seg.segment = seg_reg;

            ls.insert_before(in, lea_(spill, op));
            ls.insert_before(in, mov_ld_(spill, spill_seg));

            ops[0] = *spill;
            spill_regs.revive(spill_reg);

        } else {
            operand spill(spill_regs.get_zombie());
            operand spill_seg(spill);
            spill_seg.seg.segment = seg_reg;

            ls.insert_before(in, push_(spill));
            ls.insert_before(in, lea_(spill, op));
            ls.insert_before(in, mov_ld_(spill, spill_seg));
            ls.insert_after(in, pop_(spill));

            ops[0] = *spill;
        }

        granary_do_break_on_translate = true;
    }
#endif


    /// Save the carry flag, if needed. We use the carry flag extensively. For
    /// example, the BT instruction is used to both detect (and thus clobber
    /// CF) watchpoints, as well as to restore the carry flag, by testing the
    /// bit set with `SETcf` (`SETcc` variant).
    dynamorio::reg_id_t watchpoint_tracker::save_carry_flag(
        instruction_list &ls,
        instruction before,
        bool &spilled_carry_flag
    ) throw() {

        dynamorio::reg_id_t carry_flag(dynamorio::DR_REG_NULL);
        dynamorio::reg_id_t carry_flag_64(dynamorio::DR_REG_NULL);

        if(!restore_carry_flag_before
        && !restore_carry_flag_after) {
            return carry_flag_64;
        }

        carry_flag_64 = live_regs.get_zombie();

        // We need to spill a register to store a carry flag.
        if(!carry_flag_64) {
            carry_flag_64 = spill_regs.get_zombie();
            ASSERT(carry_flag_64);

            carry_flag = register_manager::scale(carry_flag_64, REG_8);
            ls.insert_before(before,
                push_(operand(carry_flag_64)));
            spilled_carry_flag = true;

        // We can steal a register to store the carry flag.
        } else {
            spill_regs.revive(carry_flag_64);
            carry_flag = register_manager::scale(carry_flag_64, REG_8);
        }

        ls.insert_before(before,
            setcc_(dynamorio::OP_setb, operand(carry_flag)));

        return carry_flag_64;
    }


#if GRANARY_IN_KERNEL
    /// Tries to match a binary pattern of the form:
    ///
    ///     data32 xchg %ax, %ax
    ///     <memory instruction>
    ///     data32 xchg %ax, %ax
    ///
    /// If the pattern is matched then `tracker.might_be_user_address` is
    /// set to true.
    void watchpoint_tracker::match_userspace_address_deref(void) throw() {
        enum {
            DATA32_XCHG_AX_AX = 0x906666U
        };

        // Try to determine if this might be an access to a user space
        // address by pattern matching the binary instructions.
        if(in.next().is_valid() && in.prev().is_valid()
        && in.next().pc() && in.prev().pc()
        && 3 == in.next().encoded_size()
        && 3 == in.prev().encoded_size()) {
            const unsigned data32_xchg_ax_ax(DATA32_XCHG_AX_AX);
            might_be_user_address = (
                0 == memcmp(in.next().pc(), &data32_xchg_ax_ax, 3)
             && 0 == memcmp(in.prev().pc(), &data32_xchg_ax_ax, 3));
        }
    }
#endif


#if !GRANARY_IN_KERNEL
    /// Add in a user space redzone guard if necessary. This looks for a PUSH
    /// instruction anywhere between `first` and `last` and if it finds one then
    /// it guards the entire instrumented block with a redzone shift.
    void guard_redzone(
        instruction_list &ls,
        instruction first,
        instruction last
    ) throw() {
        bool has_push(false);
        for(instruction in(first.next());
            in != last && in.is_valid();
            in = in.next()) {

            if(dynamorio::OP_push == in.op_code()) {
                has_push = true;
                break;
            }
        }

        if(!has_push) {
            return;
        }

        ls.insert_after(first, lea_(reg::rsp, reg::rsp[-REDZONE_SIZE]));

        // TODO: Try to merge adjacent redzone guards.

        ls.insert_before(last, lea_(reg::rsp, reg::rsp[REDZONE_SIZE]));
    }
#endif


    /// Replace/update operands around the memory instruction. This will
    /// update the `labels` field of the `operand_tracker` with labels in
    /// instruction stream so that a `Watcher` can inject its own specific
    /// instrumentation in at those well-defined points. This will also
    /// update the `sources` and `dests` field appropriately so that the
    /// `Watcher`s read/write visitors can access operands containing the
    /// watched addresses.
    void watchpoint_tracker::visit_operands(instruction_list &ls) throw() {

        instruction before(ls.insert_before(in, label_()));

        // Need multiple `after` labels so that PUSHes and POPs are in the
        // correct order.
        instruction after[MAX_NUM_OPERANDS];
        for(unsigned i(0); i < this->num_ops; ++i) {
            after[i] = ls.insert_after(in, label_());
        }

        // Figure out exactly which registers this (and only this) instruction
        // kills.
        register_manager regs_killed_by_in;
        regs_killed_by_in.visit_dests(in);

        // The registers that can be used for storing our computed address.
        register_manager regs_used_by_in;
        regs_used_by_in.visit(in);

        // If an XMM reg is used then we can replace all operands.
        if(spill_regs.has_live_xmm()) {
            can_replace[0] = true;
            can_replace[1] = true;
        }

        // Save the carry flag.
        bool spilled_carry_flag(false);
        dynamorio::reg_id_t carry_flag(this->save_carry_flag(
            ls, before, spilled_carry_flag));

        // The register that stores the (potentially) computed memory address.
        dynamorio::reg_id_t addr_reg;

        for(unsigned i(0); i < num_ops; ++i) {

            instruction not_a_watchpoint(label_());
            const operand_ref &op(ops[i]);
            const operand original_op(*op);
            const bool can_change(can_replace[i]);
            bool addr_reg_was_spilled(false);

            // Try to figure out if this operand is a base/disp type that is
            // "simple", i.e. `(R)`, or `(,R,1)`. This is used for later
            // optimisation.
            bool op_is_simple_reg(false);
            if(!original_op.value.base_disp.index_reg) {
                op_is_simple_reg = !original_op.value.base_disp.disp;
            } else if(!original_op.value.base_disp.base_reg) {
                op_is_simple_reg = !original_op.value.base_disp.disp
                                && (1 >= original_op.value.base_disp.scale);
            }

            // Spill the register if necessary.
            bool addr_reg_is_only_op_reg(false);

            // We have a special mechanism for determining if we need to restore
            // `addr_reg`, so we don't go through the usual `live_regs`
            // approach.

            // We don't want to risk taking over a dead register if we can't
            // change the original register, because if we can't change it then
            // we might need to restore that register.
            addr_reg = dynamorio::DR_REG_NULL;
            if(can_change) {

                // TODO: This is potentially unsafe!!! Hopefully the assertion
                //       below when deciding if the register should be restored
                //       will catch such unsafe cases ;-)
                addr_reg = regs_used_by_in.get_zombie();
            }

            // Still try to steal if we couldn't above.
            if(!addr_reg) {
                addr_reg = live_regs.get_zombie();
            } else {
                live_regs.revive(addr_reg);
            }

            // We've got to spill a register to store the computed (potentially)
            // watched address.
            if(!addr_reg) {
                addr_reg = spill_regs.get_zombie();
                addr_reg_was_spilled = true;
                addr_reg_is_only_op_reg = false;
                ls.insert_before(before, push_(operand(addr_reg)));

            // We have stolen a register for storing the watched address.
            } else {
                spill_regs.revive(addr_reg);

                // Try to detect of addr_reg is the only register used in
                // the operand.
                if(!original_op.value.base_disp.index_reg) {
                    addr_reg_is_only_op_reg = (
                        addr_reg == register_manager::scale(
                            original_op.value.base_disp.base_reg, REG_64));

                } else if(!original_op.value.base_disp.base_reg) {
                    addr_reg_is_only_op_reg = (
                        addr_reg == register_manager::scale(
                            original_op.value.base_disp.index_reg, REG_64));
                }

                // If we're using an explicit segment then we technically have
                // more registers in play.
                if(original_op.seg.segment) {
                    addr_reg_is_only_op_reg = false;
                }
            }

            const operand addr(addr_reg);

            // We are using the original register, but we need to do compute the
            // final address as there is a scale or displacement.
            bool compute_addr(true);
            if(addr_reg_is_only_op_reg) {
                compute_addr = 0 != original_op.value.base_disp.disp
                            || 1 < original_op.value.base_disp.scale;
            }

            // If we aren't stealing the register that we will (in most cases)
            // test, then we need to compute the memory address to be
            // dereferenced.
            if(compute_addr) {
                ls.insert_before(before, lea_(addr, *op));

                // Replace the operand if possible.
                if(can_change) {
                    operand op_replacement(*addr);
                    op_replacement.size = original_op.size;
                    const_cast<operand_ref &>(op) = op_replacement;
                }
            }

            // Test for a watched address. This sets the carry flag, and lets
            // us go ahead and re-purpose `addr` if necessary for later
            // restoring.
            ls.insert_before(before,
                bt_(addr, int8_(DISTINGUISHING_BIT_OFFSET)));

            // Figure out what the original (single) register of the operand
            // is, if any. This is so that later we can restore the original
            // address register to its previous state if necessary.
            dynamorio::reg_id_t original_addr_reg(dynamorio::DR_REG_NULL);

            // Can't be changed and the register that we clobbered isn't the
            // one that's part of the instruction.
            if(!can_change) {
                original_addr_reg = original_op.value.base_disp.base_reg;

            // Two cases:
            //      - We're clobbering the original register operand that is
            //        the source of the address in the operand, so remember it.
            //      - We're not clobbering the original (it is live after the
            //        op or elsewhere within the op), but we are replacing the
            //        original op with our new one, and s the original has
            //        changed.
            } else {
                original_addr_reg = addr_reg;
            }

            // Curious bug detected.
            ASSERT(!(true
             && can_change
             && addr_reg_is_only_op_reg
             && regs_killed_by_in.is_live(original_addr_reg)
             && live_regs_after.is_live(original_addr_reg)
            ));

            // Figure out if we need to restore `original_addr_reg` after the
            // instruction executes.
            //
            // Need to double check that we don't uselessly attempt to restore
            // a spilled register.
            bool restore_unwatched_reg(false);
            bool restore_full_unwatched_reg(false);
            if(regs_killed_by_in.is_live(original_addr_reg)
            && live_regs_after.is_live(original_addr_reg)
            && (!addr_reg_was_spilled || original_addr_reg != addr_reg)) {

                restore_unwatched_reg = true;

                // If we are using string instructions then operand address
                // might have been changed by the instruction itself. As such,
                // we need to restore the counter index, but nothing else.
                if(dynamorio::OP_ins <= in.op_code()
                && in.op_code() <= dynamorio::OP_repne_scas) {
                    restore_full_unwatched_reg = false;
                } else {
                    restore_full_unwatched_reg = true;

#if CONFIG_ENABLE_ASSERTIONS
                    // Try to detect really unusual corner cases like the MOVS
                    // instructions.
                    register_manager live_after;
                    live_after.visit_dests_check(in);
#endif
                }
            }

            // Bug detected; We need to restore the original register, but we've
            // gone and potentially clobbered it.
            ASSERT(!(true
             && compute_addr
             && addr_reg_is_only_op_reg
             && restore_unwatched_reg
            ));

            // Figure out if we need to save `original_addr_reg` before the
            // instruction.
            //
            //      - If we're not going to restore it, then we clearly don't
            //        need to save it.
            //      - If we can replace the operand, then we don't need to
            //        restore the register because we would never have changed
            //        it.
            //      - If the operand is not simple, i.e. if the `LEA`
            //
            // What is an example of a case where we need to restore the
            // address after the instruction, but where we don't also need to
            // save it beforehand (i.e. where we can change the operand)?
            //
            //      ?????
            //
            // Note: If no such condition exists then the conditions which
            //       determine `restore_unwatched_reg` should be tightened.
            const operand original_addr(original_addr_reg);
            bool double_restore_unwatched_reg(false);
            if(restore_unwatched_reg) {

                // Bug detected; use this to figure out interesting cases
                // above that our current framework doesn't handle.
                ASSERT(!can_change);

                // Two cases:
                //      - If the operand is not a simple register, then any
                //        previous LEA might have computed the effective address
                //        (including displacement/scale), and so that previously
                //        computed address is unsatisfactory for restoring.
                //      - If the address was not previously computed, then we
                //        need to compute it.
                if(!op_is_simple_reg || !compute_addr) {
                    double_restore_unwatched_reg = true;
                    ls.insert_before(before, mov_st_(addr, original_addr));
                }
            }

            // Jump around the slow path if a watched address is detected.
            ls.insert_before(before,
                mangled(IF_USER_ELSE(jnb_, jb_)(instr_(not_a_watchpoint))));

#if GRANARY_IN_KERNEL
            /// Jump around user space addresses, if detected.
            if(might_be_user_address) {
                ls.insert_before(before,
                    bt_(addr, int8_(DISTINGUISHING_BIT_OFFSET - 1)));
                ls.insert_before(before,
                    mangled(jnb_(instr_(not_a_watchpoint))));
            }
#endif

            // We've found a watched address.
            //
            // Note: we assume that higher level instrumentation will not
            //       clobber the operands from sources/dests.
            this->labels[i] = ls.insert_before(before, label_());
            this->regs[i] = addr;
            this->sizes[i] = static_cast<operand_size>(
                dynamorio::opnd_size_in_bytes(original_op.size));

            // On the slow path, we might still need to restore `original_addr`
            // later, so make sure it's saved before we overwrite
            // `original_addr`.
            if(double_restore_unwatched_reg) {
                ls.insert_before(before, mov_st_(addr, original_addr));
            }

            // Mask the high order 16 bits.
            const operand original_addr_16(
                register_manager::scale(original_addr_reg, REG_16));
            ls.insert_before(before, bswap_(original_addr));
            ls.insert_before(before, mov_imm_(
                original_addr_16,
                int16_(IF_USER_ELSE(0ULL, ~0ULL))));
            ls.insert_before(before, bswap_(original_addr));

            // target of the jump if this isn't a watched address.
            ls.insert_before(before, not_a_watchpoint);

            // Restore a register after the operand if necessary.
            if(restore_unwatched_reg) {

                // Potentially a bug, or just an optimisation opportunity.
                ASSERT(addr_reg != original_addr_reg);

                // Can't restore from a killed register.
                ASSERT(regs_killed_by_in.is_live(addr_reg));

                // Restore the full register, because it has not been modified
                // by this instruction.
                if(restore_full_unwatched_reg) {
                    ls.insert_before(after[i], mov_ld_(original_addr, addr));

                // Restore only the watched bits, because it the remaining bits
                // have potentially been modified by this instruction.
                } else {
                    ls.insert_before(after[i], bswap_(original_addr));
                    ls.insert_before(after[i], bswap_(addr));
                    ls.insert_before(after[i], mov_st_(
                        original_addr_16,
                        operand(register_manager::scale(addr_reg, REG_16))));
                    ls.insert_before(after[i], bswap_(original_addr));
                }
            }

            // Restore the register if it was spilled.
            if(addr_reg_was_spilled) {
                ls.insert_before(after[i], pop_(operand(addr_reg)));
            }
        }

        // Restore the carry flag before executing the instruction.
        if(this->restore_carry_flag_before) {
            ls.insert_before(before,
                bt_(operand(register_manager::scale(carry_flag, REG_16)),
                    int8_(0)));
        }

        // Restore the carry flag after executing the instruction.
        if(this->restore_carry_flag_after) {
            ls.insert_before(after[0],
                bt_(operand(register_manager::scale(carry_flag, REG_16)),
                    int8_(0)));
        }

        // Restore the register used to spill the carry flag, if necessary.
        if(spilled_carry_flag) {
            ls.insert_before(after[0], pop_(operand(carry_flag)));
        }
    }
}}
