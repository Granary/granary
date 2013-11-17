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
    static app_pc NATIVE_VECTOR_HANDLER[256] = {nullptr};
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
            instruction first(ls.append(label_()));
            ls.append(push_(reg::rax));
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


#if CONFIG_FEATURE_INTERRUPT_DELAY
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

        // Redirect internal CTIs.
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

        IF_TEST( memset(
            cpu->interrupt_delay_handler, 0xCC, INTERRUPT_DELAY_CODE_SIZE); )
        ls.encode(cpu->interrupt_delay_handler, ls.encoded_size());

        return delay_in.pc();
    }
#endif /* CONFIG_FEATURE_INTERRUPT_DELAY */


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


        DONT_OPTIMISE void granary_break_on_gp_interrupt(
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
            cpu_state_handle cpu
        ) {
            USED(isf);
            USED(vector);
            USED(cpu);
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
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw() {

        // Need to access the basic block's info for both interrupt delaying and
        // checking if we might need to deal with an exception table entry.
        basic_block bb(isf->instruction_pointer);

        // Try to prepare for us being in a place where the kernel can
        // validly take a page fault.
        if(VECTOR_PAGE_FAULT == vector
        && bb.info->user_exception_metadata) {
            cpu->last_exception_instruction_pointer = isf->instruction_pointer;
            cpu->last_exception_table_entry = bb.info->user_exception_metadata;
        }

#if !CONFIG_FEATURE_INTERRUPT_DELAY && !CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
        // We don't need to do anything specific for interrupts.
        return INTERRUPT_DEFER;

#else
#   if CONFIG_FEATURE_INTERRUPT_DELAY
        // We might need to do something specific for interrupts.
        app_pc delay_begin(nullptr);
        app_pc delay_end(nullptr);

        // We need to delay. After the delay has occurred, we re-issue the
        // interrupt.
        if(bb.get_interrupt_delay_range(delay_begin, delay_end)) {
            granary::enter(cpu);
            isf->instruction_pointer = emit_delayed_interrupt(
                cpu, isf, vector, delay_begin, delay_end);

            return INTERRUPT_RETURN;
        }
#   endif /* CONFIG_FEATURE_INTERRUPT_DELAY */

        // We don't need to delay; let the client try to handle the
        // interrupt, or defer to the kernel if the client doesn't handle
        // the interrupt.
#   if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
        granary::enter(cpu);
        instrumentation_policy policy(bb.policy);
        return policy.handle_interrupt(
            cpu, thread_state_handle(cpu), *bb.state(), *isf, vector);

#   else
        return INTERRUPT_DEFER;
#   endif /* CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT */
#endif /* CONFIG_FEATURE_INTERRUPT_DELAY || CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT */
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

#if CONFIG_FEATURE_INTERRUPT_DELAY
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
#endif /* CONFIG_FEATURE_INTERRUPT_DELAY */

        // Detect if an exception or something else is occurring within our
        // common interrupt handler. This is expected on the emulated IRET path
        // out of the code cache (when we do a RET). If it doesn't happen
        // right after the POPF before the RET then it's a likely a serious
        // issue.
        if(COMMON_HANDLER_BEGIN <= pc && pc < COMMON_HANDLER_END) {
            granary_break_on_interrupt(isf, vector, cpu);
            return INTERRUPT_DEFER;
        }

        return INTERRUPT_DEFER;
    }


#if CONFIG_DEBUG_ASSERTIONS
    bool HAS_FAULTED_STACK = false;
    unsigned FAULTED_STACK_BASE_INDEX = 0;
    void *FAULTED_STACK[4096 / 8] = {0};
#endif

    /// Handle an interrupt in kernel code (this includes native modules). This
    /// attempts to discover a few error conditions
    __attribute__((hot))
    static interrupt_handled_state handle_kernel_interrupt(
        cpu_state_handle cpu,
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw() {

#if CONFIG_DEBUG_ASSERTIONS
        // Used to try to debug when a granary_fault is called.
        if(VECTOR_BREAKPOINT == vector && !HAS_FAULTED_STACK) {
            HAS_FAULTED_STACK = true;
            uintptr_t rsp = reinterpret_cast<uintptr_t>(isf->stack_pointer);
            rsp += ALIGN_TO(rsp, 8);
            uintptr_t rsp_base = (rsp + 4096) & ~4095;
            FAULTED_STACK_BASE_INDEX = (rsp_base - rsp) / 8;
            memcpy(FAULTED_STACK, reinterpret_cast<void *>(rsp), rsp_base - rsp);
        }
#endif
#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
        granary::enter(cpu);
        return client::handle_kernel_interrupt(
            cpu,
            thread_state_handle(cpu),
            *isf,
            vector);
#else
        UNUSED(cpu);
        UNUSED(isf);
        UNUSED(vector);
        return INTERRUPT_DEFER;
#endif /* CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT */
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
        policy.begins_functional_unit(true);

        // Probably the kernel calling the module, so make sure it supports
        // direct return back into the kernel.
        policy.return_address_in_code_cache(true);

        mangled_address target(isf->instruction_pointer, policy);
        app_pc translated_target(nullptr);

        if(!cpu->code_cache.load(target.as_address, translated_target)) {
            granary::enter(cpu);
            translated_target = code_cache::find(cpu, target);
            cpu->code_cache.store(target.as_address, translated_target);
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

        cpu_state_handle cpu;

#if CONFIG_DEBUG_ASSERTIONS
        // If the high 48 bits of the two stack pointers are the same then
        // we hit a recursive interrupt; otherwise, mark us as entering into
        // Granary.
        const uintptr_t private_stack_check(
            reinterpret_cast<uintptr_t>(isf->stack_pointer) ^
            reinterpret_cast<uintptr_t>(cpu->percpu_stack.top));
        
        if(0 == (private_stack_check >> 16)) {
            granary_break_on_nested_interrupt(isf, vector, cpu);
        }
#endif

        app_pc pc(isf->instruction_pointer);

        IF_PERF( perf::visit_interrupt(); )

        interrupt_handled_state ret(INTERRUPT_DEFER);

        // An interrupt that we have no idea how to handle.
        if(!is_valid_address(pc)) {
            granary_break_on_bad_interrupt(isf, vector, cpu);
            ret = INTERRUPT_DEFER;

        // In the code cache, defer to a client if necessary, otherwise default
        // to deferring to the kernel.
        } else if(is_code_cache_address(pc)) {
            ret = handle_code_cache_interrupt(cpu, isf, vector);

        /// An interrupt in some automatically generated, non-instrumented
        /// code. These interrupts are either ignored (common), or indicate a
        /// very serious concern (uncommon).
        } else if(is_wrapper_address(pc) || is_gencode_address(pc)) {
            ret = handle_gencode_interrupt(cpu, isf, vector);

        // App addresses should be marked as non-executable, so we should try
        // to recover. This is an instance where we are likely missing a
        // wrapper.
        } else if(is_app_address(pc)) {

            /*
            if(VECTOR_PAGE_FAULT == vector) {
                IF_PERF( perf::visit_protected_module() );
                ret = handle_module_interrupt(cpu, isf);
            } else {
                ret = INTERRUPT_DEFER;
            }*/

            // TODO: Re-enable page-protecting the module code.
            ret = INTERRUPT_DEFER;

        // Assume it's an interrupt in a host-address location.
        } else {
            ret = handle_kernel_interrupt(cpu, isf, vector);
        }

        return ret;
    }


    /// Emit an interrupt entry routine. This routine dispatches to a common
    /// vector entry routine, which does proper handling of the interrupt.
    static app_pc emit_interrupt_routine(
        unsigned vector_num,
        app_pc original_routine,
        app_pc common_interrupt_routine
    ) throw() {
        cpu_state_handle cpu;
        instruction_list ls;
        const bool vec_has_error_code(has_error_code(vector_num));

        IF_TEST( cpu->in_granary = false; )
        cpu.free_transient_allocators();

        // This makes it convenient to find top of the ISF from the common
        // interrupt handler.
        ls.append(push_(reg::rsp));
        if(!vec_has_error_code) {
            ls.append(push_(seg::ss(*reg::rsp)));
        }

        ls.append(push_(reg::arg1));
        ls.append(push_(reg::arg2));
        ls.append(lea_(reg::arg1, seg::ss(reg::rsp[24])));
        ls.append(mov_imm_(reg::arg2, int64_(vector_num)));
        ls.append(pushf_()); // Save the flags; the kernel reads them.
        ls.append(call_(pc_(common_interrupt_routine)));
        ls.append(popf_());
        ls.append(pop_(reg::arg2));
        ls.append(pop_(reg::arg1));
        ls.append(pop_(reg::rsp));

        insert_cti_after(
            ls, ls.last(), original_routine,
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_JMP);

        const unsigned size(ls.encoded_size());
        app_pc routine(reinterpret_cast<app_pc>(
            global_state::FRAGMENT_ALLOCATOR-> \
                allocate_untyped(CACHE_LINE_SIZE, size)));
        ls.encode(routine, size);

        return routine;
    }


#if CONFIG_DEBUG_CPU_RESET
    extern "C" {
        extern void **kernel_get_cpu_state(void *[]);
    }
#endif


    /// Emit a common interrupt entry routine. This routine handles the full
    /// interrupt.
    static app_pc emit_common_interrupt_routine(void) throw() {

        if(COMMON_HANDLER_BEGIN) {
            return COMMON_HANDLER_BEGIN;
        }

        instruction_list ls;
        instruction in_kernel(label_());
        operand isf_ptr(reg::arg1);
        operand isf_ptr_32(reg::arg1_32);
        operand vector(reg::arg2);

        // Save arg1 for later (likely clobbered by handle_interrupt) so that
        // if we are going to RET from the interrupt (instead of IRET), we can
        // do so regardless of whether the interrupt has an error code or not.
        ls.append(push_(isf_ptr));

        // Save the return value as it will be clobbered by handle_interrupt
        ls.append(push_(reg::ret));

#if CONFIG_DEBUG_CPU_RESET
        // Check to see if a fault is coming from `kernel_get_cpu_state`,
        // specifically, the access of the %gs register in:
        //
        //    0x0000000000052280 <+0>:      push   %rbp
        //    0x0000000000052281 <+1>:      mov    %gs:0x0,%eax
        //    0x0000000000052289 <+9>:      cltq
        //    0x000000000005228b <+11>:     mov    %rsp,%rbp
        //    0x000000000005228e <+14>:     lea    (%rdi,%rax,8),%rax
        //    0x0000000000052292 <+18>:     pop    %rbp
        //    0x0000000000052293 <+19>:     retq
        //
        // If this function faults, then it will lead to a cascade of faults
        // as our interrupt handlers try to access CPU-private data using the
        // same mechanism. This cascade eventually results in a CPU reset.
        instruction not_in_func(label_());
        ls.append(mov_imm_(reg::ret,
            int64_(unsafe_cast<uint64_t>(kernel_get_cpu_state) + 1)));
        ls.append(cmp_(reg::ret,
            seg::ss(isf_ptr[offsetof(interrupt_stack_frame, instruction_pointer)])));
        ls.append(jnz_(instr_(not_in_func)));
        insert_cti_after(
            ls, ls.last(), unsafe_cast<app_pc>(granary_break_on_curiosity),
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_CALL);
        ls.append(not_in_func);
#endif

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

        // In kernel space.
        register_manager rm;
        rm.kill_all();
        rm.revive(isf_ptr);
        rm.revive(vector);
        rm.revive(reg::ret);

        // Restore callee-saved registers, because `handle_interrupt` will
        // save them for us (because it respects the ABI).
        IF_NOT_TEST(
            rm.revive(reg::rbx);
            rm.revive(reg::rbp);
            rm.revive(reg::r12);
            rm.revive(reg::r13);
            rm.revive(reg::r14);
            rm.revive(reg::r15);
        )

        // Get ready to switch stacks and call out to the handler.
        instruction in(save_and_restore_registers(rm, ls, in_kernel));

        //        !! AT THIS POINT ALL OTHER REGISTERS ARE SAVED !!
        //           BE CAREFUL, USE `insert_after` TO INSERT
        //           CODE INSIDE THE REGISTER SAVE REGION, AND
        //           APPEND TO INSERT CODE AFTER THE REGISTER
        //           RESTORE REGION.

        // Switch to the private stack (we might be on the private stack if this
        // is a nested interrupt).
        in = insert_cti_after(
            ls, in,
            unsafe_cast<app_pc>(granary_enter_private_stack),
            CTI_STEAL_REGISTER, reg::ret,
            CTI_CALL);

        // Call handler.
        in = insert_cti_after(
            ls, in, // instruction
            unsafe_cast<app_pc>(handle_interrupt), // target
            CTI_STEAL_REGISTER, reg::ret, // clobber reg
            CTI_CALL);

        // Need to save the return value for checking (after we exit the
        // private stack again).
        in = ls.insert_after(in, mov_ld_(isf_ptr, reg::ret));

        // Switch back to original stack.
        in = insert_cti_after(
            ls, in, unsafe_cast<app_pc>(granary_exit_private_stack),
            CTI_STEAL_REGISTER, reg::ret,
            CTI_CALL);

        //        !! AT THIS POINT ALL OTHER REGISTERS ARE RESTORED !!
        //           NOTE THE DIFFERENCE BETWEEN `insert_after` and
        //           APPEND.

        // Check to see if the interrupt was handled or not.
        ls.append(xor_(reg::ret_32, reg::ret_32));
        ls.append(cmp_(isf_ptr_32, reg::ret_32));

        // Restore reg::reg (%rax) to its native state.
        ls.append(pop_(reg::ret));

        instruction ret(label_());
        instruction defer(label_());

        ls.append(jl_(instr_(ret))); // (ret = INTERRUPT_RETURN)
        ls.append(jz_(instr_(defer))); // (ret = INTERRUPT_DEFER)

        // CASE 2.1: fall-through: IRET from the interrupt, it has been handled.
        {
            // 24 = sizeof( isf_ptr + return address + flags )
            ls.append(lea_(reg::rsp, reg::rsp[24]));
            ls.append(pop_(vector)); // arg2.
            ls.append(pop_(isf_ptr)); // arg1.

            // Align to base of ISF. In `emit_interrupt_routine`, we did:
            //      a) No error code:
            //          push %rsp;
            //          push (%rsp);
            //      b) Error code:
            //          push %rsp;
            // So aligning by 16 bytes to find the top of the ISF (excluding the
            // error code) is always valid because we guarantee that there will
            // always be 16 bytes of something there.
            ls.append(lea_(reg::rsp, reg::rsp[16]));
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
            ls.append(push_(reg::rax)); // Will serve as a temp stack ptr.

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
            ls.append(mov_ld_(reg::arg2, reg::rsp[24]));
            ls.append(mov_ld_(reg::arg1, reg::rsp[32]));
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
            // Note: reg::ret (rax) has already been popped.
            ls.append(lea_(reg::rsp, reg::rsp[8])); // isf_ptr (arg1)
            ls.append(ret_());
        }

        // Encode.
        const unsigned size(ls.encoded_size());
        app_pc routine(reinterpret_cast<app_pc>(
            global_state::FRAGMENT_ALLOCATOR-> \
                allocate_untyped(CACHE_LINE_SIZE, size)));

        ls.encode(routine, size);

        COMMON_HANDLER_BEGIN = routine;
        COMMON_HANDLER_END = routine + size;

        return routine;
    }


    /// Used to share IDTs across multiple CPUs.
    static system_table_register_t PREV_IDTR = {0, nullptr};
    static system_table_register_t PREV_IDTR_GEN = {0, nullptr};


    static detail::interrupt_descriptor_table GLOBAL_IDT;


    /// Create a Granary version of the interrupt descriptor table. This does
    /// not assume that all CPUs share the same IDT, but benefits from sharing
    /// if it's true.
    system_table_register_t create_idt(system_table_register_t native) throw() {

        // Unusual; seem to be re-creating based on our own IDT.
        if(native.base == PREV_IDTR_GEN.base) {
            return PREV_IDTR_GEN;
        }

        if(native.base == PREV_IDTR.base && native.limit == PREV_IDTR.limit) {
            return PREV_IDTR_GEN;
        }

        // TODO: Add support for multiple IDTs later.
        ASSERT(!PREV_IDTR.base);

        PREV_IDTR = native;

        system_table_register_t instrumented;
        detail::interrupt_descriptor_table *idt(&GLOBAL_IDT);
        detail::interrupt_descriptor_table *kernel_idt(
            unsafe_cast<detail::interrupt_descriptor_table *>(native.base));

        memset(idt, 0, sizeof *idt);

        if(kernel_idt) {
            memcpy(idt, kernel_idt, native.limit + 1);
        }

        app_pc common_vector_handler(emit_common_interrupt_routine());

        const unsigned num_vecs((native.limit + 1) / (2 * sizeof(descriptor_t)));
        for(unsigned i(0); i < num_vecs; ++i) {
            descriptor_t *i_vec(&(idt->vectors[i * 2]));
            descriptor_t *n_vec(&(native.base[i * 2]));

            // Update the gate.
            if(GATE_DESCRIPTOR == get_descriptor_kind(i_vec)) {
                app_pc native_handler(get_gate_target_offset(&(n_vec->gate)));

                if(!is_host_address(native_handler)) {
                    continue;
                }

                // We've already made a handler for this vector.
                if(native_handler == NATIVE_VECTOR_HANDLER[i]) {
                    ASSERT(nullptr != VECTOR_HANDLER[i]);
                    set_gate_target_offset(
                        &(i_vec->gate),
                        VECTOR_HANDLER[i]);
                    continue;
                }

                app_pc target(native_handler);

#if !CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT && !CONFIG_FEATURE_INTERRUPT_DELAY
                // If clients aren't handling interrupts, and we don't care
                // about delaying interrupts.
                if(VECTOR_PAGE_FAULT == i) {
#else
                // If clients are handling interrupts, or we want to be able to
                // delay interrupts.
                //
                // Magic number 0xf0: This covers most Linux x86 and ia64 IPI
                // interrupt vectors (as well as a few others, but whatever).
                //
                // TODO: If CONFIG_FEATURE_INSTRUMENT_HOST, deal with VECTOR_SYSCALL.
                //       For the time being this might not be necessary because
                //       `INT 0x80` is used for 32-bit applications (similar to
                //       `SYSENTER`).
                if(VECTOR_SYSCALL != i
                && VECTOR_NMI != i
                && i <= 0xf0) {
#endif
                    target = emit_interrupt_routine(
                        i,
                        native_handler,
                        common_vector_handler);
                }

                // Cache, just in case some CPUs use different IDTs.
                NATIVE_VECTOR_HANDLER[i] = native_handler;
                VECTOR_HANDLER[i] = target;

                // Set the target into our IDT.
                set_gate_target_offset(
                    &(i_vec->gate),
                    target);
            }
        }

        instrumented.base = &(idt->vectors[0]);
        instrumented.limit = native.limit;
        PREV_IDTR_GEN = instrumented;

        return instrumented;
    }
}
