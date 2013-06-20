/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_policy.cc
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#include "clients/watchpoints/policies/bound_policy.h"

using namespace granary;

/// Should we add any watchpoints?
#define ENABLE_WATCHPOINTS 1

/// Should we enable descriptors?
#define ENABLE_DESCRIPTORS 1

/// Should we add in a call to the bounds checker?
#define ENABLE_INSTRUMENTATION 1

/// Should we try to store descriptors on a free list?
#define ENABLE_FREE_LIST 1


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


    /// Register-specific (generated) functions to do bounds checking.
    static unsigned REG_TO_INDEX[] = {
        ~0U,    // null
        0,      // rax
        1,      // rcx
        2,      // rdx
        3,      // rbx
        ~0U,    // rsp
        4,      // rbp
        5,      // rsi
        6,      // rdi
        7,      // r8
        8,      // r9
        9,      // r10
        10,     // r11
        11,     // r12
        12,     // r13
        13,     // r14
        14      // r15
    };


    /// Size to index.
    static unsigned SIZE_TO_INDEX[] = {
        ~0U,
        0,      // 1
        1,      // 2
        ~3U,
        2,      // 4
        ~5U,
        ~6U,
        ~7U,
        3,      // 8
        ~8U,
        ~9U,
        ~10U,
        ~11U,
        ~12U,
        ~13U,
        ~14U,
        4       // 16
    };


#if ENABLE_DESCRIPTORS
    /// Configuration for bound descriptors.
    struct descriptor_allocator_config {
        enum {
            SLAB_SIZE = granary::PAGE_SIZE,
            EXECUTABLE = false,
            TRANSIENT = false,
            SHARED = true,
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

#else

    bound_descriptor *DESCRIPTORS[1] = {nullptr};

#endif /* ENABLE_DESCRIPTORS */

    /// Allocate a watchpoint descriptor and assign `desc` and `index`
    /// appropriately.
    bool bound_descriptor::allocate(
        bound_descriptor *&desc,
        uintptr_t &counter_index,
        const uintptr_t
    ) throw() {
        counter_index = 0;
        desc = nullptr;

#if !ENABLE_WATCHPOINTS
        return false;
#endif /* ENABLE_WATCHPOINTS */

#if ENABLE_FREE_LIST && ENABLE_DESCRIPTORS
        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        IF_KERNEL( cpu_state_handle state; )
        IF_USER( thread_state_handle state; )

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

#endif /* ENABLE_FREE_LIST */

#if ENABLE_DESCRIPTORS
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
#else
        counter_index = 0;
        UNUSED(DESCRIPTORS);

#endif /* ENABLE_DESCRIPTORS */

        return true;
    }


    /// Initialise a watchpoint descriptor.
    void bound_descriptor::init(
        bound_descriptor *desc,
        void *base_address,
        size_t size
    ) throw() {
#if ENABLE_DESCRIPTORS
        const uintptr_t base(reinterpret_cast<uintptr_t>(base_address));
        desc->lower_bound = static_cast<uint32_t>(base);
        desc->upper_bound = static_cast<uint32_t>(base + size);
#else
        UNUSED(desc);
        UNUSED(base_address);
        UNUSED(size);
#endif /* ENABLE_DESCRIPTORS */
    }


    /// Notify the bounds policy that the descriptor can be assigned to
    /// the index.
    void bound_descriptor::assign(
        bound_descriptor *desc,
        uintptr_t index
    ) throw() {
#if ENABLE_DESCRIPTORS
        ASSERT(index < MAX_NUM_WATCHPOINTS);
        desc->my_index = index;
        DESCRIPTORS[index] = desc;
#else
        UNUSED(desc);
        UNUSED(index);
#endif /* ENABLE_DESCRIPTORS */
    }


    /// Get the descriptor of a watchpoint based on its index.
    bound_descriptor *bound_descriptor::access(
        uintptr_t index
    ) throw() {
#if ENABLE_DESCRIPTORS
        ASSERT(index < MAX_NUM_WATCHPOINTS);
        return DESCRIPTORS[index];
#else
        UNUSED(index);
        return nullptr;
#endif /* ENABLE_DESCRIPTORS */
    }


    /// Free a watchpoint descriptor by adding it to a free list.
    void bound_descriptor::free(
        bound_descriptor *desc,
        uintptr_t index
    ) throw() {
        if(!is_valid_address(desc)) {
            return;
        }
#if ENABLE_DESCRIPTORS
        ASSERT(index == desc->my_index);

#   if ENABLE_FREE_LIST
        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        IF_KERNEL( cpu_state_handle state; )
        IF_USER( thread_state_handle state; )

        bound_descriptor *&free_list(state->free_list);

        if(free_list) {
            desc->next_free_index = free_list->my_index;
        } else {
            desc->next_free_index = bound_descriptor::FREE_LIST_END;
        }
        free_list = desc;

        IF_KERNEL( granary_store_flags(flags); )
#   endif /* ENABLE_FREE_LIST */
#else
        UNUSED(index);
#endif /* ENABLE_DESCRIPTORS */
    }


    void app_bound_policy::visit_read(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
#if ENABLE_INSTRUMENTATION
        const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
        const unsigned size_index(SIZE_TO_INDEX[tracker.sizes[i]]);

        ASSERT(reg_index < 16);
        ASSERT(size_index < 5);

        instruction call(insert_cti_after(ls, tracker.labels[i],
            unsafe_cast<app_pc>(BOUNDS_CHECKERS[reg_index][size_index]),
            false, operand(),
            CTI_CALL));
        call.set_mangled();
#else
        UNUSED(ls);
        UNUSED(tracker);
        UNUSED(i);
        UNUSED(BOUNDS_CHECKERS);
        UNUSED(REG_TO_INDEX);
        UNUSED(SIZE_TO_INDEX);
#endif /* ENABLE_INSTRUMENTATION */
    }


    void app_bound_policy::visit_write(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
#if ENABLE_INSTRUMENTATION
        const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
        const unsigned size_index(SIZE_TO_INDEX[tracker.sizes[i]]);

        ASSERT(reg_index < 16);
        ASSERT(size_index < 5);

        instruction call(insert_cti_after(ls, tracker.labels[i],
            unsafe_cast<app_pc>(BOUNDS_CHECKERS[reg_index][size_index]),
            false, operand(),
            CTI_CALL));
        call.set_mangled();
        //(void)ls;
#else
        UNUSED(ls);
        UNUSED(tracker);
        UNUSED(i);
#endif /* ENABLE_INSTRUMENTATION */
    }


    void host_bound_policy::visit_read(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
#if ENABLE_INSTRUMENTATION
        const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
        const unsigned size_index(SIZE_TO_INDEX[tracker.sizes[i]]);

        ASSERT(reg_index < 16);
        ASSERT(size_index < 5);

        instruction call(insert_cti_after(ls, tracker.labels[i],
            unsafe_cast<app_pc>(BOUNDS_CHECKERS[reg_index][size_index]),
            false, operand(),
            CTI_CALL));
        call.set_mangled();
#else
        UNUSED(ls);
        UNUSED(tracker);
        UNUSED(i);
#endif /* ENABLE_INSTRUMENTATION */
    }


    void host_bound_policy::visit_write(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
#if ENABLE_INSTRUMENTATION
        const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
        const unsigned size_index(SIZE_TO_INDEX[tracker.sizes[i]]);

        ASSERT(reg_index < 16);
        ASSERT(size_index < 5);

        instruction call(insert_cti_after(ls, tracker.labels[i],
            unsafe_cast<app_pc>(BOUNDS_CHECKERS[reg_index][size_index]),
            false, operand(),
            CTI_CALL));
        call.set_mangled();
#else
        UNUSED(ls);
        UNUSED(tracker);
        UNUSED(i);
#endif /* ENABLE_INSTRUMENTATION */
    }


    void visit_overflow(
        uintptr_t watched_addr,
        app_pc addr_in_bb,
        unsigned size
    ) throw() {
        printf("Address %p in bb %p\n", watched_addr, addr_in_bb);
        (void) watched_addr;
        (void) addr_in_bb;
        (void) size;
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    interrupt_handled_state app_bound_policy::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_OVERFLOW == vec) {
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }
} /* wp namespace */
#else
} /* wp namespace */
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

} /* client namespace */
