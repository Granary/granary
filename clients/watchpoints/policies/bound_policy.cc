/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_policy.cc
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#include "clients/watchpoints/policies/bound_policy.h"

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


    /// Register-specific (generated) functions to do bounds checking.
    static unsigned REG_TO_INDEX[] = {
        ~0U,    // null
        0,      // rax
        1,      // rbx
        2,      // rcx
        3,      // rdx
        4,      // rbx
        ~0U,    // rsp
        5,      // rbp
        6,      // rsi
        7,      // rdi
        8,      // r8
        9,      // r9
        10,      // r10
        11,     // r11
        12,     // r12
        13,     // r13
        14,     // r14
        15      // r15
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
    extern "C++" bound_descriptor *DESCRIPTORS[MAX_NUM_WATCHPOINTS] = {nullptr};


    /// Allocate a watchpoint descriptor and assign `desc` and `index`
    /// appropriately.
    bool bound_descriptor::allocate(
        bound_descriptor *&desc,
        uintptr_t &counter_index
    ) throw() {
        counter_index = next_counter_index();
        if(counter_index > MAX_COUNTER_INDEX) {
            return false;
        }

        desc = DESCRIPTOR_ALLOCATOR->allocate<bound_descriptor>();
        return true;
    }


    /// Initialise a watchpoint descriptor.
    void bound_descriptor::init(
        bound_descriptor *desc,
        void *base_address,
        size_t size
    ) throw() {
        const uintptr_t base(reinterpret_cast<uintptr_t>(base_address));
        desc->lower_bound = static_cast<uint16_t>(base);
        desc->upper_bound = static_cast<uint16_t>(base + size);
        desc->next_free_index = 0;
    }


    /// Free a watchpoint descriptor.
    void bound_descriptor::free(
        bound_descriptor *desc,
        uintptr_t index
    ) throw() {
        (void) desc;
        (void) index;
    }


    void bound_policy::visit_read(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
        const unsigned size_index(SIZE_TO_INDEX[tracker.sizes[i]]);

        ASSERT(reg_index < 16);
        ASSERT(size_index < 5);

        instruction call(insert_cti_after(ls, tracker.labels[i],
            unsafe_cast<app_pc>(BOUNDS_CHECKERS[reg_index][size_index]),
            false, operand(),
            CTI_CALL));
        call.set_mangled();
    }


    void bound_policy::visit_write(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
        const unsigned size_index(SIZE_TO_INDEX[tracker.sizes[i]]);

        ASSERT(reg_index < 16);
        ASSERT(size_index < 5);

        instruction call(insert_cti_after(ls, tracker.labels[i],
            unsafe_cast<app_pc>(BOUNDS_CHECKERS[reg_index][size_index]),
            false, operand(),
            CTI_CALL));
        call.set_mangled();
    }


    void bound_policy::visit_overflow(
        uintptr_t watched_addr,
        app_pc *addr_in_bb,
        unsigned size
    ) throw() {
        (void) watched_addr;
        (void) addr_in_bb;
        (void) size;
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    interrupt_handled_state bound_policy::handle_interrupt(
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


    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    interrupt_handled_state handle_kernel_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        interrupt_stack_frame &,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_OVERFLOW == vec) {
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }
#else
} /* wp namespace */
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

} /* client namespace */
