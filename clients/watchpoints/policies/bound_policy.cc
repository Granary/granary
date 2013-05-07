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


    /// Register-specific bounds checker functions
    /// (defined in x86/bound_policy.asm).
    extern "C" {
        extern app_pc granary_bounds_check_rcx;
        extern app_pc granary_bounds_check_rdx;
        extern app_pc granary_bounds_check_rbx;
        extern app_pc granary_bounds_check_rbp;
        extern app_pc granary_bounds_check_rsi;
        extern app_pc granary_bounds_check_rdi;
        extern app_pc granary_bounds_check_r8;
        extern app_pc granary_bounds_check_r9;
        extern app_pc granary_bounds_check_r10;
        extern app_pc granary_bounds_check_r11;
        extern app_pc granary_bounds_check_r12;
        extern app_pc granary_bounds_check_r13;
        extern app_pc granary_bounds_check_r14;
        extern app_pc granary_bounds_check_r15;
        extern app_pc granary_bounds_check_rdx;
    }

    /// Register-specific (generated) functions to do bounds checking.
    static app_pc BOUNDS_CHECKERS[16] = {
        granary_bounds_check_rcx,
        granary_bounds_check_rdx,
        granary_bounds_check_rbx,
        nullptr, // rsp
        granary_bounds_check_rbp,
        granary_bounds_check_rsi,
        granary_bounds_check_rdi,
        granary_bounds_check_r8,
        granary_bounds_check_r9,
        granary_bounds_check_r10,
        granary_bounds_check_r11,
        granary_bounds_check_r12,
        granary_bounds_check_r13,
        granary_bounds_check_r14,
        granary_bounds_check_r15,
        granary_bounds_check_rdx
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
        ls.insert_after(
            tracker.labels[i],
            call_(pc_(BOUNDS_CHECKERS[tracker.regs[i].value.reg])));
    }


    void bound_policy::visit_write(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        ls.insert_after(
            tracker.labels[i],
            call_(pc_(BOUNDS_CHECKERS[tracker.regs[i].value.reg])));
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
