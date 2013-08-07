/*
 * instrument.cc
 *
 *  Created on: 2013-08-04
 *      Author: akshayk
 */


#include "clients/watchpoints/clients/shadow_memory/instrument.h"


#define DECLARE_READ_ACCESSOR(reg) \
    extern void CAT(granary_shadow_update_read_, reg)(void);

#define DECLARE_WRITE_ACCESSOR(reg) \
    extern void CAT(granary_shadow_update_write_, reg)(void);


#define DECLARE_READ_ACCESSORS(reg, rest) \
    DECLARE_READ_ACCESSOR(reg) \
    rest

#define DECLARE_WRITE_ACCESSORS(reg, rest) \
    DECLARE_WRITE_ACCESSOR(reg) \
    rest


#define DESCRIPTOR_READ_ACCESSOR_PTR(reg) \
    &CAT(granary_shadow_update_read_, reg)


#define DESCRIPTOR_READ_ACCESSOR_PTRS(reg, rest) \
    DESCRIPTOR_READ_ACCESSOR_PTR(reg), \
    rest

#define DESCRIPTOR_WRITE_ACCESSOR_PTR(reg) \
    &CAT(granary_shadow_update_write_, reg)


#define DESCRIPTOR_WRITE_ACCESSOR_PTRS(reg, rest) \
    DESCRIPTOR_WRITE_ACCESSOR_PTR(reg), \
    rest

extern "C" {
    ALL_REGS(DECLARE_READ_ACCESSORS, DECLARE_READ_ACCESSOR)

    ALL_REGS(DECLARE_WRITE_ACCESSORS, DECLARE_WRITE_ACCESSOR)
}



namespace client {

    namespace wp {

    /// Register-specific (generated) functions to mark a leak descriptor
    /// as being accessed.
    typedef void (*descriptor_accessor_type)(void);
    static descriptor_accessor_type DESCRIPTOR_READ_ACCESSORS[] = {
        ALL_REGS(DESCRIPTOR_READ_ACCESSOR_PTRS, DESCRIPTOR_READ_ACCESSOR_PTR)
    };

    static descriptor_accessor_type DESCRIPTOR_WRITE_ACCESSORS[] = {
        ALL_REGS(DESCRIPTOR_WRITE_ACCESSOR_PTRS, DESCRIPTOR_WRITE_ACCESSOR_PTR)
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

        /// Add instrumentation on every read and write that marks the
        /// shadow bits for the corresponding types .
        void shadow_policy::visit_read(
            granary::basic_block_state &,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            using namespace granary;
            const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
            instruction call(insert_cti_after(ls, tracker.labels[i],
                unsafe_cast<app_pc>(DESCRIPTOR_READ_ACCESSORS[reg_index]),
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL));
            call.set_mangled();
        }


        void shadow_policy::visit_write(
            granary::basic_block_state &,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            using namespace granary;
            const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
            instruction call(insert_cti_after(ls, tracker.labels[i],
                unsafe_cast<app_pc>(DESCRIPTOR_WRITE_ACCESSORS[reg_index]),
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL));
            call.set_mangled();
        }

        /// Visit shadow callback. This is invoked by
        void visit_shadow(
            uintptr_t watched_addr,
            granary::app_pc addr_in_bb,
            unsigned size
        ) throw() {
            granary::printf("Access of size %u to %p in basic block %p overflowed\n",
                size, watched_addr, addr_in_bb);
        }

#if CONFIG_CLIENT_HANDLE_INTERRUPT
        granary::interrupt_handled_state shadow_policy::handle_interrupt(
            granary::cpu_state_handle,
            granary::thread_state_handle,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw() {
            return granary::INTERRUPT_DEFER;
        }
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

    }
}
