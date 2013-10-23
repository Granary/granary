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

#include "clients/watchpoints/config.h"


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
            NUM_HIGH_ORDER_BITS         = WP_COUNTER_INDEX_WIDTH,

            /// Number of inherited index bits.
            NUM_INHERITED_INDEX_BITS      = WP_INHERITED_INDEX_WIDTH,

            /// Offset of inherited index bits.
            INHERITED_INDEX_OFFSET        = WP_INHERITED_INDEX_GRANULARITY,

            /// Mask for extracting the inherited index.
#if WP_USE_INHERITED_INDEX
            INHERITED_INDEX_MASK          = (
                (((~0ULL)
                    << (NUM_BITS_PER_ADDR - NUM_INHERITED_INDEX_BITS))
                    >> (NUM_BITS_PER_ADDR - NUM_INHERITED_INDEX_BITS
                                          - INHERITED_INDEX_OFFSET))),
#else
            INHERITED_INDEX_MASK          = (1 << NUM_INHERITED_INDEX_BITS) - 1,
#endif

            /// Maximum counter index (inclusive).
            MAX_COUNTER_INDEX           = 0x7FFFU,

            /// Maximum number of watchpoints for the given number of high-order
            /// bits.
#if WP_USE_INHERITED_INDEX
            MAX_NUM_WATCHPOINTS         = (
                (MAX_COUNTER_INDEX + 1) * (1 << NUM_INHERITED_INDEX_BITS)),
#else
            MAX_NUM_WATCHPOINTS         = MAX_COUNTER_INDEX + 1,
#endif

            /// The bit offset, where LSB=0 and MSB=(NUM_BITS_PER_ADDR - 1), of
            /// the distinguishing bit of an un/watched address.
            DISTINGUISHING_BIT_OFFSET   = (
                NUM_BITS_PER_ADDR - NUM_HIGH_ORDER_BITS),

            /// Mask to clear the correct number of high-order bits.
            CLEAR_INDEX_MASK            = ~(
                (~0ULL) << DISTINGUISHING_BIT_OFFSET),

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
            || 16 == NUM_HIGH_ORDER_BITS),
            "The number of high order bits for a watched address descriptor "
            "index must be either 8 or 16, as sub-8/16 writes cannot be made "
            "to registers without clobbering flags.");


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB


        /// The memory operand size in bytes.
        enum operand_size {
            OP_SIZE_1   = 1,  // 8 bit
            OP_SIZE_2   = 2,  // 16 bit
            OP_SIZE_4   = 4,  // 32 bit
            OP_SIZE_8   = 8,  // 64 bit
            OP_SIZE_16  = 16  // xmm (128 bit)
        };


        /// Keeps track of instruction-specific state for the watchpoint
        /// instrumentation.
        struct watchpoint_tracker {

            /// Current instruction
            granary::instruction in;
            granary::instrumentation_policy policy;


            /// True if this instruction reads from or writes to RSP.
            bool reads_from_rsp;
            bool writes_to_rsp;


            /// Represents the set of instructions that we can legally save and
            /// restore around the instrumented instruction without breaking
            /// things.
            granary::register_manager live_regs;


            /// Represents the set of registers that have already been spilled.
            granary::register_manager spill_regs;


            /// Represents the set of registers that are actually live after
            /// whatever instruction we're currently instrumenting.
            granary::register_manager live_regs_after;


            /// Direct references to the operands within an instruction.
            granary::operand_ref ops[MAX_NUM_OPERANDS];


            /// Instruction labels where specific watchpoint implementations
            /// can add in their code. If no code is added before/after these
            /// labels, then the net effect is that the code will just "mask"
            /// out all watchpoint accesses to behave as normal.
            granary::instruction pre_labels[MAX_NUM_OPERANDS];
            granary::instruction labels[MAX_NUM_OPERANDS];


            /// Conveniences for specific watchpoint implementations so that
            /// they can know where the watchpoint info is stored.
            granary::operand regs[MAX_NUM_OPERANDS];


            /// True iff the ith operand (in `ops`) can be modified, or if it
            /// must be left as-is.
            bool can_replace[MAX_NUM_OPERANDS];


            /// The operand sizes of each of the memory operands.
            operand_size sizes[MAX_NUM_OPERANDS];


            /// Kernel space:
            ///     Applies if we detect a specific binary pattern, or if we
            ///     have auto-detection of user space addresses enabled. Here,
            ///     if bit 47 is 0 then we assume it's a user space address.
            ///
            /// User space:
            ///     Applied when we have %FS- and %GS-segmented memory operands.
            ///     We assume that indexes with bit 47 set to 1 are unwatched.
            bool check_bit_47;


            /// The number of operands that need to be instrumented.
            unsigned num_ops;


            /// Track the carry flag.
            bool restore_carry_flag_before;
            bool restore_carry_flag_after;


            /// Replace/update operands around the memory instruction. This will
            /// update the `labels` field of the `operand_tracker` with labels in
            /// instruction stream so that a `Watcher` can inject its own specific
            /// instrumentation in at those well-defined points. This will also
            /// update the `sources` and `dests` field appropriately so that the
            /// `Watcher`s read/write visitors can access operands containing the
            /// watched addresses.
            void visit_operands(granary::instruction_list &) throw();


            /// Perform watchpoint-specific mangling of an instruction.
            bool mangle(granary::instruction_list &) throw();


            /// Small state machine to track whether or not we can clobber the carry
            /// flag. The carry flag is relevant because we use the BT instruction to
            /// determine if the address is a watched address.
            void track_carry_flag(bool &) throw();


            /// Save the carry flag, if needed. We use the carry flag extensively. For
            /// example, the BT instruction is used to both detect (and thus clobber
            /// CF) watchpoints, as well as to restore the carry flag, by testing the
            /// bit set with `SETcf` (`SETcc` variant).
            dynamorio::reg_id_t save_carry_flag(
                granary::instruction_list &ls,
                granary::instruction before,
                bool &spilled_carry_flag
            ) throw();


            /// Post-process that instrumented instructions. This looks for
            /// minor peephole optimisation opportunities.
            void post_process_instructions(granary::instruction_list &ls);
        };


        /// Find memory operands that might need to be checked for watchpoints.
        /// If one is found, then num_ops is incremented, and the operand
        /// reference is stored in the passed array.
        void find_memory_operand(
            const granary::operand_ref &,
            watchpoint_tracker &
        ) throw();


        /// Do register stealing coalescing by recognising sequences of instructions
        /// than can benefit from shared spilled registers, and spill/restore
        /// registers around those regions.
        void region_register_spiller(
            watchpoint_tracker &tracker,
            granary::instruction_list &ls
        ) throw();


#   if !GRANARY_IN_KERNEL
        /// Add in a user space redzone guard if necessary. This looks for a PUSH
        /// instruction anywhere between `first` and `last` and if it finds one then
        /// it guards the entire instrumented block with a redzone shift.
        void guard_redzone(
            granary::instruction_list &ls,
            granary::instruction first,
            granary::instruction last
        ) throw();
#   endif /* GRANARY_IN_KERNEL */


#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */


        /// Forward declaration of the template class that will allow watchpoint
        /// implementations to specify their descriptor types lazily.
        template <typename>
        struct descriptor_type;


        /// Returns true iff an address is watched.
        inline bool is_watched_address(uintptr_t ptr) throw() {
#if GRANARY_IN_KERNEL
            enum {
                MASK_47_48 = (USER_OR_MASK >> 1) | USER_OR_MASK
            };
            const uintptr_t masked_ptr(ptr & MASK_47_48);
            return masked_ptr && MASK_47_48 != masked_ptr;
#else
            return ptr && DISTINGUISHING_BIT_MASK == (ptr & DISTINGUISHING_BIT_MASK);
#endif
        }
        template <typename T>
        inline bool is_watched_address(T *ptr_) throw() {
            return is_watched_address(reinterpret_cast<uintptr_t>(ptr_));
        }


        /// Returns the unwatched version of an address, regardless of if it's
        /// watched.
        inline uintptr_t unwatched_address(uintptr_t ptr) throw() {
#if GRANARY_IN_KERNEL
            return ptr | (~CLEAR_INDEX_MASK);
#else
            return ptr & CLEAR_INDEX_MASK;
#endif
        }
        template <typename T>
        inline T *unwatched_address(T *ptr_) throw() {
            return reinterpret_cast<T *>(
                unwatched_address(
                    reinterpret_cast<uintptr_t>(ptr_)));
        }


        /// Returns the unwatched version of an address.
        inline uintptr_t unwatched_address_check(uintptr_t ptr) throw() {
            if(is_watched_address(ptr)) {
                return unwatched_address(ptr);
            }
            return ptr;
        }
        template <typename T>
        inline T *unwatched_address_check(T *ptr_) throw() {
            return reinterpret_cast<T *>(
                unwatched_address_check(
                    reinterpret_cast<uintptr_t>(ptr_)));
        }


#if WP_USE_INHERITED_INDEX
        /// Return the inherited index of an address.
        inline uintptr_t inherited_index_of(uintptr_t ptr) throw() {
            return (ptr & INHERITED_INDEX_MASK) >> INHERITED_INDEX_OFFSET;
        }


        /// Combine a inherited index with a counter index.
        inline uintptr_t combined_index(
            uintptr_t counter_index,
            uintptr_t inherited_index
        ) throw() {
            return (counter_index << NUM_INHERITED_INDEX_BITS) | inherited_index;
        }
#else
        /// Return the inherited index of an address.
        inline uintptr_t inherited_index_of(uintptr_t) throw() {
            return 0;
        }


        /// Combine a inherited index with a counter index.
        inline uintptr_t combined_index(
            uintptr_t counter_index,
            uintptr_t
        ) throw() {
            return counter_index;
        }

#endif /* WP_USE_INHERITED_INDEX */

        /// Return the counter index component of a combined index.
        inline uintptr_t counter_index_of(uintptr_t ptr) throw() {
            return ptr >> (DISTINGUISHING_BIT_OFFSET + 1);
        }


        /// Return the inherited index of an address.
        template <typename T>
        inline uintptr_t inherited_index_of(T *addr) throw() {
            return inherited_index_of(reinterpret_cast<uintptr_t>(addr));
        }


        /// Return the inherited index of an address.
        template <typename T>
        inline uintptr_t counter_index_of(T *addr) throw() {
            return counter_index_of(reinterpret_cast<uintptr_t>(addr));
        }


        /// Return the index into the descriptor table for this watched address.
        ///
        /// Note: This assumes that the address is watched.
        inline uintptr_t combined_index_of(uintptr_t ptr) throw() {
            return combined_index(
                counter_index_of(ptr),
                inherited_index_of(ptr)
            );
        }


        /// Return the index into the descriptor table for this watched address.
        ///
        /// Note: This assumes that the address is watched.
        template <typename T>
        inline uintptr_t combined_index_of(T *ptr_) throw() {
            return combined_index_of(granary::unsafe_cast<uintptr_t>(ptr_));
        }


        /// Return the next counter index.
        uintptr_t next_counter_index(void) throw();


        /// Destructure a combined index into its counter and inherited indexes.
        inline void destructure_combined_index(
            const uintptr_t index,
            uintptr_t &counter_index,
            uintptr_t &inherited_index
        ) throw() {
#if WP_USE_INHERITED_INDEX
            counter_index = index >> NUM_INHERITED_INDEX_BITS;
            inherited_index = index & INHERITED_INDEX_MASK;
#else
            counter_index = index;
            inherited_index = 0U;
#endif
        }


        /// Return a pointer to the descriptor for this watched address.
        ///
        /// Note: This assumes that the address is watched.
        template <typename T>
        inline typename descriptor_type<T>::type *
        descriptor_of(T ptr_) throw() {
            typedef typename descriptor_type<T>::type desc_type;
            return desc_type::access(combined_index_of(ptr_));
        }


        /// Kill a watchpoint descriptor.
        ///
        /// Note: This assumes that the address is watched.
        ///
        /// Note: This does not remove the association in the descriptor table.
        template <typename T>
        void free_descriptor_of(T ptr_) throw() {
            typedef typename descriptor_type<T>::type desc_type;
            const uintptr_t index(combined_index_of(ptr_));
            desc_type::free(desc_type::access(index), index);
        }


        /// State representing whether or not a watchpoint descriptor was
        /// allocated and its index added to an address.
        enum add_watchpoint_status {
            ADDRESS_ALREADY_WATCHED,
            ADDRESS_NOT_WATCHED,
            ADDRESS_WATCHED,
            ADDRESS_TAINTED
        };


        /// Add a watchpoint to an address.
        ///
        /// This taints the address, but does nothing else.
        template <typename T>
        add_watchpoint_status add_watchpoint(T &ptr_) throw() {
            static_assert(
                std::is_pointer<T>::value || std::is_integral<T>::value,
                "`add_watchpoint` expects an integral/pointer operand.");

            uintptr_t ptr(granary::unsafe_cast<uintptr_t>(ptr_));
#if GRANARY_IN_KERNEL
            ptr &= DISTINGUISHING_BIT_MASK;
#else
            ptr |= DISTINGUISHING_BIT_MASK;
#endif

            ptr_ = granary::unsafe_cast<T>(ptr);
            return ADDRESS_TAINTED;
        }


        /// Alllocate and initialise a watched address.
        template <typename desc_type, bool>
        struct allocate_and_init_watched_address {
            template <typename... InitArgs>
            static add_watchpoint_status apply(
                desc_type *&desc,
                uintptr_t &counter_index,
                uintptr_t inherited_index,
                InitArgs... init_args
            ) throw() {
                // Allocate descriptor.
                if(!desc_type::allocate_and_init(desc,
                                                 counter_index,
                                                 inherited_index,
                                                 init_args...)) {
                    return ADDRESS_NOT_WATCHED;
                }

                if(desc) {
                    const uintptr_t index(
                        combined_index(counter_index, inherited_index));
                    desc_type::assign(desc, index);
                }

                ASSERT(counter_index <= MAX_COUNTER_INDEX);

                return ADDRESS_WATCHED;
            }
        };


        template <typename DescType>
        struct allocate_and_init_watched_address<DescType, false> {

            template <typename... InitArgs>
            static add_watchpoint_status apply(
                DescType *&desc,
                uintptr_t &counter_index,
                uintptr_t inherited_index,
                InitArgs... init_args
            ) throw() {

                // Allocate descriptor.
                if(!DescType::allocate(desc, counter_index, inherited_index)) {
                    return ADDRESS_NOT_WATCHED;
                }

                ASSERT(counter_index <= MAX_COUNTER_INDEX);

                // Construct the descriptor and add it to the table.
                if(desc) {
                    const uintptr_t index(
                        combined_index(counter_index, inherited_index));

                    DescType::init(desc, init_args...);
                    DescType::assign(desc, index);
                }

                return ADDRESS_WATCHED;
            }
        };


        /// Add a new watchpoint to an address.
        ///
        /// This is responsible for deferring to a higher-level watchpoint
        /// implementation for the allocation of watchpoints, but the insertion
        /// into the descriptor table is automatically handled by this function.
        template <typename T, typename... ConstructorArgs>
        add_watchpoint_status
        add_watchpoint(T &ptr_, ConstructorArgs... init_args) throw() {
            static_assert(
                std::is_pointer<T>::value || std::is_integral<T>::value,
                "`add_watchpoint` expects an integral/pointer operand.");

            typedef descriptor_type<T> container;
            typedef typename container::type desc_type;

            uintptr_t ptr(granary::unsafe_cast<uintptr_t>(ptr_));

            enum {
                ALLOC_AND_INIT = container::ALLOC_AND_INIT,
                REINIT_WATCHED_POINTERS = container::REINIT_WATCHED_POINTERS,
            };

            if(is_watched_address(ptr)) {
                if(!REINIT_WATCHED_POINTERS) {
                    return ADDRESS_ALREADY_WATCHED;
                }
            } else if(!granary::is_valid_address(ptr)) {
                return ADDRESS_NOT_WATCHED;
            }

            uintptr_t counter_index(0);
            uintptr_t inherited_index(inherited_index_of(ptr));
            desc_type *desc(nullptr);

            const add_watchpoint_status status(allocate_and_init_watched_address<
                desc_type,
                ALLOC_AND_INIT
            >::apply(desc, counter_index, inherited_index, init_args...));

            if(ADDRESS_NOT_WATCHED == status) {
                return status;
            }

            // Taint the pointer.
            counter_index &= MAX_COUNTER_INDEX;
            counter_index <<= 1;
            counter_index |= DISTINGUISHING_BIT;
            counter_index <<= DISTINGUISHING_BIT_OFFSET;

            ptr &= CLEAR_INDEX_MASK;
            ptr |= counter_index;

            ASSERT(is_watched_address(ptr));

            ptr_ = granary::unsafe_cast<T>(ptr);

            return status;
        }
    }

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB

    /// Generalised behavioural watchpoints instrumentation. A `Watcher` type is
    /// passed, which generates instrumentation code when watched memory
    /// operations on detected.
    template <typename AppWatcher, typename HostWatcher>
    struct watchpoints : public granary::instrumentation_policy {
    public:


        typedef watchpoints<AppWatcher, HostWatcher> self_type;


        enum {
            AUTO_INSTRUMENT_HOST = AppWatcher::AUTO_INSTRUMENT_HOST
        };

        /// Instrument a basic block.
        template <typename Watcher>
        granary::instrumentation_policy visit_instructions(
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {

            using namespace granary;

            instruction prev_in;
            register_manager next_live_regs;
            wp::watchpoint_tracker tracker;
            bool next_reads_carry_flag(true);

            IF_KERNEL( const bool in_user_access_zone(
                WP_CHECK_FOR_USER_ADDRESS || accesses_user_data()); )

#if WP_ENABLE_REGISTER_REGIONS
            region_register_spiller(tracker, ls);
#endif /* WP_ENABLE_REGISTER_REGIONS */

            // Backward pass.
            for(instruction in(ls.last()); in.is_valid(); in = prev_in) {
                prev_in = in.prev();

                memset(&tracker, 0, sizeof tracker);
                tracker.in = in;
                tracker.policy = *this;
                tracker.live_regs = next_live_regs;
                tracker.live_regs_after = next_live_regs;

                // Makes it so that we can't overwrite any registers used in
                // the instruction.
                tracker.live_regs.revive(in);

                // Track the carry flag. The carry flag will be used to detect
                // watched addresses.
                tracker.track_carry_flag(next_reads_carry_flag);

                // Compute live regs for next iteration based on this
                // instruction (before it is potentially modified, which would
                // corrupt the live reg set going forward).
                next_live_regs.visit(in);

                // Ignore two special purpose instructions which have memory-
                // like operands but don't actually touch memory.
                if(dynamorio::OP_lea == in.op_code()
                || dynamorio::OP_nop_modrm == in.op_code()
                || dynamorio::OP_leave == in.op_code()
                || dynamorio::OP_enter == in.op_code()
                || in.is_mangled()
                || in.is_cti()) {
                //|| in.op_code() < 396) {
                    continue;
                }

                IF_KERNEL( tracker.check_bit_47 = in_user_access_zone; )

                // Try to find memory operations.
                in.for_each_operand(wp::find_memory_operand, tracker);
                if(!tracker.num_ops) {
                    continue;
                }

                // Valid for user-space TLS accesses, and kernel-space CPU-
                // private data accesses.
                if(1 == tracker.num_ops) {
                    const operand_ref &only_op(tracker.ops[0]);
                    if(dynamorio::DR_SEG_GS == only_op->seg.segment
                    || dynamorio::DR_SEG_FS == only_op->seg.segment) {
                        tracker.check_bit_47 = true;
                        continue;
                    }
                }

                // Before we do any mangling (which might spill registers), go
                // and figure out what registers can never be spilled.
                tracker.spill_regs.kill_all();
                tracker.spill_regs.revive(in);

                // Mangle the instruction. This makes it "safer" for use by
                // later generic watchpoint instrumentation.
                if(tracker.mangle(ls)) {
                    tracker.num_ops = 0;
                    tracker.reads_from_rsp = false;
                    tracker.writes_to_rsp = false;
                    in = tracker.in;
                    in.for_each_operand(wp::find_memory_operand, tracker);
                }

                IF_USER( instruction first(ls.insert_before(in, label_())); )
                IF_USER( instruction last(ls.insert_after(in, label_())); )

                // Apply generic watchpoint instrumentation to the necessary
                // operands.
                tracker.visit_operands(ls);

                // Allow `Watcher` to add instrumentation within the existing
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

            // Apply any minor peephole optimisations to get rid of redundant
            // instructions.
            tracker.post_process_instructions(ls);

            return policy_for<self_type>();
        }


        /// Visit app instructions (module, user program)
        granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            return visit_instructions<AppWatcher>(bb, ls);
        }


        /// Visit host instructions (module, user program)
        granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
#if !WP_TRANSITIVE_INSTRUMENT_HOST && !CONFIG_INSTRUMENT_HOST
            using namespace granary;

            // Force Granary to detach on exiting each basic block.
            granary::instruction in(ls.first());
            for(; in.is_valid(); in = in.next()) {
                if(!in.is_cti()) {
                    continue;
                }

                operand target(in.cti_target());

                // Assume that indirect control-flow can go into module
                // wrappers.
                if(!dynamorio::opnd_is_pc(target)) {
                    continue;
                }

                // Make sure that we wrap things that should be wrapped.
                if(find_detach_target(target.value.pc, RUNNING_AS_HOST)) {
                    continue;
                }

                // Make it so that the current function is instrumented, but not
                // called functions.
                if(in.is_call()) {
                    in.set_mangled();
                }
            }
#endif
            return visit_instructions<HostWatcher>(bb, ls);
        }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
        /// Defers to the `Watcher` to decide how it will handle the interrupt.
        granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle cpu,
            granary::thread_state_handle thread,
            granary::basic_block_state &bb,
            granary::interrupt_stack_frame &isf,
            granary::interrupt_vector vector
        ) throw() {
            return AppWatcher::handle_interrupt(cpu, thread, bb, isf, vector);
        }
#endif
    };

#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */

} /* namespace client */


/// Used to declare a simple read/write instrumentation policy.
#define DECLARE_READ_WRITE_POLICY(name, auto_instrument) \
    namespace wp { \
        struct name { \
            \
            enum { \
                AUTO_INSTRUMENT_HOST = auto_instrument \
            }; \
            \
            static void visit_read( \
                granary::basic_block_state &bb, \
                granary::instruction_list &ls, \
                watchpoint_tracker &tracker, \
                unsigned i \
            ) throw(); \
            \
            static void visit_write( \
                granary::basic_block_state &bb, \
                granary::instruction_list &ls, \
                watchpoint_tracker &tracker, \
                unsigned i \
            ) throw(); \
            \
            IF_CONFIG_CLIENT_HANDLE_INTERRUPT( \
            static granary::interrupt_handled_state handle_interrupt( \
                granary::cpu_state_handle cpu, \
                granary::thread_state_handle thread, \
                granary::basic_block_state &bb, \
                granary::interrupt_stack_frame &isf, \
                granary::interrupt_vector vector \
            ) throw(); ) \
        }; \
    }


#define DEFINE_READ_VISITOR(rw_policy_name, ...) \
    namespace wp { \
        void rw_policy_name::visit_read( \
            granary::basic_block_state &bb, \
            instruction_list &ls, \
            watchpoint_tracker &tracker, \
            unsigned i \
        ) throw() { \
            const operand &op(tracker.regs[i]); \
            const instruction &label(tracker.labels[i]); \
            const instruction &pre_label(tracker.pre_labels[i]); \
            (void) bb; (void) ls; (void) op; (void) label; (void) pre_label; \
            __VA_ARGS__ \
        } \
    }


#define DEFINE_WRITE_VISITOR(rw_policy_name, ...) \
    namespace wp { \
        void rw_policy_name::visit_write( \
            granary::basic_block_state &bb, \
            instruction_list &ls, \
            watchpoint_tracker &tracker, \
            unsigned i \
        ) throw() { \
            const operand &op(tracker.regs[i]); \
            const instruction &label(tracker.labels[i]); \
            const instruction &pre_label(tracker.pre_labels[i]); \
            (void) bb; (void) ls; (void) op; (void) label; (void) pre_label; \
            __VA_ARGS__ \
        } \
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
#   define DEFINE_INTERRUPT_VISITOR(rw_policy_name, ...) \
        namespace wp { \
            interrupt_handled_state rw_policy_name::handle_interrupt( \
                cpu_state_handle cpu, \
                thread_state_handle thread, \
                granary::basic_block_state &bb, \
                interrupt_stack_frame &isf, \
                interrupt_vector vector \
            ) throw() { \
                (void) cpu; (void) thread; (void) bb; (void) isf; (void) vector; \
                { __VA_ARGS__ } \
                return INTERRUPT_DEFER; \
            } \
        }
#else
#   define DEFINE_INTERRUPT_VISITOR(rw_policy_name, ...)
#endif




/// Used to declare a simple watchpoints policy based on a host and app
/// read/write policy.
#define DECLARE_INSTRUMENTATION_POLICY(policy_name, read_policy_name, write_policy_name, ...) \
    struct policy_name \
        : public watchpoints<wp::read_policy_name, wp::write_policy_name> \
    __VA_ARGS__ ;

#endif /* WATCHPOINT_INSTRUMENT_H_ */
