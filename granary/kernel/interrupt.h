/*
 * interrupt.h
 *
 *  Created on: 2013-04-09
 *      Author: pag
 */

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include "deps/drk/segment_descriptor.h"

namespace granary {


    /// Signify to what extent Granary handled this interrupt.
    enum interrupt_handled_state : int64_t {

        // Return from this interrupt as if it were just another function call.
        INTERRUPT_RETURN = -1,

        // Defer to the kernel's interrupt handlers for this interrupt.
        INTERRUPT_DEFER = 0,

        // We've handled this interrupt, perform an iret to return from the
        // interrupt.
        INTERRUPT_IRET = 1
    };


    /// Represents an interrupt stack frame that is automatically pushed on the
    /// stack during an interrupt / exception.
    struct interrupt_stack_frame {
        uint64_t error_code;
        app_pc instruction_pointer;
        uint64_t segment_cs;
        eflags flags;
        uint64_t *stack_pointer;
        uint64_t segment_ss;
    };


    /// Interrupt vector number.
    enum interrupt_vector {
        VECTOR_DIVIDE_ERROR = 0,
        VECTOR_START = VECTOR_DIVIDE_ERROR,
        VECTOR_EXCEPTION_START = VECTOR_DIVIDE_ERROR,
        VECTOR_DEBUG = 1,
        VECTOR_NMI = 2,
        VECTOR_BREAKPOINT = 3,
        VECTOR_OVERFLOW = 4,
        VECTOR_BOUND_RANGE_EXCEEDED = 5,
        VECTOR_INVALID_OPCODE = 6,
        VECTOR_DEVICE_NOT_AVAILABLE = 7,
        VECTOR_DOUBLE_FAULT = 8,
        VECTOR_COPROCESSOR_SEGMENT_OVERRUN = 9,
        VECTOR_INVALID_TSS = 10,
        VECTOR_SEGMENT_NOT_PRESENT = 11,
        VECTOR_STACK_FAULT = 12,
        VECTOR_GENERAL_PROTECTION = 13,
        VECTOR_PAGE_FAULT = 14,
        /* no 15 */
        VECTOR_X87_FPU_FLOATING_POINT_ERROR = 16,
        VECTOR_ALIGNMENT_CHECK = 17,
        VECTOR_MACHINE_CHECK = 18,
        VECTOR_SIMD_FLOATING_POINT = 19,
        VECTOR_SECURITY_EXCEPTION = 20,

        VECTOR_EXCEPTION_END = VECTOR_SECURITY_EXCEPTION,
        VECTOR_INTERRUPT_START = 32,
        VECTOR_INTERRUPT_END = 255,
        VECTOR_END = VECTOR_INTERRUPT_END
    };


    /// Replace the IDT with one that Granary controls.
    system_table_register_t create_idt(void) throw();
}


#endif /* INTERRUPT_H_ */
