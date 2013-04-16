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
#include "granary/basic_block.h"

#include "clients/instrument.h"


extern "C" {
    extern granary::cpu_state *get_percpu_state(void *);

    /// Allocates memory for an interrupt descriptor table.
    extern granary::detail::interrupt_descriptor_table *
    granary_allocate_idt(void);
}


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


    /// Emit the code needed to reconstruct this interrupt after executing the
    /// code within the delay region.
    static app_pc emit_delay_interrupt(void) {
        return nullptr;
    }


    /// Handle an interrupt. Returns true iff the interrupt was handled, or
    /// false if the interrupt should be handled by the kernel. This also
    /// decides when an interrupt must be delayed, and invokes the necessary
    /// functionality to delay the interrupt.
    interrupt_handled_state handle_interrupt(
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw() {
        app_pc pc(isf->instruction_pointer);

        if(is_code_cache_address(pc)) {
            basic_block bb(pc);

            // the patch stub code at the head of a basic block was interrupted
            if(bb.cache_pc_start > pc) {
                return INTERRUPT_DEFER;
            }

            app_pc delay_begin(nullptr);
            app_pc delay_end(nullptr);

            // we don't need to delay; let the client try to handle the
            // interrupt, or defer to the kernel if the client doesn't handle
            // the interrupt.
            if(!bb.get_interrupt_delay_range(delay_begin, delay_end)) {
#if CONFIG_CLIENT_HANDLE_INTERRUPT
                cpu_state_handle cpu;
                thread_state_handle thread;
                basic_block_state *bb_state(bb.state());
                instrumentation_policy policy(bb.policy);

                return policy.handle_interrupt(
                    cpu, thread, *bb_state, *isf, vector);

#else
                return false;
#endif
            }

            // we need to delay
            interrupt_stack_frame orig_isf(*isf);
            (void) orig_isf;
            return INTERRUPT_RETURN;

#if CONFIG_CLIENT_HANDLE_INTERRUPT
        } else if(is_host_address(pc)) {
            cpu_state_handle cpu;
            thread_state_handle thread;
            return client::handle_kernel_interrupt(
                cpu,
                thread,
                *isf,
                vector);
#endif
        } else {
            return INTERRUPT_DEFER;
        }
        return INTERRUPT_DEFER;
    }


    /// Emit an interrupt entry routine. This routine dispatches to a common
    /// vector entry routine, which does proper handling of the interrupt.
    static app_pc emit_interrupt_routine(
        unsigned vector_num,
        app_pc original_routine,
        app_pc common_interrupt_routine
    ) throw() {
        instruction_list ls;

        if(!has_error_code(vector_num)) {
            ls.append(push_(reg::rsp));
            ls.append(push_(seg::ss(*reg::rsp)));
        } else {
            ls.append(push_(reg::rsp));
        }

        ls.append(push_(reg::arg1));
        ls.append(push_(reg::arg2));
        ls.append(mov_ld_(reg::arg1, seg::ss(reg::rsp[16])));
        ls.append(mov_imm_(reg::arg2, int64_(vector_num)));
        ls.append(call_(pc_(common_interrupt_routine)));
        ls.append(pop_(reg::arg2));
        ls.append(pop_(reg::arg1));
        ls.append(pop_(reg::rsp));
        ls.append(jmp_(pc_(original_routine)));

        app_pc routine(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(ls.encoded_size() + 16));

        routine += ALIGN_TO(reinterpret_cast<uint64_t>(routine), 16);
        ls.encode(routine);

        return routine;
    }


    /// Emit a common interrupt entry routine. This routine handles the full
    /// interrupt.
    static app_pc emit_common_interrupt_routine() throw() {
        instruction_list ls;
        instruction in_kernel(label_());
        operand isf_ptr(reg::arg1);
        operand vector(reg::arg2);

        // save arg1 for later (likely clobbered by handle_interrupt) so that
        // if we are going to RET from the interrupt (instead of IRET), we can
        // do so regardless of whether the interrupt has an error code or not.
        ls.append(push_(isf_ptr));

        // save the return value as it will be clobbered by handle_interrupt
        ls.append(push_(reg::ret));

        // check for a privilege level change; OR together CS and SS, then
        // inspect the low order bits. If they are both zero then we were
        // interrupted in kernel space.
        ls.append(mov_ld_(reg::ret,
            seg::ss(isf_ptr[offsetof(interrupt_stack_frame, segment_cs)])));
        ls.append(or_(reg::ret,
            seg::ss(isf_ptr[offsetof(interrupt_stack_frame, segment_ss)])));
        ls.append(and_(reg::ret, int8_(0x3)));

        // CASE 1: not in kernel; set reg::ret to zero to signal that the
        // interrupt was not handled by Granary.
        {
            ls.append(jz_(instr_(in_kernel)));
            ls.append(pop_(reg::ret));
            ls.append(lea_(reg::rsp, seg::ss(reg::rsp[8])));
            ls.append(ret_());
        }

        // CASE 2: in the kernel.
        ls.append(in_kernel);

        // in kernel space
        register_manager rm;
        rm.kill_all();
        rm.revive(isf_ptr);
        rm.revive(vector);
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

        instruction ret(label_());
        instruction defer(label_());

        ls.append(jl_(instr_(ret))); // (ret = INTERRUPT_RETURN)
        ls.append(jz_(instr_(defer))); // (ret = INTERRUPT_DEFER)

        // CASE 2.1: fall-through: IRET from the interrupt, it has been handled.
        {
            ls.append(lea_(reg::rsp, reg::rsp[16])); // arg1 + return address
            ls.append(pop_(vector));
            ls.append(pop_(isf_ptr));
            ls.append(pop_(reg::rsp)); // align to base of ISF
            ls.append(iret_());
        }

        // CASE 2.2: return from the interrupt. This is tricky because the
        // interrupt stack frame might not be adjacent to the previous value of
        // the stack pointer (at the time of the interrupt). We will RET to the
        // return address in the ISF, which has been manipulated to be in the
        // position of previous stack pointer plus one.
        //
        // TODO: if a NMI comes when there is some funky stack manipulation then
        //       there will be an issue.
        ls.append(ret);
        {
            // make a mask for interrupts.
            // TODO: trap flag? virtual interrupt flag?
            eflags flags;
            flags.value = 0ULL;
            flags.interrupt = true;

            // restore the flags, but leave interrupts disabled. Assumes that
            // instrumentation code does not inspect the interrupt bit.
            ls.append(pop_(isf_ptr));
            ls.append(mov_imm_(reg::arg2, int64_(~(flags.value))));
            ls.append(and_(
                reg::arg2,
                isf_ptr[offsetof(interrupt_stack_frame, flags)]));
            ls.append(push_(reg::arg2));
            ls.append(popf_());

            // restore the old stack pointer, then allocate space on the stack
            // for the return address.
            ls.append(mov_ld_(
                reg::rsp,
                isf_ptr[offsetof(interrupt_stack_frame, stack_pointer)]));
            ls.append(lea_(reg::rsp, reg::rsp[-8]));

            // copy the return address of the ISF onto the top of the
            // (now changed) stack.
            ls.append(mov_ld_(
                reg::arg2,
                isf_ptr[offsetof(interrupt_stack_frame, instruction_pointer)]));
            ls.append(mov_st_(*reg::rsp, reg::arg2));

            // restore arg2 and arg1.
            ls.append(mov_ld_(reg::arg2, isf_ptr[-16]));
            ls.append(mov_ld_(reg::arg1, isf_ptr[-8]));

            // return, as if the interrupt was a function, with interrupts
            // disabled.
            ls.append(ret_());
        }

        // CASE 2.3: Return from this function because Granary has not handled
        // the interrupt, and defer to the kernel's interrupt handlers.
        ls.append(defer);
        {
            ls.append(lea_(reg::rsp, reg::rsp[8])); // arg1
            ls.append(ret_());
        }

        // encode.
        app_pc routine(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(ls.encoded_size() + 16));

        routine += ALIGN_TO(reinterpret_cast<uint64_t>(routine), 16);
        ls.encode(routine);

        return routine;
    }


    /// Create a Granary version of the interrupt descriptor table. This
    /// assumes that the IDT for all CPUs is the same.
    system_table_register_t emit_idt(void) throw() {

        system_table_register_t native;
        system_table_register_t instrumented;
        detail::interrupt_descriptor_table *idt(granary_allocate_idt());

        get_idtr(&native);

        detail::interrupt_descriptor_table *kernel_idt(
            unsafe_cast<detail::interrupt_descriptor_table *>(native.base));

        memset(idt, 0, sizeof *idt);
        memcpy(idt, kernel_idt, native.limit + 1);

        app_pc common_vector_handler(emit_common_interrupt_routine());

        const unsigned num_vecs((native.limit + 1) / (2 * sizeof(descriptor_t)));
        for(unsigned i(0); i < num_vecs; ++i) {
            descriptor_t *i_vec(&(idt->vectors[i * 2]));
            descriptor_t *n_vec(&(native.base[i * 2]));

            // update the gate
            if(GATE_DESCRIPTOR == get_descriptor_kind(i_vec)) {
                app_pc native_handler(get_gate_target_offset(&(n_vec->gate)));

                if(!is_host_address(native_handler)) {
                    continue;
                }

                set_gate_target_offset(
                    &(i_vec->gate),
                    emit_interrupt_routine(
                        i,
                        native_handler,
                        common_vector_handler));
            }
        }

        instrumented.base = &(idt->vectors[0]);
        instrumented.limit = native.limit;

        return instrumented;
    }
}
