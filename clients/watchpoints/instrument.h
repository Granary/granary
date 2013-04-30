/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: 2013-04-20
 *      Author: pag
 */

#ifndef WATCHPOINT_INSTRUMENT_H_
#define WATCHPOINT_INSTRUMENT_H_

#include "granary/client.h"

/// Enable if %RBP should be treated as a frame pointer and not as a potential
/// watched address.
#define IF_WP_IGNORE_FRAME_POINTER(...) __VA_ARGS__


namespace client {

    namespace wp {

        enum : uint64_t {
            /// Maximum number of memory operands in a single instruction.
            MAX_NUM_OPERANDS            = 2,

            /// The value of the distinguishing bit. Assumes the 64-bit split-
            /// address space mode, where kernel addresses have 1 as their high-
            /// order bits, and user addresses have 0 as their high-order
            /// bits.
            DISTINGUISHING_BIT          = IF_USER_ELSE(1, 0),

            /// Number of high-order bits reserved for watchpoint tainting. One
            /// of these bits is a distinguishing bit. There must be at least
            /// two, otherwise user/kernel space addresses cannot be
            /// distinguished in kernel space.
            NUM_HIGH_ORDER_BITS         = 16,

            /// Number of bits in an address.
            NUM_BITS_PER_ADDR           = 64,

            /// The bit offset, where LSB=0 and MSB=(NUM_BITS_PER_ADDR - 1), of
            /// the distinguishing bit of an un/watched address.
            DISTINGUISHING_BIT_OFFSET   = (
                NUM_BITS_PER_ADDR - NUM_HIGH_ORDER_BITS),

            /// OR this with a user address to add a watchpoint distinguisher
            /// bit to the address
            USER_OR_MASK                 = (
                1ULL << DISTINGUISHING_BIT_OFFSET),

            /// AND this with with a kernel address to add a watchpoint
            /// distinguisher bit to the address
            KERNEL_AND_MASK              = ~USER_OR_MASK,

            /// Bit mask to use
            DISTINGUISHING_BIT_MASK      = IF_USER_ELSE(
                USER_OR_MASK, KERNEL_AND_MASK)
        };


        static_assert(1 < NUM_HIGH_ORDER_BITS,
            "The number of high-order bits must be at least two otherwise "
            "user space addresses won't be distinguishable from kernel space "
            "addresses.");

        static_assert((false
            || 8 == NUM_HIGH_ORDER_BITS
            || 16 == NUM_HIGH_ORDER_BITS),
            "The number of high order bits for a watched address descriptor "
            "index must be either 8 or 16, as sub-8/16 writes cannot be made "
            "to registers without clobbering flags.");



        /// Keeps track of instruction-specific state for the watchpoint
        /// instrumentation.
        struct watchpoint_tracker {

            /// Tracks which registers are live at the current instruction
            /// that we are instrumenting.
            granary::register_manager live_regs;

            /// Tracks which registers have been used by the watchpoint
            /// translation so that one step doesn't clobber another. Tracking
            /// this is necessary when there are no live registers.
            granary::register_manager used_regs;

            /// Direct references to the operands within an instruction.
            granary::operand_ref ops[MAX_NUM_OPERANDS];

            /// Instruction labels where specific watchpoint implementations
            /// can add in their code. If no code is added before/after these
            /// labels, then the net effect is that the code will just "mask"
            /// out all watchpoint accesses to behave as normal.
            granary::instruction labels[MAX_NUM_OPERANDS];

            /// Conveniences for specific watchpoint implementations so that
            /// they can know where the watchpoint info is stored.
            granary::operand sources[MAX_NUM_OPERANDS];
            granary::operand dests[MAX_NUM_OPERANDS];

            /// True iff the ith operand (in `ops`) can be modified, or if it
            /// must be left as-is.
            bool can_replace[MAX_NUM_OPERANDS];

            /// The number of operands that need to be instrumented.
            unsigned num_ops;

            /// Track the carry flag.
            bool restore_carry_flag_before;
            bool restore_carry_flag_after;

            dynamorio::reg_id_t get_zombie(void) throw();
            dynamorio::reg_id_t get_zombie(granary::register_scale scale) throw();

            dynamorio::reg_id_t get_spill(void) throw();
            dynamorio::reg_id_t get_spill(granary::register_scale scale) throw();
        };


        /// Find memory operands that might need to be checked for watchpoints.
        /// If one is found, then num_ops is incremented, and the operand
        /// reference is stored in the passed array.
        void find_memory_operand(
            const granary::operand_ref &,
            watchpoint_tracker &
        ) throw();


        /// Add in a user space redzone guard if necessary. This looks for a PUSH
        /// instruction anywhere between `first` and `last` and if it finds one then
        /// it guards the entire instrumented block with a redzone shift.
        void guard_redzone(
            granary::instruction_list &ls,
            granary::instruction first,
            granary::instruction last
        ) throw();


        /// Replace/update operands around the memory instruction. This will
        /// update the `labels` field of the `operand_tracker` with labels in
        /// instruction stream so that a `Watcher` can inject its own specific
        /// instrumentation in at those well-defined points. This will also
        /// update the `sources` and `dests` field appropriately so that the
        /// `Watcher`s read/write visitors can access operands containing the
        /// watched addresses.
        void visit_operands(
            granary::instruction_list &,
            granary::instruction,
            watchpoint_tracker &
        ) throw();


        /// Perform watchpoint-specific mangling of an instruction.
        granary::instruction mangle(
            granary::instruction_list &,
            granary::instruction,
            watchpoint_tracker &
        ) throw();


        /// Small state machine to track whether or not we can clobber the carry
        /// flag. The carry flag is relevant because we use the BT instruction to
        /// determine if the address is a watched address.
        void track_carry_flag(
            watchpoint_tracker &,
            granary::instruction,
            bool &
        ) throw();
    }


    /// Generalised behavioural watchpoints instrumentation. A `Watcher` type is
    /// passed, which generates instrumentation code when watched memory
    /// operations on detected.
    template <typename Watcher>
    struct watchpoints : public granary::instrumentation_policy {
    public:

        /// Instrument a basic block.
        static granary::instrumentation_policy visit_basic_block(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            using namespace granary;

            instruction prev_in;
            register_manager next_live_regs;
            wp::watchpoint_tracker tracker;
            bool next_reads_carry_flag(true);

            for(instruction in(ls.last()); in.is_valid(); in = prev_in) {
                prev_in = in.prev();

                memset(&tracker, 0, sizeof tracker);
                tracker.live_regs = next_live_regs;
                tracker.used_regs.kill_all();
                tracker.used_regs.revive(in);

                // track the carry flag. The carry flag will be used to detect
                // watched addresses.
                wp::track_carry_flag(tracker, in, next_reads_carry_flag);

                // compute live regs for next iteration based on this
                // instruction (before it is potentially modified, which would
                // corrupt the live reg set going forward).
                next_live_regs.visit(in);

                // Ignore two special purpose instructions which have memory-
                // like operands but don't actually touch memory.
                if(dynamorio::OP_lea == in.op_code()
                || dynamorio::OP_nop_modrm == in.op_code()
                || in.is_mangled()
                || in.is_cti()) {
                    continue;
                }

                // try to find memory operations.
                in.for_each_operand(wp::find_memory_operand, tracker);
                if(!tracker.num_ops) {
                    continue;
                }

                // Mangle the instruction. This makes it "safer" for use by
                // later generic watchpoint instrumentation.
                instruction old_in(in);
                in = wp::mangle(ls, in, tracker);
                if(in != old_in) {
                    tracker.num_ops = 0;
                    in.for_each_operand(wp::find_memory_operand, tracker);
                }

                IF_USER( instruction first(ls.insert_before(in, label_())); )
                IF_USER( instruction last(ls.insert_after(in, label_())); )

                // apply generic watchpoint instrumentation to the necessary
                // operands.
                wp::visit_operands(ls, in, tracker);

                // allow `Watcher` to add instrumentation within the existing
                // watchpoint instrumentation. This gives it access to an
                // operand (register) that contains the watched address being
                // read or written to.
                for(unsigned i(0); i < tracker.num_ops; ++i) {
                    const operand_ref &op(tracker.ops[i]);
                    if(SOURCE_OPERAND & op.kind) {
                        Watcher::visit_read(cpu, thread, bb, ls, tracker, i);
                    }

                    if(DEST_OPERAND & op.kind) {
                        Watcher::visit_write(cpu, thread, bb, ls, tracker, i);
                    }
                }

                IF_USER( wp::guard_redzone(ls, first, last); )
            }

            return policy_for<watchpoints<Watcher>>();
        }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
        /// Defers to the `Watcher` to decide how it will handle the interrupt.
        static granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::interrupt_stack_frame &isf,
            granary::interrupt_vector vector
        ) throw() {
            return Watcher::handle_interrupt(cpu, thread, bb, isf, vector);
        }
#endif

    };

}


#endif /* WATCHPOINT_INSTRUMENT_H_ */
