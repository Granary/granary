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
        case 20: // #SX: security exception fault (svm);
                 // TODO: does amd64 and intel docs disagree about vec20 ec?
        default:
            return false;
        }
    }


    /// Handers for the various interrupt vectors.
    static app_pc VECTOR_HANDLER[256] = {nullptr};


    /// Offsets used to align the stack pointer.
    static uint8_t STACK_ALIGN[256] = {0};


    /// Initialise the stack alignment table. Note: the stack grows down, and
    /// we will use this table with XLATB to align the pseudo stack pointer in
    /// one shot.
    STATIC_INITIALISE_ID(stack_align_table, {
        for(unsigned i(0); i < 256; ++i) {
            STACK_ALIGN[i] = i - (i % 16);
        }
    })


    /// Mangle a delayed instruction.
    ///
    /// We make several basic assumptions:
    ///     i)  Code in a delay region does not fault.
    ///     ii) Eliding cli/sti in a delay region is safe because the code
    ///         within the delay region (given assumption 1) must have been
    ///         interrupted before the 'cli' or after the 'sti'.
    ///     iii) If a delay region contains control-flow instructions then
    ///          the targets of those instructions will remain within the
    ///          delay region. This restrics delay regions to not containing
    ///          any indirect control-flow instructions, as correctness cannot
    ///          be guaranteed for such instructions.
    ///
    /// Mangling this code is tricky for two reasons:
    ///     i) POPFs cannot be allowed to restore interrupts, as code in
    ///        a delayed interrupt region is not re-entrant.
    ///     ii) Control flow within the delay region stays within the delay
    ///         region.
    __attribute__((hot))
    static void mangle_delayed_instruction(
        instruction_list &ls,
        instruction in
    ) throw() {
        switch(in.op_code()) {

        // Elide these instructions. This does make a minor second assumption.
        case dynamorio::OP_cli:
        case dynamorio::OP_sti: {

            // make sure any CTIs can correctly be resolved to the elided
            // instruction.
            instruction place(ls.append(label_()));
            place.set_pc(in.pc());
            return;
        }

        // Pop the flags, but first force interrupts to remain disabled.
        case dynamorio::OP_popf: {
            eflags mask;
            mask.value = 0ULL;
            mask.interrupt = true;
            instruction first(ls.append(push_(reg::rax)));
            ls.append(mov_imm_(reg::rax, int64_(~(mask.value))));
            ls.append(and_(reg::rsp[8], reg::rax));
            ls.append(pop_(reg::rax));
            ls.append(in); // popf_

            // make sure any CTIs can correctly be resolved to the expanded
            // instruction.
            first.set_pc(in.pc());
            in.set_pc(nullptr);

            break;
        }

        default:
            ls.append(in);
            break;
        }
    }


    /// Mangle a CTI within the delay region. This tries to redirect a CTI to
    /// point back within the delay region. This code will fault if the CTI
    /// cannot be redirected.
    static void mangle_cti(instruction_list &ls, instruction cti) throw() {
        if(cti.is_return()) {
            return;
        }

        operand target(cti.cti_target());
        if(!dynamorio::opnd_is_pc(target)) {
            return;
        }

        const app_pc target_pc(target.value.pc);
        for(instruction in(ls.first()); in; in = in.next()) {
            if(in.pc() == target_pc) {
                cti.set_cti_target(instr_(in));
                return;
            }
        }

        FAULT;
    }


    /// Emit the code needed to reconstruct this interrupt after executing the
    /// code within the delay region. Interrupt delaying works by copying and
    /// re-relativizing all of the code within a interrupt delay region into a
    /// CPU-private location. After this, code is emitted to re-build the
    /// interrupt stack frame, but with `delay_end` as the return address for
    /// the interrupt. The interrupt is then re-issued to Granary (and then
    /// likely to the kernel).
    ///
    /// Returns the app_pc for the instruction where execution should resume.
    __attribute__((hot))
    app_pc emit_delayed_interrupt(
        cpu_state_handle cpu,
        interrupt_stack_frame *isf,
        interrupt_vector vector,
        app_pc delay_begin,
        app_pc delay_end
    ) {
        instruction_list ls;
        instruction delay_in;

        const bool has_ec(has_error_code(vector));

        IF_PERF( perf::visit_delayed_interrupt(); )

        unsigned num_ctis(0);
        for(; delay_begin < delay_end; ) {
            if(isf->instruction_pointer == delay_begin) {
                delay_in = ls.append(label_());
            }

            instruction in(instruction::decode(&delay_begin));
            if(in.is_cti()) {
                ++num_ctis;
            }
            mangle_delayed_instruction(ls, in);
        }

        // redirect internal CTIs.
        if(num_ctis) {
            for(instruction in(ls.first()); num_ctis-- && in; in = in.next()) {
                if(in.is_cti()) {
                    mangle_cti(ls, in);
                }
            }
        }

        // Spill RAX and RBX to their designated CPU-private spill slots so
        // that we can align the stack pointer (for the ISF) without modifying
        // the flags.
        ls.append(push_(reg::rax));
        ls.append(mov_imm_(
            reg::rax,
            int64_(reinterpret_cast<uint64_t>(&(cpu->spill[0])))));
        ls.append(mov_st_(reg::rax[8], reg::rbx));
        ls.append(pop_(reg::rbx)); // RBX now holds original value of RAX
        ls.append(mov_st_(*reg::rax, reg::rbx));

        // Align the new stack pointer (RAX) to a 16-byte boundary using a
        // lookup table (so we don't corrupt the flags).
        ls.append(mov_st_(reg::rax, reg::rsp));
        ls.append(mov_imm_(
            reg::rbx,
            int64_(reinterpret_cast<uint64_t>(STACK_ALIGN))));
        ls.append(xlat_());

        // save the old stack pointer into RBX, update the stack pointer.
        ls.append(mov_st_(reg::rbx, reg::rsp));
        ls.append(mov_st_(reg::rsp, reg::rax));

        // SS
        ls.append(mov_imm_(reg::rax, int64_(isf->segment_ss)));
        ls.append(push_(reg::rax));

        // RSP (old stack pointer)
        ls.append(push_(reg::rbx));

        // RFLAGS at the end of the delay region; we'll assume that delay regions
        // are smart enough not to corrupt the flags. We also need to be careful
        // about re-enabling interrupts.
        ls.append(pushf_());

        // emit code to re-enable interrupts (when the flags are restored),
        // assuming they were previously enabled.
        //
        // TODO: this assumes that code in a delay region will not fault!!!
        if(isf->flags.interrupt) {
            eflags mask;
            mask.value = 0ULL;
            mask.interrupt = true;
            ls.append(mov_imm_(reg::rax, int64_(mask.value)));
            ls.append(or_(*reg::rsp, reg::rax));
        }

        // CS
        ls.append(mov_imm_(reg::rax, int64_(isf->segment_cs)));
        ls.append(push_(reg::rax));

        // RIP (instruction pointer), points to next instruction in the basic
        // block after the delay region.
        ls.append(mov_imm_(
            reg::rax,
            int64_(reinterpret_cast<uint64_t>(delay_end))));
        ls.append(push_(reg::rax));

        // Error Code, if any.
        if(has_ec) {
            ls.append(mov_imm_(reg::rax, int64_(isf->error_code)));
            ls.append(push_(reg::rax));
        }

        // restore RAX and RBX.
        ls.append(mov_imm_(
            reg::rax,
            int64_(reinterpret_cast<uint64_t>(&(cpu->spill[0])))));
        ls.append(mov_ld_(reg::rbx, reg::rax[8]));
        ls.append(mov_ld_(reg::rax, *reg::rax));

        // jump to the interrupt handler
        ls.append(jmp_(pc_(VECTOR_HANDLER[vector])));

        ls.encode(cpu->interrupt_delay_handler);

        return delay_in.pc();
    }


    /// Handle an interrupt. Returns true iff the interrupt was handled, or
    /// false if the interrupt should be handled by the kernel. This also
    /// decides when an interrupt must be delayed, and invokes the necessary
    /// functionality to delay the interrupt.
    GRANARY_ENTRYPOINT
    __attribute__((hot))
    interrupt_handled_state handle_interrupt(
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw() {
        kernel_preempt_disable();

        cpu_state_handle cpu;
        thread_state_handle thread;
        granary::enter(cpu, thread);

        app_pc pc(isf->instruction_pointer);

        IF_PERF( perf::visit_interrupt(); )

        if(is_code_cache_address(pc)) {
            basic_block bb(pc);

            // the patch stub code at the head of a basic block was interrupted
            if(bb.cache_pc_start > pc) {
                kernel_preempt_enable();
                return INTERRUPT_DEFER;
            }

            app_pc delay_begin(nullptr);
            app_pc delay_end(nullptr);

            // we don't need to delay; let the client try to handle the
            // interrupt, or defer to the kernel if the client doesn't handle
            // the interrupt.
            if(!bb.get_interrupt_delay_range(delay_begin, delay_end)) {
#if CONFIG_CLIENT_HANDLE_INTERRUPT
                basic_block_state *bb_state(bb.state());
                instrumentation_policy policy(bb.policy);

                interrupt_handled_state ret(policy.handle_interrupt(
                    cpu, thread, *bb_state, *isf, vector));

                kernel_preempt_enable();
                return ret;

#else
                kernel_preempt_enable();
                return INTERRUPT_DEFER;
#endif
            }

            // we need to delay
            isf->instruction_pointer = emit_delayed_interrupt(
                cpu, isf, vector, delay_begin, delay_end);

            kernel_preempt_enable();
            return INTERRUPT_RETURN;

#if CONFIG_CLIENT_HANDLE_INTERRUPT
        } else if(is_host_address(pc)) {
            interrupt_handled_state ret(client::handle_kernel_interrupt(
                cpu,
                thread,
                *isf,
                vector));

            kernel_preempt_enable();
            return ret;
#endif
        } else {
            // Detect an exception within a delayed interrupt handler. This
            // is really bad.
            //
            // TODO: if the vector is an exception, then perhaps an okay
            //       strategy is to force execution to the next instruction.
            const app_pc delayed_begin(cpu->interrupt_delay_handler);
            const app_pc delayed_end(delayed_begin + INTERRUPT_DELAY_CODE_SIZE);
            if(delayed_begin <= pc && pc < delayed_end) {
                IF_PERF( perf::visit_recursive_interrupt(); )
                FAULT;
                //return INTERRUPT_IRET;
            }
        }

        kernel_preempt_enable();
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

        // this makes it convenient to find top of the ISF from the common
        // interrupt handler
        ls.append(push_(reg::rsp));
        if(!has_error_code(vector_num)) {
            ls.append(push_(seg::ss(*reg::rsp)));
        }

        ls.append(push_(reg::arg1));
        ls.append(push_(reg::arg2));
        ls.append(lea_(reg::arg1, seg::ss(reg::rsp[24])));
        ls.append(mov_imm_(reg::arg2, int64_(vector_num)));
        ls.append(call_(pc_(common_interrupt_routine)));
        ls.append(pop_(reg::arg2));
        ls.append(pop_(reg::arg1));
        ls.append(pop_(reg::rsp));
        ls.append(jmp_(pc_(original_routine)));

        app_pc routine(reinterpret_cast<app_pc>(
            global_state::FRAGMENT_ALLOCATOR-> \
                allocate_untyped(CACHE_LINE_SIZE, ls.encoded_size())));

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

        // check for a privilege level change: OR together CS and SS, then
        // inspect the low order bits. If they are both zero then we were
        // interrupted in kernel space.
        ls.append(mov_ld_(reg::ret,
            seg::ss(isf_ptr[offsetof(interrupt_stack_frame, segment_cs)])));
        ls.append(or_(reg::ret,
            seg::ss(isf_ptr[offsetof(interrupt_stack_frame, segment_ss)])));
        ls.append(and_(reg::ret, int8_(0x3)));
        ls.append(jz_(instr_(in_kernel)));

        // CASE 1: not in kernel
        {
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
        insert_restore_old_stack_alignment_after(ls, in);

        // check to see if the interrupt was handled or not
        ls.append(xor_(isf_ptr, isf_ptr));
        ls.append(cmp_(reg::ret, isf_ptr));
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
        // position of previous stack pointer plus one. This is also tricky
        // because it must be safe w.r.t. NMIs.
        ls.append(ret);
        {
            // make a mask for interrupts.
            // TODO: trap flag? virtual interrupt flag?
            eflags flags;
            flags.value = 0ULL;
            flags.interrupt = true;

            ls.append(pop_(isf_ptr));
            ls.append(push_(reg::rax)); // will serve as a temp stack ptr

            // use rax as a stack pointer
            ls.append(mov_ld_(
                reg::rax,
                isf_ptr[offsetof(interrupt_stack_frame, stack_pointer)]));

            // put the return address on the stack
            ls.append(mov_ld_(
                reg::arg2,
                isf_ptr[offsetof(interrupt_stack_frame, instruction_pointer)]));
            ls.append(mov_st_(reg::rax[-8], reg::arg2));

            // put the flags on the stack, with interrupts disabled.
            ls.append(mov_imm_(reg::arg2, int64_(~(flags.value))));
            ls.append(and_(
                reg::arg2,
                isf_ptr[offsetof(interrupt_stack_frame, flags)]));
            ls.append(mov_st_(reg::rax[-16], reg::arg2));

            // move the saved rax to a different location
            ls.append(mov_ld_(reg::arg2, *reg::rsp));
            ls.append(mov_st_(reg::rax[-24], reg::arg2));

            // compute the new stack pointer, restore arg1, arg2, rsp, and rax.
            ls.append(lea_(reg::rax, reg::rax[-24]));
            ls.append(mov_ld_(reg::arg2, reg::rsp[16]));
            ls.append(mov_ld_(reg::arg1, reg::rsp[24]));
            ls.append(mov_ld_(reg::rsp, reg::rax));
            ls.append(pop_(reg::rax));

            // restore flags, and return
            ls.append(popf_());
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
    system_table_register_t create_idt(void) throw() {

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

                app_pc target(nullptr);

                if(VECTOR_NMI == i || VECTOR_DEBUG == i) {
                    target = native_handler;
                } else {
                    target = emit_interrupt_routine(
                        i,
                        native_handler,
                        common_vector_handler);
                }


                VECTOR_HANDLER[i] = target;

                set_gate_target_offset(
                    &(i_vec->gate),
                    target);
            }
        }

        instrumented.base = &(idt->vectors[0]);
        instrumented.limit = native.limit;

        return instrumented;
    }
}
