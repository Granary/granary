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

        // In 64-bit mode, we'll ignore GS and FS-segmented addresses because
        // the offsets from those are generally not addresses.
        if(dynamorio::BASE_DISP_kind != op->kind
        || dynamorio::DR_SEG_GS == op->seg.segment
        || dynamorio::DR_SEG_FS == op->seg.segment) {
            return;
        }

        // Guard against RSP up here as it will never be returned by get_zombie
        // (as a preventive measure for preventing the stack pointer from being
        // clobbered). Also, treat all stack addresses as unwatched.
        if(dynamorio::DR_REG_RSP == op->value.base_disp.base_reg
        || dynamorio::DR_REG_RSP == op->value.base_disp.index_reg) {
            return;
        }

        register_manager rm;
        const operand ref_to_op(*op);
        rm.kill(ref_to_op);

        // make sure we've got at least one general purpose register
        dynamorio::reg_id_t regs[2];
        regs[0] = rm.get_zombie();
        regs[1] = rm.get_zombie();
        if(!regs[0]) {
            return;
        }

        // If we consider RBP as a frame pointer then prevent it from being
        // clobbered as well. Also, treat all stack addresses as unwatched.
#if WP_IGNORE_FRAME_POINTER
        if(dynamorio::DR_REG_RBP == regs[0]
        || dynamorio::DR_REG_RBP == regs[1]) {
            return;
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
            // optimisation for simple instructions that are typically used and
            // are known to have replaceable operands.
            case dynamorio::OP_mov_ld:
            case dynamorio::OP_mov_st:
            case dynamorio::OP_add:
            case dynamorio::OP_sub:
            case dynamorio::OP_inc:
            case dynamorio::OP_dec:
            case dynamorio::OP_or:
            case dynamorio::OP_xor:
            case dynamorio::OP_xadd:
            case dynamorio::OP_xchg:
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
    void track_carry_flag(
        watchpoint_tracker &tracker,
        instruction in,
        bool &next_reads_carry_flag
    ) throw() {
        const unsigned eflags(dynamorio::instr_get_eflags(in));

        // assume flags do not propagate through RETs or CALLs.
        if(in.is_return() || in.is_call()) {
            next_reads_carry_flag = false;
            tracker.restore_carry_flag_before = false;
            tracker.restore_carry_flag_after = false;
            return;

        // for a specific propagation for CTIs.
        } else if(in.is_cti()) {
            next_reads_carry_flag = true;
            tracker.restore_carry_flag_before = true;
            tracker.restore_carry_flag_after = false;
            return;
        }

        // Read-after-write dependency.
        if(eflags & EFLAGS_READ_CF) {
            next_reads_carry_flag = true;
            tracker.restore_carry_flag_before = true;
            tracker.restore_carry_flag_after = false;

        // Output dependency.
        } else if(eflags & EFLAGS_WRITE_CF) {
            next_reads_carry_flag = false;
            tracker.restore_carry_flag_before = false;
            tracker.restore_carry_flag_after = false;

        // inherit
        } else {
            tracker.restore_carry_flag_before = false;
            tracker.restore_carry_flag_after = next_reads_carry_flag;
        }
    }


    /// Perform watchpoint-specific mangling of an instruction.
    instruction mangle(
        instruction_list &ls,
        instruction in,
        watchpoint_tracker &tracker
    ) throw() {
        instruction ret(in);
        register_manager live(tracker.live_regs);
        register_manager spill;

        spill.kill_all();
        spill.revive(in);
        live.revive(in);

        // mangle a push instruction.
        switch(in.op_code()) {

        case dynamorio::OP_push: {
            dynamorio::reg_id_t spill_reg(live.get_zombie());
            operand op = *(tracker.ops[0]);

            // a dead register is available.
            if(spill_reg) {
                const operand dead_reg(spill_reg);
                ret = ls.insert_before(in, mov_ld_(dead_reg, op));
                ls.insert_before(in, push_(dead_reg));

            // we need to spill a register to emulate the PUSH.
            } else {
                spill_reg = spill.get_zombie();

                ASSERT(spill_reg);

                const operand dead_reg(spill_reg);

                // don't need to protect from the userspace redzone on a push.
                ret = ls.insert_before(in, lea_(reg::rsp, reg::rsp[-8]));
                ret.set_pc(in.pc());
                ls.insert_before(in, push_(dead_reg));
                ret = ls.insert_before(in, mov_ld_(dead_reg, op));
                ls.insert_before(in, mov_st_(reg::rsp[8], dead_reg));
                ls.insert_before(in, pop_(dead_reg));
            }

            tracker.live_regs.revive(spill_reg);
            ls.remove(in);
            break;
        }

        case dynamorio::OP_pop: {
            dynamorio::reg_id_t spill_reg(live.get_zombie());
            operand op = *tracker.ops[0];

            // a dead register is available.
            if(spill_reg) {
                const operand dead_reg(spill_reg);
                ret = ls.insert_before(in, pop_(dead_reg));
                ret.set_pc(in.pc());
                ret = ls.insert_before(in, mov_st_(op, dead_reg));

            // we need to spill a register to emulate the POP.
            } else {
                spill_reg = spill.get_zombie();
                ASSERT(spill_reg);

                const operand dead_reg(spill_reg);

                // don't need to protect from the userspace redzone on a pop.
                ret = ls.insert_before(in, push_(dead_reg));
                ret.set_pc(in.pc());
                ls.insert_before(in, mov_ld_(dead_reg, reg::rsp[8]));
                ret = ls.insert_before(in, mov_st_(op, dead_reg));
                ls.insert_before(in, pop_(dead_reg));
                ls.insert_before(in, lea_(reg::rsp, reg::rsp[8]));
            }

            tracker.live_regs.revive(spill_reg);
            ls.remove(in);
            break;
        }

        default: break;
        }

        return ret;
    }


    /// The scale of registers that will be used to mask the tainted bits of a
    /// watched address.
    const register_scale REG_SCALE = (8 == NUM_HIGH_ORDER_BITS
        ? REG_8
        : REG_16);


    /// Save the carry flag, if needed. We use the carry flag extensively. For
    /// example, the BT instruction is used to both detect (and thus clobber
    /// CF) watchpoints, as well as to restore the carry flag, by testing the
    /// bit set with `SETcf` (`SETcc` variant).
    static dynamorio::reg_id_t save_carry_flag(
        instruction_list &ls,
        instruction before,
        watchpoint_tracker &tracker,
        bool &spilled_carry_flag
    ) throw() {

        dynamorio::reg_id_t carry_flag(0);
        dynamorio::reg_id_t carry_flag_64(0);

        if(!tracker.restore_carry_flag_before
        && !tracker.restore_carry_flag_after) {
            return carry_flag_64;
        }

        register_manager live(tracker.live_regs);
        register_manager spill;

        spill.kill_all();
        spill.revive(tracker.in);
        live.revive(tracker.in);

        carry_flag_64 = live.get_zombie();
        if(!carry_flag_64) {
            carry_flag_64 = spill.get_zombie();
            ASSERT(carry_flag_64);

            carry_flag = register_manager::scale(carry_flag_64, REG_8);
            ls.insert_before(before,
                push_(operand(carry_flag_64)));
            spilled_carry_flag = true;

        } else {
            carry_flag = register_manager::scale(carry_flag_64, REG_8);
        }

        ls.insert_before(before,
            setcc_(dynamorio::OP_setb, operand(carry_flag)));

        tracker.live_regs.revive(carry_flag_64);
        return carry_flag_64;
    }


    /// Revive certain registers. This is special-purposed to look for matching
    /// register operands in an instruction (e.g. `RAX` in `CMPXCHG`), and allow
    /// us to ensure that `RAX` is not seen as dead (as would be the case if we
    /// only visited destination operands). The issue at play here is that `RAX`
    /// is an implicit operand, and the other infrastructure for handling
    /// implicits looks for memory operands and not register operands.
    static void kill_register_op(
        const operand_ref &op,
        register_manager &sources,
        register_manager &dests
    ) throw() {
        if(SOURCE_OPERAND == op.kind) {
            if(dynamorio::REG_kind == op->kind) {
                sources.kill(op->value.reg);
            }
        } else if(DEST_OPERAND == op.kind) {
            if(dynamorio::REG_kind == op->kind) {
                dests.kill(op->value.reg);
            } else {
                dests.revive(*op);
            }
        }
    }


    /// Need to make sure that registers (that are not used in memory
    /// operands) but that are used as both sources and dests (e.g.
    /// RAX in CMPXCHG) are treated as live.
    void revive_matching_operand_regs(
        register_manager &rm,
        instruction in
    ) throw() {
        register_manager sources;
        register_manager dests;
        in.for_each_operand(kill_register_op, sources, dests);

        for(dynamorio::reg_id_t dead_dest(dests.get_zombie());
            dead_dest;
            dead_dest = dests.get_zombie()) {

            if(sources.is_dead(dead_dest)) {
                rm.revive(dead_dest);
            }
        }
    }


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
    void visit_operands(
        instruction_list &ls,
        instruction in,
        watchpoint_tracker &tracker
    ) throw() {

        instruction before(ls.insert_before(in, label_()));
        instruction after(ls.insert_after(in, label_()));

        // Helps use get spill registers that *aren't* used by the instruction.
        register_manager in_regs;
        in_regs.kill_all();
        in_regs.revive(in);

        // If an XMM reg is used then we can replace all operands.
        if(in_regs.has_live_xmm()) {
            tracker.can_replace[0] = true;
        }

        // Save the carry flag.
        bool spilled_carry_flag(false);
        dynamorio::reg_id_t carry_flag(save_carry_flag(
            ls, before, tracker, spilled_carry_flag));

        // The register that stores the (potentially) computed memory address.
        dynamorio::reg_id_t op_reg[MAX_NUM_OPERANDS] = {0};
        bool op_reg_was_spilled[MAX_NUM_OPERANDS] = {false};
        dynamorio::reg_id_t op_reg_has_watched_bits[MAX_NUM_OPERANDS] = {0};

        // constructor for the immediate value that is used to mask the watched
        // address.
        operand (*mov_mask_imm_)(uint64_t) = (8 == NUM_HIGH_ORDER_BITS
            ? int8_
            : int16_);

        const bool is_xlat(dynamorio::OP_xlat == in.op_code());

        IF_TEST( register_manager dest_regs; )
        IF_TEST( dest_regs.visit_dests(in); )
        IF_TEST( revive_matching_operand_regs(dest_regs, in); )

        for(unsigned i(0); i < tracker.num_ops; ++i) {

            const operand_ref &op(tracker.ops[i]);
            const bool can_change(tracker.can_replace[i]);

            // Try to figure out if this operand is a base/disp type that is
            // "simple", i.e. `(R)`, or `(,R,1)`.
            bool op_is_simple_reg(false);
            if(!op->value.base_disp.index_reg) {
                op_is_simple_reg = !op->value.base_disp.disp;
            } else if(!op->value.base_disp.base_reg) {
                op_is_simple_reg = !op->value.base_disp.disp
                                && (1 >= op->value.base_disp.scale);
            }

            // Spill the register if necessary.
            bool using_original_reg(false);
            bool try_get_op_reg(false);

            do {
                try_get_op_reg = false;
                op_reg[i] = tracker.live_regs.get_zombie();

                if(!op_reg[i]) {
                    op_reg[i] = in_regs.get_zombie();
                    op_reg_was_spilled[i] = true;
                    using_original_reg = false;
                    ls.insert_before(before, push_(operand(op_reg[i])));
                } else {

                    // Try to detect of op_reg[i] is actually one of the
                    // registers used in the operand.
                    if(!op->value.base_disp.index_reg) {
                        using_original_reg = op_reg[i] == register_manager::scale(
                            op->value.base_disp.base_reg, REG_64);

                    } else if(!op->value.base_disp.base_reg) {
                        using_original_reg = op_reg[i] == register_manager::scale(
                            op->value.base_disp.index_reg, REG_64);
                    }

                    // The `op_reg` is killed in the instruction, but live after
                    // the instruction. `!using_original_reg` is a necessary
                    // condition for checking if we need to save/restore the
                    // original reg's (inside the operand) value.
                    if(!using_original_reg
                    && !is_xlat
                    && tracker.live_regs.is_dead(op_reg[i])
                    && tracker.live_regs_after.is_live(op_reg[i])) {
                        try_get_op_reg = true;
                    }
                }
            } while(try_get_op_reg);

            dynamorio::reg_id_t op_reg_to_test(op_reg[i]);

            // Note: we are guaranteed that op_reg[i] != RAX or RBX. See
            // instrument.h.
            if(is_xlat) {
                using_original_reg = true;
                op_reg_to_test = dynamorio::DR_REG_RBX;
            }

            const operand addr(op_reg[i]);

            // If we aren't stealing the reg that we will (in most cases) test,
            // then we need to compute the memory address to be dereferenced.
            if(!using_original_reg) {
                ls.insert_before(before, lea_(addr, *op));

                // Replace the operand if possible.
                if(can_change) {
                    operand op_replacement(*addr);
                    op_replacement.size = op->size;
                    const_cast<operand_ref &>(op) = op_replacement;
                }
            }

            // Test for a watched address.
            ls.insert_before(before,
                bt_(operand(op_reg_to_test), int8_(DISTINGUISHING_BIT_OFFSET)));

            // save the original register value if we're not modifying the
            // original operand. If we can change the operand, then we already
            // have, so we don't need to worry about index/displacement of
            // this operand.
            dynamorio::reg_id_t original_addr_reg(op_reg[i]);
            bool try_save_original_addr_reg(true);
            bool restore_unwatched_reg(false);

            // Can't be changed and the register that we clobbered isn't the
            // one that's part of the instruction.
            if(!can_change && !using_original_reg) {
                original_addr_reg = op->value.base_disp.base_reg;

            // XLAT's don't use their original reg; we need to specifically
            // save RBX because it will be clobbered.
            } else if(is_xlat) {
                original_addr_reg = dynamorio::DR_REG_RBX;

            // Don't need to save the register that we might clobber for later.
            } else {
                try_save_original_addr_reg = false;
            }

            // Try to see if we need to save, and thus restore, the original
            // value of the register.
            operand original_addr(original_addr_reg);
            if(try_save_original_addr_reg
            && tracker.live_regs_after.is_live(original_addr_reg)) {
                restore_unwatched_reg = true;

                // Only save the register if the LEA doesn't effect a save of
                // the register's original value for us.
                if(!op_is_simple_reg) {
                    ls.insert_before(before, mov_st_(addr, original_addr));
                }
            }

            // The register is both live and dead in the dests. This happens
            // in cases like STOS, where RDI is read in the sources, read
            // in the dests (for the memory op), and written in the dests
            // to update to the next byte/word/doubleword/quadword. So, we
            // need to save the high-order watchpoint bytes for later restoring.
            if(dynamorio::OP_ins <= in.op_code()
                   && in.op_code() <= dynamorio::OP_repne_scas) {

                op_reg_has_watched_bits[i] = register_manager::scale(
                    original_addr_reg, REG_SCALE);

                restore_unwatched_reg = false;
            }

            // Jump around the slow path if a watched address is detected.
            instruction not_a_watchpoint(label_());
            ls.insert_before(before,
                mangled(IF_USER_ELSE(jnb_, jb_)(instr_(not_a_watchpoint))));

            // if this was an XLAT, then we didn't resolve the full watched
            // address into `addr`. Do it now.
            if(is_xlat) {
                ls.insert_before(before, movzx_(addr, reg::al));
                ls.insert_before(before, lea_(addr, addr + reg::rbx));
            }

            // we've found a watchpoint; note: we assume that watchpoint-
            // implementation instrumentation will not clobber the operands
            // from sources/dests.
            tracker.labels[i] = ls.insert_before(before, label_());
            tracker.regs[i] = addr;

            // In the case of XLAT, we (unfortunately) still need to save
            // RBX to the clobbered reg so that in the fast path we can
            // restore it, thus making the fast and slow paths uniform. This is
            // because above we re-clobber `addr` with the computed XLAT address
            // so that we can expose it to higher-level instrumentation.
            if(is_xlat && restore_unwatched_reg) {
                ls.insert_before(before, mov_st_(addr, reg::rbx));
            }

            // mask the high order bits.
            ls.insert_before(before, bswap_(original_addr));
            ls.insert_before(before, mov_imm_(
                operand(register_manager::scale(original_addr_reg, REG_SCALE)),
                mov_mask_imm_(IF_USER_ELSE(0ULL, ~0ULL))));
            ls.insert_before(before, bswap_(original_addr));

            // target of the jump if this isn't a watched address.
            ls.insert_before(before, not_a_watchpoint);

            // Restore a register after the operand if necessary.
            if(restore_unwatched_reg) {
                ls.insert_before(after, mov_ld_(original_addr, addr));
            }
        }

        // Restore the carry flag before executing the instruction.
        if(tracker.restore_carry_flag_before) {
            ls.insert_before(before,
                bt_(operand(register_manager::scale(carry_flag, REG_16)),
                    int8_(0)));
        }

        // Restore any spilled/clobbered registers.
        for(int i(tracker.num_ops); i --> 0; ) {

            // Restore the watched bits in the case of string instructions. This
            // starts with two BSWAPs so that we can get at the stored bits in
            // op_reg[i] on both the fast and slow paths.
            if(op_reg_has_watched_bits[i]) {
               operand orig_reg(register_manager::scale(
                    op_reg_has_watched_bits[i], REG_64));

                ls.insert_before(after, bswap_(orig_reg));
                ls.insert_before(after, bswap_(operand(op_reg[i])));
                ls.insert_before(after, mov_st_(
                    operand(op_reg_has_watched_bits[i]),
                    operand(register_manager::scale(op_reg[i], REG_SCALE))));
                ls.insert_before(after, bswap_(orig_reg));
            }

            if(op_reg_was_spilled[i]) {
                ls.insert_before(after, pop_(operand(op_reg[i])));
            }
        }

        // restore the carry flag after executing the instruction.
        if(tracker.restore_carry_flag_after) {
            ls.insert_before(after,
                bt_(operand(register_manager::scale(carry_flag, REG_16)),
                    int8_(0)));
        }

        if(spilled_carry_flag) {
            ls.insert_before(after, pop_(operand(carry_flag)));
        }
    }
}}
