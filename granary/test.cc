/*
 * test.cc
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/test.h"

extern "C" {

    void granary_break_on_fault(void) { }

    int granary_fault(void) {
        ASM("mov 0, %rax;");
        return 1;
    }

    void granary_break_on_encode(
        dynamorio::app_pc pc,
        dynamorio::instr_t *instr
    ) {
        (void) pc;
        (void) instr;
        granary_fault();
    }

    void granary_break_on_bb(granary::basic_block *bb) {
        (void) bb;
    }

    void granary_break_on_allocate(void *ptr) {
        (void) ptr;
    }

    int granary_test_return_true(void) {
        return 1;
    }

    int granary_test_return_false(void) {
        return 0;
    }

    /// Hacks so that we don't need to link in libc++.
    int __cxa_guard_acquire(void) {
        return 1;
    }
    int __cxa_guard_release(void) {
        return 1;
    }
}

#if CONFIG_RUN_TEST_CASES

namespace granary {

    __attribute__((noinline, optimize("O0")))
    static void break_in_xmm_context(void) throw() {
        ASM("");
    }

    static void debug_bb(app_pc *ret_addr, bool in_xmm_context) throw() {
        granary::basic_block bb(*ret_addr);
        printf("bb native_pc = %p instrumented_pc = %p in xmm context = %d\n",
            bb.info->generating_pc,
            bb.cache_pc_start + (in_xmm_context ? 329 : 98),
            in_xmm_context);
        (void) ret_addr;
        if(in_xmm_context) {
            break_in_xmm_context();
        }
    }

    /// Instruction a basic block.
    instrumentation_policy test_policy::visit_basic_block(
        cpu_state_handle &,
        thread_state_handle &,
        basic_block_state &,
        instruction_list &ls
    ) throw() {
#if 1
        register_manager all_regs;
        all_regs.kill_all();

        if(!is_in_xmm_context()) {
            all_regs.revive_all_xmm();
        }

        instruction_list_handle in(ls.prepend(granary::label_()));

        IF_USER( in = ls.insert_after(
            in, lea_(granary::reg::rsp, granary::reg::rsp[-REDZONE_SIZE])); )

        // save the flags
        in = ls.insert_after(in, granary::pushf_());

        // restore the flags
        instruction_list_handle tail_in(ls.insert_after(in, granary::popf_()));

        IF_USER( tail_in = ls.insert_after(
                tail_in, lea_(granary::reg::rsp, granary::reg::rsp[REDZONE_SIZE])); )

        // save register state
        in = save_and_restore_registers(all_regs, ls, in);
        in = save_and_restore_xmm_registers(all_regs, ls, in, XMM_SAVE_UNALIGNED);

        // align the stack.
        in = ls.insert_after(in, granary::push_(granary::reg::rsp));
        in = ls.insert_after(in, granary::push_(granary::reg::rsp[0]));
        in = ls.insert_after(in,
            granary::and_(granary::reg::rsp, granary::int8_(-16)));

        // call debug_bb
        in = ls.insert_after(in, granary::lea_(reg::arg1, reg::rsp[-8]));
        in = ls.insert_after(in,
            granary::mov_imm_(reg::arg2, int32_(is_in_xmm_context())));

        in = ls.insert_after(in, granary::mov_imm_(
            granary::reg::rax, int64_(granary::unsafe_cast<uint64_t>(debug_bb))));

        in = ls.insert_after(in, granary::call_ind_(granary::reg::rax));
        in->set_mangled();

        // restore previous stack alignment
        in = ls.insert_after(in, mov_ld_(reg::rsp, reg::rsp[8]));


#endif
        (void) ls;
        return granary::policy_for<test_policy>();
    }


    /// List of test cases to run.
    static static_test_list STATIC_TEST_LIST_HEAD;


    /// Initialise the list of test functions to execute.
    static_test_list::static_test_list(void) throw()
        : func(nullptr)
        , desc(nullptr)
        , next(nullptr)
    { }


    void static_test_list::append(static_test_list &entry) throw() {
        entry.next = STATIC_TEST_LIST_HEAD.next;
        STATIC_TEST_LIST_HEAD.next = &entry;
    }


    void run_tests(void) throw() {
        static_test_list *test(STATIC_TEST_LIST_HEAD.next);
        for(; test; test = test->next) {
            if(test->func) {
                printf("Running test '%s'\n", test->desc);
                test->func();
            }
        }
    }
}
#endif

