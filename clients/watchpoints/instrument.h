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
#define WP_IGNORE_FRAME_POINTER 1

/// Enable if partial indexes should be used (bit [15, 20]).
#define WP_USE_PARTIAL_INDEX 1

namespace client {

    namespace wp {

        enum : uint64_t {
            /// Maximum number of memory operands in a single instruction.
            MAX_NUM_OPERANDS            = 2,

            /// Number of bits in an address.
            NUM_BITS_PER_ADDR           = 64,

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

            /// Number of partial index bits.
            NUM_PARTIAL_INDEX_BITS      = 5,

            /// Offset of partial index bits.
            PARTIAL_INDEX_OFFSET        = 15,

            /// Mask for extracting the partial index.
            PARTIAL_INDEX_SHIFT         = (
                NUM_BITS_PER_ADDR - NUM_PARTIAL_INDEX_BITS),
            PARTIAL_INDEX_MASK          = (
                ((~0ULL) << PARTIAL_INDEX_SHIFT) >> (
                    PARTIAL_INDEX_SHIFT - PARTIAL_INDEX_OFFSET
                )),

            /// Maximum number of watchpoints for the given number of high-order
            /// bits.
            MAX_NUM_WATCHPOINTS_        = ((1ULL << NUM_HIGH_ORDER_BITS) - 1),
#if WP_USE_PARTIAL_INDEX
            MAX_NUM_WATCHPOINTS         = (
                MAX_NUM_WATCHPOINTS_ * (1 << (NUM_PARTIAL_INDEX_BITS - 1))),
#else
            MAX_NUM_WATCHPOINTS         = MAX_NUM_WATCHPOINTS_,
#endif

            /// The bit offset, where LSB=0 and MSB=(NUM_BITS_PER_ADDR - 1), of
            /// the distinguishing bit of an un/watched address.
            DISTINGUISHING_BIT_OFFSET   = (
                NUM_BITS_PER_ADDR - NUM_HIGH_ORDER_BITS),

            /// Mask to clear the correct number of high-order bits.
            CLEAR_INDEX_MASK            = ~(
                (~0ULL) << (DISTINGUISHING_BIT_OFFSET - 1)),

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


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB


        /// The scale of registers that will be used to mask the tainted bits of a
        /// watched address.
        extern const granary::register_scale REG_SCALE;


        /// Keeps track of instruction-specific state for the watchpoint
        /// instrumentation.
        struct watchpoint_tracker {

            /// Current instruction
            granary::instruction in;

            /// Tracks which registers are live at the current instruction
            /// that we are instrumenting.
            granary::register_manager live_regs;
            granary::register_manager live_regs_after;

            /// Direct references to the operands within an instruction.
            granary::operand_ref ops[MAX_NUM_OPERANDS];

            /// Instruction labels where specific watchpoint implementations
            /// can add in their code. If no code is added before/after these
            /// labels, then the net effect is that the code will just "mask"
            /// out all watchpoint accesses to behave as normal.
            granary::instruction labels[MAX_NUM_OPERANDS];

            /// Conveniences for specific watchpoint implementations so that
            /// they can know where the watchpoint info is stored.
            granary::operand regs[MAX_NUM_OPERANDS];

            /// True iff the ith operand (in `ops`) can be modified, or if it
            /// must be left as-is.
            bool can_replace[MAX_NUM_OPERANDS];

            /// The number of operands that need to be instrumented.
            unsigned num_ops;

            /// Track the carry flag.
            bool restore_carry_flag_before;
            bool restore_carry_flag_after;
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


        /// Need to make sure that registers (that are not used in memory
        /// operands) but that are used as both sources and dests (e.g.
        /// RAX in CMPXCHG) are treated as live.
        void revive_matching_operand_regs(
            granary::register_manager &rm,
            granary::instruction in
        ) throw();


#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */


        /// Forward declaration of the template class that will allow watchpoint
        /// implementations to specify their descriptor types lazily.
        template <typename>
        struct descriptor_type;


        /// Returns true iff an address is watched.
        template <typename T>
        inline bool is_watched_address(T ptr_) throw() {
            const uintptr_t ptr(granary::unsafe_cast<uintptr_t>(ptr_));
#if GRANARY_IN_KERNEL
            return DISTINGUISHING_BIT_MASK == (ptr | DISTINGUISHING_BIT_MASK);
#else
            return DISTINGUISHING_BIT_MASK == (ptr & DISTINGUISHING_BIT_MASK);
#endif
        }


        /// Returns the unwatched version of an address, regardless of if it's
        /// watched.
        template <typename T>
        T unwatched_address(T ptr_) throw() {
            const uintptr_t ptr(granary::unsafe_cast<uintptr_t>(ptr_));
#if GRANARY_IN_KERNEL
            return granary::unsafe_cast<T>(ptr | CLEAR_INDEX_MASK);
#else
            return granary::unsafe_cast<T>(ptr & (~CLEAR_INDEX_MASK));
#endif
        }


#if WP_USE_PARTIAL_INDEX
        /// Return the partial index of an address.
        inline uintptr_t partial_index_of(uintptr_t ptr) throw() {
            return (ptr & PARTIAL_INDEX_MASK) >> PARTIAL_INDEX_OFFSET;
        }


        /// Combine a partial index with a counter index.
        inline uintptr_t combined_index(
            uintptr_t counter_index,
            uintptr_t partial_index
        ) throw() {
            return (counter_index << NUM_PARTIAL_INDEX_BITS) | partial_index;
        }


        /// Return the counter index component of a combined index.
        inline uintptr_t counter_index_of(uintptr_t index) throw() {
            return index >> NUM_PARTIAL_INDEX_BITS;
        }
#else
        /// Return the partial index of an address.
        inline uintptr_t partial_index_of(uintptr_t) throw() {
            return 0;
        }


        /// Combine a partial index with a counter index.
        inline uintptr_t combined_index(
            uintptr_t counter_index,
            uintptr_t
        ) throw() {
            return counter_index;
        }


        /// Return the counter index component of a combined index.
        inline uintptr_t counter_index_of(uintptr_t index) throw() {
            return index;
        }
#endif /* WP_USE_PARTIAL_INDEX */


        /// Return the next counter index.
        uintptr_t next_counter_index(void) throw();


        /// Return the index into the descriptor table for this watched address.
        ///
        /// Note: This assumes that the address is watched.
        template <typename T>
        inline uintptr_t
        index_of(T ptr_) throw() {
            const uintptr_t ptr(granary::unsafe_cast<uintptr_t>(ptr_));
            const uintptr_t counter_index(ptr >> DISTINGUISHING_BIT_OFFSET);
#if WP_USE_PARTIAL_INDEX
            return combined_index(counter_index, partial_index_of(ptr));
#else
            return counter_index;
#endif
        }


        /// Return a pointer to the descriptor for this watched address.
        ///
        /// Note: This assumes that the address is watched.
        template <typename T>
        inline typename descriptor_type<T>::type *
        descriptor_of(T ptr_) throw() {
            extern typename descriptor_type<T>::type *DESCRIPTORS[];
            return DESCRIPTORS[index_of(ptr_)];
        }


        /// Kill a watchpoint descriptor.
        ///
        /// Note: This assumes that the address is watched.
        ///
        /// Note: This does not remove the association in the descriptor table.
        template <typename T>
        void free_descriptor_of(T ptr_) throw() {
            typedef typename descriptor_type<T>::type desc_type;
            desc_type::free(descriptor_of(ptr_), index_of(ptr_));
        }


        /// State representing whether or not a watchpoint descriptor was
        /// allocated and its index added to an address.
        enum add_watchpoint_status {
            ADDRESS_ALREADY_WATCHED,
            ADDRESS_NOT_WATCHED,
            ADDRESS_WATCHED
        };


        /// Add a new watchpoint to an address.
        ///
        /// This is responsible for deferring to a higher-level watchpoint
        /// implementation for the allocation of watchpoints, but the insertion
        /// into the descriptor table is automatically handled by this function.
        template <typename T, typename... ConstructorArgs>
        add_watchpoint_status
        add_watchpoint(T &ptr_, ConstructorArgs... init_args) throw() {
            typedef typename descriptor_type<T>::type desc_type;
            extern desc_type *DESCRIPTORS[];

            if(is_watched_address(ptr_)) {
                return ADDRESS_ALREADY_WATCHED;
            }

            uintptr_t index(0);
            desc_type *desc(nullptr);

            if(!desc_type::allocate(desc, index)) {
                return ADDRESS_NOT_WATCHED;
            }

            desc_type::init(desc, init_args...);
            DESCRIPTORS[index] = desc;

            index <<= 1;
            index |= DISTINGUISHING_BIT;
            index <<= (DISTINGUISHING_BIT_OFFSET - 1);

            const uintptr_t ptr(granary::unsafe_cast<uintptr_t>(ptr_));

            ptr_ = granary::unsafe_cast<T>(index | (ptr & CLEAR_INDEX_MASK));
            return ADDRESS_WATCHED;
        }
    }

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB

    /// Generalised behavioural watchpoints instrumentation. A `Watcher` type is
    /// passed, which generates instrumentation code when watched memory
    /// operations on detected.
    template <typename Watcher>
    struct watchpoints : public granary::instrumentation_policy {
    public:


        /// Instrument a basic block.
        static granary::instrumentation_policy visit_basic_block(
            granary::cpu_state_handle &,
            granary::thread_state_handle &,
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
                tracker.live_regs_after = next_live_regs;

#define NARROW 0

#if NARROW
                // TODO: remove me
                register_manager old_next_live_regs(next_live_regs);
                (void) old_next_live_regs;
#endif

                // Special case for XLAT; to expose the "full" watched address
                // to higher-level instrumentation, we need to compute the full
                // address, but doing so risks clobbering RBX or RAX if either
                // is dead (RAX is guaranteed dead).
                if(dynamorio::OP_xlat == in.op_code()) {
                    tracker.live_regs.revive(dynamorio::DR_REG_RBX);
                    tracker.live_regs.revive(dynamorio::DR_REG_RAX);

                // Kill the destination regs of this instruction so that we can
                // can take advantage of those.
                //
                // Note: this operates in the reverse order as
                //       `register_manager::visit` so that we can make sure we
                //       don't clobber source registers while still being able
                //       to use destination registers.
                } else {
                    tracker.live_regs.visit_sources(in);
                    tracker.live_regs.visit_dests(in);
                }

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

                wp::revive_matching_operand_regs(tracker.live_regs, in);

                // try to find memory operations.
                tracker.in = in;
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
                    tracker.in = in;
                    in.for_each_operand(wp::find_memory_operand, tracker);
                }


#if NARROW
                // narrow down step 1:
                /*
                bool skip(false);
                for(unsigned i(0); i < tracker.num_ops; ++i) {
                    const operand_ref &op(tracker.ops[i]);
                    if((SOURCE_OPERAND & op.kind)) {
                        skip = true;
                    }
                }
                if(skip) {
                    continue;
                }
                */
                // TODO: remove this after fixing REP MOVS issue.
                // narrow step 2:
                if(tracker.num_ops == 1) {
                    continue;
                }
#if 0
                if(56 != in.op_code()) {
                    continue;
                }

                // narrow step 3:
                const operand_ref &op2(tracker.ops[0]);
                (void) op2;

                if(!op2->value.base_disp.base_reg) {
                    continue;
                }

                // narrow step 4:
                if(!op2->value.base_disp.index_reg) {
                    continue;
                }

                // narrow step 5:
                if(!op2->value.base_disp.disp) {
                    continue;
                }

                (void) old_next_live_regs;

                if(!tracker.live_regs.is_live(op2->value.base_disp.base_reg)) {
                    continue;
                }

                if(!old_next_live_regs.is_live(op2->value.base_disp.base_reg)) {
                    continue;
                }

                if(!in.pc()) {
                    continue;
                }
                /*
                if(dynamorio::OP_mov_st == in.op_code()) {
                    continue;
                }
                */

                /*
                if(55 == in.op_code()) {
                    continue;
                }

                if(14 == in.op_code()) {
                    continue;
                }

                if(190 == in.op_code()) {
                    continue;
                }

                if(16 == in.op_code()) {
                    continue;
                }

                if(10 == in.op_code()) {
                    continue;
                }
                */

                // 195 - potentially solved
                /*
                if(in != old_in) {
                    continue;
                }*/
#endif
                // only one operand
                // memory operand is a source
                // has a base reg
                // no index reg
                // no displacement
                // base reg is killed by the instruction
                // base reg is live after the instruction
                // instruction wasn't pre-mangled
                // not a mangled PUSH/POP
                // not a MOV
                printf("%p %u\n", in.pc(), in.op_code());
#endif
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
                        Watcher::visit_read(bb, ls, tracker, i);
                    }

                    if(DEST_OPERAND & op.kind) {
                        Watcher::visit_write(bb, ls, tracker, i);
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

#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */

}


#endif /* WATCHPOINT_INSTRUMENT_H_ */
