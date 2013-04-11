/*
 * interrupt.cc
 *
 *  Created on: 2013-04-08
 *      Author: pag
 */

#include <cstddef>

#include "granary/globals.h"

#include "granary/instruction.h"
#include "granary/emit_utils.h"
#include "granary/state.h"

namespace granary {


    /// Return true iff a particular vector will push an error code on the
    /// stack.
    static bool has_error_code(unsigned vector) throw() {
        switch(vector) {
        case 0: // #DE: divide by zero exception
        case 1: // #DB: debug exception
        case 2: // NMI: non-maskable interrupt
        case 3: // #BP: breakpoint (caused by an int3)
        case 4: // #OF: overflow exception
        case 5: // #BR: bound range exception
        case 6: // #UD: invalid opcode
        case 7: // #NM: device not available
            return false;
        case 8: // #DF: double fault
            return true;
        case 9: // coprocessor segment overrun
            return false;
        case 10: // #TS: invalid TSS
        case 11: // #NP: segment not present
        case 12: // #SS: stack exception
        case 13: // #GP: general protection fault
        case 14: // #PF: page fault
            return true;
        case 15: // ???
        case 16: // #MF: floating point exception pending
            return false;
        case 17: // #AC: alignment check exception
            return true;
        case 18: // #MC: machine check exception
        case 19: // #XF: SIMD floating point exception
            return false;
        case 20: // #SX: security exception fault (svm);
                 // TODO: dispatches to vector 30 on amd64?
            return true;
        default:
            return false;
        }
    }


    /// Handle an interrupt. Returns true iff the interrupt was handled, or
    /// false if the interrupt should be handled by the kernel.
    static bool handle_interrupt(
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw() {
        (void) isf;
        (void) vector;
        return false;
    }


    /// Emit an interrupt entry routine. This routine dispatches to a common
    /// vector entry routine, which does proper handling of the interrupt.
    app_pc emit_interrupt_routine(
        unsigned vector_num,
        app_pc original_routine,
        app_pc common_interrupt_routine
    ) throw() {
        instruction_list ls;
        ls.append(push_(reg::arg1));
        ls.append(push_(reg::arg2));
        ls.append(lea_(reg::arg1, reg::rsp[has_error_code(vector_num) ? 16 : 8]));
        ls.append(mov_imm_(reg::arg2_16, int16_(vector_num)));
        ls.append(call_(pc_(common_interrupt_routine)));
        ls.append(pop_(reg::arg2));
        ls.append(pop_(reg::arg1));
        ls.append(jmp_(pc_(original_routine)));

        app_pc routine(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(ls.encoded_size()));
        ls.encode(routine);

        return routine;
    }


    /// Emit a common interrupt entry routine. This routine handles the full
    /// interrupt.
    app_pc emit_common_interrupt_routine() throw() {
        instruction_list ls;
        instruction in_kernel(label_());

        ls.append(push_(reg::ret));

        // check for a privilege level change; OR together CS and SS, then
        // inspect the low order bits
        ls.append(mov_ld_(reg::ret,
            reg::arg1[offsetof(interrupt_stack_frame, segment_cs)]));
        ls.append(or_(reg::ret,
            reg::arg1[offsetof(interrupt_stack_frame, segment_ss)]));
        ls.append(and_(reg::ret_8, int8_(0x3)));
        ls.append(in_kernel);

        // not in kernel; set reg::ret to zero to signal that the interrupt
        // was not handled by Granary.
        ls.insert_before(in_kernel, test_(reg::ret_8, reg::ret_8));
        ls.insert_before(in_kernel, jz_(instr_(in_kernel)));
        ls.insert_before(in_kernel, pop_(reg::ret));
        ls.insert_before(in_kernel, ret_());

        // in kernel space
        register_manager rm;
        rm.kill_all();
        rm.revive(reg::arg1);
        rm.revive(reg::arg2);
        rm.revive(reg::ret);

        // call out to the handler
        instruction in(ls.append(label_()));
        in = save_and_restore_registers(rm, ls, in);
        in = insert_align_stack_after(ls, in);
        in = insert_cti_after(
            ls, in, // instruction
            unsafe_cast<app_pc>(handle_interrupt), // target
            true, reg::ret, // clobber reg
            CTI_CALL);
        in = insert_restore_old_stack_alignment_after(ls, in);

        // check to see if the interrupt was handled or not
        ls.append(test_(reg::ret, reg::ret));
        ls.append(pop_(reg::ret));

        instruction ui(label_());
        ls.append(ui);
        ls.insert_before(ui, jz_(instr_(ui)));

        // the interrupt was handled; pop off the saved registers and return
        // from the interrupt.
        ls.insert_before(ui, lea_(reg::rsp, reg::rsp[8])); // return address
        ls.insert_before(ui, pop_(reg::arg2));
        ls.insert_before(ui, pop_(reg::arg1));
        ls.insert_before(ui, iret_());

        // the interrupt was not handled; return to the vector-specific
        // interrupt handler and transfer control to the kernel's vector
        // handler.
        ls.append(ret_());

        app_pc routine(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(ls.encoded_size()));
        ls.encode(routine);

        return routine;
    }
}
