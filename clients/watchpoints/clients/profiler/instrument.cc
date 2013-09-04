/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-06-24
 *      Author: Peter Goodman, akshayk
 */

#include "clients/watchpoints/clients/profiler/instrument.h"


#define DECLARE_DESCRIPTOR_ACCESSOR_APP(reg) \
    extern void CAT(granary_access_descriptor_app_, reg)(void);


#define DECLARE_DESCRIPTOR_ACCESSORS_APP(reg, rest) \
    DECLARE_DESCRIPTOR_ACCESSOR_APP(reg) \
    rest


#define DESCRIPTOR_ACCESSOR_PTR_APP(reg) \
    &CAT(granary_access_descriptor_app_, reg)


#define DESCRIPTOR_ACCESSOR_PTRS_APP(reg, rest) \
    DESCRIPTOR_ACCESSOR_PTR_APP(reg), \
    rest


#define DECLARE_DESCRIPTOR_ACCESSOR_HOST(reg) \
    extern void CAT(granary_access_descriptor_host_, reg)(void);


#define DECLARE_DESCRIPTOR_ACCESSORS_HOST(reg, rest) \
    DECLARE_DESCRIPTOR_ACCESSOR_HOST(reg) \
    rest


#define DESCRIPTOR_ACCESSOR_PTR_HOST(reg) \
    &CAT(granary_access_descriptor_host_, reg)


#define DESCRIPTOR_ACCESSOR_PTRS_HOST(reg, rest) \
    DESCRIPTOR_ACCESSOR_PTR_HOST(reg), \
    rest


extern "C" {
    ALL_REGS(DECLARE_DESCRIPTOR_ACCESSORS_APP, DECLARE_DESCRIPTOR_ACCESSOR_APP)
    ALL_REGS(DECLARE_DESCRIPTOR_ACCESSORS_HOST, DECLARE_DESCRIPTOR_ACCESSOR_HOST)

}


namespace client {

    namespace wp {


        /// Register-specific (generated) functions to mark a leak descriptor
        /// as being accessed.
        typedef void (*descriptor_accessor_type)(void);
        static descriptor_accessor_type DESCRIPTOR_ACCESSORS_APP[] = {
            ALL_REGS(DESCRIPTOR_ACCESSOR_PTRS_APP, DESCRIPTOR_ACCESSOR_PTR_APP)
        };

        static descriptor_accessor_type DESCRIPTOR_ACCESSORS_HOST[] = {
            ALL_REGS(DESCRIPTOR_ACCESSOR_PTRS_HOST, DESCRIPTOR_ACCESSOR_PTR_HOST)
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

        void host_policy::visit_read(
            granary::basic_block_state &,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            using namespace granary;
            const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
            instruction call(insert_cti_after(ls, tracker.labels[i],
                unsafe_cast<app_pc>(DESCRIPTOR_ACCESSORS_HOST[reg_index]),
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL));
            call.set_mangled();

        }


        /// Mark the descriptor as having been accessed.
        void host_policy::visit_write(
            granary::basic_block_state &bb,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            visit_read(bb, ls, tracker, i);
        }


        /// Add instrumentation on every read and write that marks the
        /// watchpoint descriptor as having been accessed.
        void app_policy::visit_read(
            granary::basic_block_state &,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            using namespace granary;
            const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
            instruction call(insert_cti_after(ls, tracker.labels[i],
                unsafe_cast<app_pc>(DESCRIPTOR_ACCESSORS_APP[reg_index]),
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL));
            call.set_mangled();

        }


        /// Mark the descriptor as having been accessed.
        void app_policy::visit_write(
            granary::basic_block_state &bb,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            visit_read(bb, ls, tracker, i);
        }



#if CONFIG_CLIENT_HANDLE_INTERRUPT
        granary::interrupt_handled_state app_policy::handle_interrupt(
            granary::cpu_state_handle,
            granary::thread_state_handle,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw() {
            return granary::INTERRUPT_DEFER;
        }

        granary::interrupt_handled_state host_policy::handle_interrupt(
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

