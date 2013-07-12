/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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
        case 9: // co-processor segment overrun
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


    /// Start and end of the common interrupt handler.
    static app_pc COMMON_HANDLER_BEGIN = nullptr;
    static app_pc COMMON_HANDLER_END = nullptr;


    /// Initialise the stack alignment table. Note: the stack grows down, and
    /// we will use this table with XLATB to align the pseudo stack pointer in
    /// one shot.
    STATIC_INITIALISE_ID(stack_align_table, {
        for(unsigned i(0); i < 256; ++i) {
            STACK_ALIGN[i] = i - (i % 16);
        }
    })


    /// CPU States array for each processor.
    extern cpu_state *CPU_STATES[];


    /// Emit code to restore thread-local data onto the CPU executing an
    /// interrupt's return address instruction.
    ///
    /// Note: Tricky bits with TLS restore is that it should be re-entrant
    ///       at every instruction.
    app_pc get_tls_restore_stub(thread_state *state, app_pc target) throw() {

        // Set the stub's target, regardless of if we need to generate the
        // stub or not.
        state->restore_stub_target = target;

        // Already created the stub.
        if(state->restore_stub) {
            return state->restore_stub;
        }

        instruction_list ls;
        ls.append(lea_(reg::rsp, reg::rsp[-8])); // (1)
        ls.append(pushf_()); // (2)
        ls.append(cli_()); // (3)

        // It's okay to be interrupted before (1), (2), or (3) because we'll
        // assume that the `thread_data` pointer of the CPU-private state
        // is NULL.

        // After this point, we assume no NMIs / exceptions.

        ls.append(push_(reg::rax));
        ls.append(push_(reg::rbx));

        // TODO: Linux-specific use of `smp_processor_id.`
        ls.append(mov_ld_(reg::eax, seg::gs[0]));
        ls.append(movzx_(reg::rax, reg::ax));

        ls.append(mov_imm_(
            reg::rbx, int64_(reinterpret_cast<uintptr_t>(&(CPU_STATES[0])))));

        // RAX now has the address of our CPU's `cpu_state *`.
        ls.append(mov_ld_(reg::rax, reg::rbx + reg::rax * 8));

        // Address of our thread's `thread_state *`.
        ls.append(mov_imm_(
            reg::rbx, int64_(reinterpret_cast<uintptr_t>(state->restore_stub))));

        // Store TLS pointer into CPU state.
        ls.append(mov_st_(
            reg::rax[offsetof(cpu_state, thread_data)],
            reg::rbx));

        const unsigned stub_target_offset(
            reinterpret_cast<uintptr_t>(&(state->restore_stub_target))
          - reinterpret_cast<uintptr_t>(state));


        // Get our target address.
        ls.append(mov_ld_(
            reg::rax,
            reg::rbx[stub_target_offset]));

        // Save target as return address of the stub.
        ls.append(mov_st_(reg::rsp[24], reg::rax));

        ls.append(pop_(reg::rbx));
        ls.append(pop_(reg::rax));
        ls.append(popf_());

        // If we're interrupted here, then we've already restored the
        // thread-local data, and we will go through the process all over
        // again; however, this is fine because the interrupt won't clobber
        // the stack, and the eventual target address is safely located on the
        // stack.
        ls.append(ret_());

        unsigned size(ls.encoded_size());

        state->restore_stub = unsafe_cast<app_pc>(
            global_state::FRAGMENT_ALLOCATOR->allocate_untyped(16, size));

        ls.encode(state->restore_stub);

        return state->restore_stub;
    }


    /// Mangle a delayed instruction.
    ///
    /// We make several basic assumptions:
    ///     i)   Code in a delay region does not fault.
    ///     ii)  Eliding cli/sti in a delay region is safe because the code
    ///          within the delay region (given assumption 1) must have been
    ///          interrupted before the 'cli' or after the 'sti'.
    ///     iii) If a delay region contains control-flow instructions then
    ///          the targets of those instructions will remain within the
    ///          delay region. This restrics delay regions to not containing
    ///          any indirect control-flow instructions, as correctness cannot
    ///          be guaranteed for such instructions.
    ///
    /// Mangling this code is tricky for two reasons:
    ///     i)  POPFs cannot be allowed to restore interrupts, as code in
    ///         a delayed interrupt region is not re-entrant.
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
        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {
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
    static app_pc emit_delayed_interrupt(
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
            instruction next_in;

            for(instruction in(ls.first());
                num_ctis-- && in.is_valid();
                in = next_in) {
                next_in = in.next();

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

        // Emit code to re-enable interrupts (when the flags are restored),
        // assuming they were previously enabled.
        //
        // TODO: this assumes that code in a delay region will not fault!!!
        if(isf->flags.interrupt) {

            // Disable interrupts upon return from the interrupt that caused the
            // delay.
            isf->flags.interrupt = false;

            // TODO: maybe XOR with old flags?
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


    extern "C" {
        DONT_OPTIMISE void granary_break_on_interrupt(
            granary::interrupt_stack_frame *isf,
            interrupt_vector vector,
            cpu_state_handle cpu
        ) {
            USED(isf);
            USED(vector);
            USED(cpu);
        }


        DONT_OPTIMISE void granary_break_on_bad_interrupt(
            granary::interrupt_stack_frame *isf,
            interrupt_vector vector,
            cpu_state_handle cpu
        ) {
            USED(isf);
            USED(vector);
            USED(cpu);
        }


        DONT_OPTIMISE void granary_break_on_nested_interrupt(
            granary::interrupt_stack_frame *isf,
            interrupt_vector vector,
            cpu_state_handle cpu,
            unsigned prev_num_nested_interrupts
        ) {
            USED(isf);
            USED(vector);
            USED(cpu);
            USED(prev_num_nested_interrupts);
        }
    }


    /// Handle an interrupt in the code cache. This has two responsibilities:
    ///
    ///     1)  Decide if we need to delay the interrupt. If so, then delay the
    ///         delay the interrupt.
    ///     2)  Otherwise, defer to the kernel to handle the interrupt.
    __attribute__((hot))
    static interrupt_handled_state handle_code_cache_interrupt(
        cpu_state_handle cpu,
        thread_state_handle thread,
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw() {
        basic_block bb(isf->instruction_pointer);
        app_pc delay_begin(nullptr);
        app_pc delay_end(nullptr);

        // We need to delay. After the delay has occurred, we re-issue the
        // interrupt.
        if(bb.get_interrupt_delay_range(delay_begin, delay_end)) {
            isf->instruction_pointer = emit_delayed_interrupt(
                cpu, isf, vector, delay_begin, delay_end);

            return INTERRUPT_RETURN;

        // We don't need to delay; let the client try to handle the
        // interrupt, or defer to the kernel if the client doesn't handle
        // the interrupt.
        } else {
#if CONFIG_CLIENT_HANDLE_INTERRUPT
            basic_block_state *bb_state(bb.state());
            instrumentation_policy policy(bb.policy);

            return policy.handle_interrupt(
                cpu, thread, *bb_state, *isf, vector);

#else
            return INTERRUPT_DEFER;
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */
        }
    }


    /// Handle an interrupt in a gencode region. Here, we just try to detect
    /// bad cases.
    __attribute__((hot))
    static interrupt_handled_state handle_gencode_interrupt(
        cpu_state_handle cpu,
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw() {
        const app_pc pc(isf->instruction_pointer);

        // Detect an exception within a delayed interrupt handler. This
        // is really bad.
        //
        // TODO: if the vector is an exception, then perhaps an okay
        //       strategy is to force execution to the next instruction.
        const app_pc delayed_begin(cpu->interrupt_delay_handler);
        const app_pc delayed_end(delayed_begin + INTERRUPT_DELAY_CODE_SIZE);
        if(delayed_begin <= pc && pc < delayed_end) {
            IF_PERF( perf::visit_recursive_interrupt(); )
            granary_break_on_interrupt(isf, vector, cpu);
            return INTERRUPT_IRET;
        }

        // Detect if an exception or something else is occurring within our
        // common interrupt handler. This is really bad.
        if(COMMON_HANDLER_BEGIN <= pc && pc < COMMON_HANDLER_END) {
            granary_break_on_interrupt(isf, vector, cpu);
            return INTERRUPT_DEFER;
        }

        return INTERRUPT_DEFER;
    }


    /// Handle an interrupt in kernel code (this includes native modules). This
    /// attempts to discover a few error conditions
    __attribute__((hot))
    static interrupt_handled_state handle_kernel_interrupt(
        cpu_state_handle cpu,
        thread_state_handle thread,
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw() {
#if CONFIG_CLIENT_HANDLE_INTERRUPT
        return client::handle_kernel_interrupt(
            cpu,
            thread,
            *isf,
            vector);
#else
        return INTERRUPT_DEFER;
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */
    }


    /// Handle an interrupt in page-protected module (app) code. This transfers
    /// us into the code cache under the default starting policy for the client
    /// as a recovery mechanism from the fault.
    __attribute__((hot))
    static interrupt_handled_state handle_module_interrupt(
        cpu_state_handle cpu,
        interrupt_stack_frame *isf
    ) throw() {
        granary_break_on_interrupt(isf, VECTOR_PAGE_FAULT, cpu);

        instrumentation_policy policy(START_POLICY);
        policy.in_host_context(false);
        policy.force_attach(true);

        mangled_address target(isf->instruction_pointer, policy);
        app_pc translated_target(nullptr);
        if(!cpu->code_cache.load(target.as_address, translated_target)) {
            translated_target = code_cache::find(cpu, target);
        }

        isf->instruction_pointer = translated_target;
        return INTERRUPT_IRET;
    }


    /// Handle an interrupt.
    GRANARY_ENTRYPOINT
    __attribute__((hot))
    interrupt_handled_state handle_interrupt(
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw() {
        kernel_preempt_disable();

        cpu_state_handle cpu;
        thread_state_handle thread(cpu);

        // Used to determine the TLS data that we might need to restore when
        // we've handled this interrupt. Stored "locally" so that if an
        // interrupt handler is interrupted then eventually we'll distinguish
        // TLS data in threads and TLS data in interrupt handlers.
        thread_state *restore_thread_state(cpu->thread_data);
        cpu->thread_data = nullptr;

        const unsigned prev_num_nested_interrupts(
            cpu->num_nested_interrupts.fetch_add(1));

        // Avoid nested interrupts clobbering Granary's internal data
        // structures.
        if(!prev_num_nested_interrupts) {
            granary::enter(cpu);
        } else {
            granary_break_on_nested_interrupt(
                isf, vector, cpu, prev_num_nested_interrupts);
        }

        app_pc pc(isf->instruction_pointer);

        IF_PERF( perf::visit_interrupt(); )

        interrupt_handled_state ret;

        // An interrupt that we have no idea how to handle.
        if(!is_valid_address(pc)) {
            granary_break_on_bad_interrupt(isf, vector, cpu);
            ret = INTERRUPT_DEFER;

        // In the code cache, defer to a client if necessary, otherwise default
        // to deferring to the kernel.
        } else if(is_code_cache_address(pc)) {
            ret = handle_code_cache_interrupt(cpu, thread, isf, vector);

        /// An interrupt in some automatically generated, non-instrumented
        /// code. These interrupts are either ignored (common), or indicate a
        /// very serious concern (uncommon).
        } else if(is_wrapper_address(pc) || is_gencode_address(pc)) {
            ret = handle_gencode_interrupt(cpu, isf, vector);

        // App addresses should be marked as non-executable, so we should try
        // to recover. This is an instance where we are likely missing a
        // wrapper.
        } else if(is_app_address(pc)) {
            if(VECTOR_PAGE_FAULT == vector) {
                IF_PERF( perf::visit_protected_module() );
                ret = handle_module_interrupt(cpu, isf);
            } else {
                ret = INTERRUPT_DEFER;
            }

        // Assume it's an interrupt in a host-address location.
        } else {
            ret = handle_kernel_interrupt(cpu, thread, isf, vector);
        }

        // We're deferring to the kernel, and the thread might resumed on a
        // different CPU.
        if(INTERRUPT_DEFER == ret) {
            if(restore_thread_state) {
                isf->instruction_pointer = get_tls_restore_stub(
                    restore_thread_state, isf->instruction_pointer);
            }

        // We're returning to the same CPU, so leave things as-is.
        } else {
            cpu->thread_data = restore_thread_state;
        }

        // Reset the stack pointer for this CPU.
        cpu->num_nested_interrupts.fetch_sub(1);
        kernel_preempt_enable();

        return ret;
    }


    /// Emit an interrupt entry routine. This routine dispatches to a common
    /// vector entry routine, which does proper handling of the interrupt.
    static app_pc emit_interrupt_routine(
        unsigned vector_num,
        app_pc original_routine,
        app_pc common_interrupt_routine
    ) throw() {
        instruction_list ls;

        // This makes it convenient to find top of the ISF from the common
        // interrupt handler.
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

        // Save arg1 for later (likely clobbered by handle_interrupt) so that
        // if we are going to RET from the interrupt (instead of IRET), we can
        // do so regardless of whether the interrupt has an error code or not.
        ls.append(push_(isf_ptr));

        // Save the return value as it will be clobbered by handle_interrupt
        ls.append(push_(reg::ret));

        // Check for a privilege level change: OR together CS and SS, then
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

        // Get ready to switch stacks and call out to the handler
        instruction in(save_and_restore_registers(rm, ls, in_kernel));
        in = insert_align_stack_after(ls, in);

        // Call the interrupt handler while on the private stack
        in = insert_cti_after(
                ls, in,
                unsafe_cast<app_pc>(granary_enter_private_stack),
                true, reg::ret,
                CTI_CALL);

        // Call handler 
        in = insert_cti_after(
            ls, in, // instruction
            unsafe_cast<app_pc>(handle_interrupt), // target
            true, reg::ret, // clobber reg
            CTI_CALL);

        // Need to save the return value for checking (after we exit the private stack again)
        ls.append(mov_ld_(isf_ptr, reg::ret));

        // Switch back to original stack
        in = insert_cti_after(
                ls, in, unsafe_cast<app_pc>(granary_exit_private_stack),
                true, reg::ret, CTI_CALL);

        insert_restore_old_stack_alignment_after(ls, in);

        // check to see if the interrupt was handled or not
        ls.append(xor_(reg::ret, reg::ret));
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
            ls.append(lea_(reg::rsp, reg::rsp[16])); // align to base of ISF
            ls.append(iret_());
        }

        // CASE 2.2: Return from the interrupt. This is tricky because the
        // interrupt stack frame might not be adjacent to the previous value of
        // the stack pointer (at the time of the interrupt). We will RET to the
        // return address in the ISF, which has been manipulated to be in the
        // position of previous stack pointer plus one. This is also tricky
        // because it must be safe w.r.t. NMIs.
        //
        // TODO: unaligned RET issue? Potentially consider RETn.
        ls.append(ret);
        {

            ls.append(pop_(isf_ptr));
            ls.append(push_(reg::rax)); // will serve as a temp stack ptr

            // Use RAX as a stack pointer.
            ls.append(mov_ld_(
                reg::rax,
                isf_ptr[offsetof(interrupt_stack_frame, stack_pointer)]));

            // Put the return address on the stack.
            // TODO: unaligned RET?
            ls.append(mov_ld_(
                reg::arg2,
                isf_ptr[offsetof(interrupt_stack_frame, instruction_pointer)]));
            ls.append(mov_st_(reg::rax[-8], reg::arg2));

            // Put the flags on the stack.
            ls.append(mov_ld_(
                reg::arg2,
                isf_ptr[offsetof(interrupt_stack_frame, flags)]));
            ls.append(mov_st_(reg::rax[-16], reg::arg2));

            // Move the saved rax to a different location.
            ls.append(mov_ld_(reg::arg2, *reg::rsp));
            ls.append(mov_st_(reg::rax[-24], reg::arg2));

            // Compute the new stack pointer, restore arg1, arg2, rsp, and rax.
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
        const unsigned size(ls.encoded_size());
        app_pc routine(reinterpret_cast<app_pc>(
            global_state::FRAGMENT_ALLOCATOR-> \
                allocate_untyped(CACHE_LINE_SIZE, size)));

        ls.encode(routine);

        COMMON_HANDLER_BEGIN = routine;
        COMMON_HANDLER_END = routine + size;

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

                app_pc target(emit_interrupt_routine(
                    i,
                    native_handler,
                    common_vector_handler));

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
