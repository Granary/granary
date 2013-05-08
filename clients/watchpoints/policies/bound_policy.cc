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


#define DECLARE_BOUND_CHECKER(reg) \
    extern app_pc CAT(granary_bounds_check_1_, reg); \
    extern app_pc CAT(granary_bounds_check_2_, reg); \
    extern app_pc CAT(granary_bounds_check_4_, reg); \
    extern app_pc CAT(granary_bounds_check_8_, reg); \
    extern app_pc CAT(granary_bounds_check_16_, reg);


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
        CAT(granary_bounds_check_1_, reg), \
        CAT(granary_bounds_check_2_, reg), \
        CAT(granary_bounds_check_4_, reg), \
        CAT(granary_bounds_check_8_, reg), \
        CAT(granary_bounds_check_16_, reg) \
    }

#define BOUND_CHECKER_GROUPS(reg, rest) \
    BOUND_CHECKER_GROUP(reg), \
    rest


    /// Register-specific (generated) functions to do bounds checking.
    static app_pc BOUNDS_CHECKERS[15][5] = {
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


    /// Allocate a watchpoint descriptor and assign `desc` and `index`
    /// appropriately.
    bool bound_descriptor::allocate(
        bound_descriptor *&desc,
        uintptr_t &index
    ) throw() {
        (void) desc;
        (void) index;
        return false;
    }


    /// Free a watchpoint descriptor.
    void bound_descriptor::free(
        bound_descriptor *desc,
        uintptr_t index
    ) throw() {
        (void) desc;
        (void) index;
    }


    /// Initialise a watchpoint descriptor.
    void bound_descriptor::init(
        bound_descriptor *desc,
        void *base_address,
        size_t size
    ) throw() {
        (void) desc;
        (void) base_address;
        (void) size;
    }


    void bound_policy::visit_read(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
        const unsigned size_index(SIZE_TO_INDEX[tracker.op_sizes[i]]);
        instruction call(insert_cti_after(ls, tracker.labels[i],
            BOUNDS_CHECKERS[reg_index][size_index],
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
        const unsigned size_index(SIZE_TO_INDEX[tracker.op_sizes[i]]);
        instruction call(insert_cti_after(ls, tracker.labels[i],
            BOUNDS_CHECKERS[reg_index][size_index],
            false, operand(),
            CTI_CALL));
        call.set_mangled();
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
