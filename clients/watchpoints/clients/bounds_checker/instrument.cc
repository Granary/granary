/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_policy.cc
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */


#include "clients/watchpoints/utils.h"
#include "clients/watchpoints/clients/bounds_checker/instrument.h"


using namespace granary;


namespace client { namespace wp {


#define DECLARE_BOUND_CHECKER(reg) \
    extern void CAT(granary_bounds_check_1_, reg)(void); \
    extern void CAT(granary_bounds_check_2_, reg)(void); \
    extern void CAT(granary_bounds_check_4_, reg)(void); \
    extern void CAT(granary_bounds_check_8_, reg)(void); \
    extern void CAT(granary_bounds_check_16_, reg)(void);


#define DECLARE_BOUND_CHECKERS(reg, rest) \
    DECLARE_BOUND_CHECKER(reg) \
    rest

    /// Register-specific bounds checker functions
    /// (defined in x86/bound_policy.asm).
    extern "C" {
        ALL_REGS(DECLARE_BOUND_CHECKERS, DECLARE_BOUND_CHECKER)
    }


#define BOUND_CHECKER_GROUP(reg) \
    { \
        &CAT(granary_bounds_check_1_, reg), \
        &CAT(granary_bounds_check_2_, reg), \
        &CAT(granary_bounds_check_4_, reg), \
        &CAT(granary_bounds_check_8_, reg), \
        &CAT(granary_bounds_check_16_, reg) \
    }


#define BOUND_CHECKER_GROUPS(reg, rest) \
    BOUND_CHECKER_GROUP(reg), \
    rest


    /// Register-specific (generated) functions to do bounds checking.
    typedef void (*bounds_checker_type)(void);
    static bounds_checker_type BOUNDS_CHECKERS[15][5] = {
        ALL_REGS(BOUND_CHECKER_GROUPS, BOUND_CHECKER_GROUP)
    };


    /// Configuration for bound descriptors.
    struct descriptor_allocator_config {
        enum {
            SLAB_SIZE = granary::PAGE_SIZE,
            EXECUTABLE = false,
            TRANSIENT = false,
            SHARED = true,
            SHARE_DEAD_SLABS = false,
            EXEC_WHERE = granary::EXEC_NONE,
            MIN_ALIGN = 4
        };
    };


    /// Allocator for the bound descriptors.
    static granary::static_data<
        granary::bump_pointer_allocator<descriptor_allocator_config>
    > DESCRIPTOR_ALLOCATOR;


    /// Initialise the descriptor allocator.
    STATIC_INITIALISE({
        DESCRIPTOR_ALLOCATOR.construct();
    })


    /// Pointers to the descriptors.
    ///
    /// Note: Note `static` so that we can access by the mangled name in
    ///       x86/bound_policy.asm.
    bound_descriptor *DESCRIPTORS[MAX_NUM_WATCHPOINTS] = {nullptr};


    /// Allocate a watchpoint descriptor and assign `desc` and `index`
    /// appropriately.
    bool bound_descriptor::allocate(
        bound_descriptor *&desc,
        uintptr_t &counter_index,
        const uintptr_t
    ) throw() {
        counter_index = 0;
        desc = nullptr;

        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        cpu_state_handle state;
        bound_descriptor *&free_list(state->free_list);
        if(free_list) {
            desc = free_list;
            if(bound_descriptor::FREE_LIST_END != desc->next_free_index) {
                free_list = DESCRIPTORS[desc->next_free_index];
            } else {
                free_list = nullptr;
            }
        }

        IF_KERNEL( granary_store_flags(flags); )

        // We got one from the free list.
        counter_index = 0;
        if(desc) {
            uintptr_t inherited_index_;
            destructure_combined_index(
                desc->my_index, counter_index, inherited_index_);

        // Try to allocate one.
        } else {
            counter_index = next_counter_index();
            if(counter_index > MAX_COUNTER_INDEX) {
                return false;
            }

            desc = DESCRIPTOR_ALLOCATOR->allocate<bound_descriptor>();
        }

        ASSERT(counter_index <= MAX_COUNTER_INDEX);

        return true;
    }


    /// Initialise a watchpoint descriptor.
    void bound_descriptor::init(
        bound_descriptor *desc,
        void *base_address,
        size_t size
    ) throw() {
        if(!is_valid_address(desc)) {
            return;
        }

        const uintptr_t base(reinterpret_cast<uintptr_t>(base_address));
        desc->lower_bound = static_cast<uint32_t>(base);
        desc->upper_bound = static_cast<uint32_t>(base + size);
    }


    /// Notify the bounds policy that the descriptor can be assigned to
    /// the index.
    void bound_descriptor::assign(
        bound_descriptor *desc,
        uintptr_t index
    ) throw() {
        if(!is_valid_address(desc)) {
            return;
        }
        ASSERT(index < MAX_NUM_WATCHPOINTS);
        desc->my_index = index;
        DESCRIPTORS[index] = desc;
    }


    /// Get the descriptor of a watchpoint based on its index.
    bound_descriptor *bound_descriptor::access(
        uintptr_t index
    ) throw() {
        ASSERT(index < MAX_NUM_WATCHPOINTS);
        return DESCRIPTORS[index];
    }


    /// Free a watchpoint descriptor by adding it to a free list.
    void bound_descriptor::free(
        bound_descriptor *desc,
        uintptr_t IF_TEST( index )
    ) throw() {
        if(!is_valid_address(desc)) {
            return;
        }
        ASSERT(index == desc->my_index);

        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        cpu_state_handle state;
        bound_descriptor *&free_list(state->free_list);

        if(free_list) {
            desc->next_free_index = free_list->my_index;
        } else {
            desc->next_free_index = bound_descriptor::FREE_LIST_END;
        }
        free_list = desc;

        IF_KERNEL( granary_store_flags(flags); )
    }


    void bound_policy::visit_read(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        const unsigned reg_index = register_to_index(tracker.regs[i].value.reg);
        const unsigned size_index = operand_size_order(tracker.sizes[i]);

        ASSERT(reg_index < 16);
        ASSERT(size_index < 5);

        instruction call(insert_cti_after(ls, tracker.labels[i],
                unsafe_cast<app_pc>(BOUNDS_CHECKERS[reg_index][size_index]),
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_CALL));
        call.set_mangled();
    }


    void bound_policy::visit_write(
        granary::basic_block_state &bb,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        if(!(SOURCE_OPERAND & tracker.ops[i].kind)) {
            visit_read(bb, ls, tracker, i);
        }
    }

    /// Visit a buffer overflow. This is invoked by
    void visit_overflow(
        uintptr_t watched_addr,
        app_pc addr_in_bb,
        unsigned size
    ) throw() {
        printf("Access of size %u to %p in basic block %p overflowed\n",
            size, watched_addr, addr_in_bb);
    }


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
    interrupt_handled_state bound_policy::handle_interrupt(
        cpu_state_handle,
        thread_state_handle,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector
    ) throw() {
        return INTERRUPT_DEFER;
    }
} /* wp namespace */
#else
} /* wp namespace */
#endif /* CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT */

} /* client namespace */
