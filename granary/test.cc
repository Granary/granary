/*
 * test.cc
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/test.h"
#include "granary/register.h"

extern "C" {


#if CONFIG_RUN_TEST_CASES
    __attribute__((noinline, optimize("O0")))
    int granary_test_return_true(void) {
        return 1;
    }

    __attribute__((noinline, optimize("O0")))
    int granary_test_return_false(void) {
        return 0;
    }
#endif

#if !GRANARY_IN_KERNEL

    __attribute__((noinline, optimize("O0")))
    void granary_break_on_fault(void) {
        ASM("");
    }


    __attribute__((noinline, optimize("O0")))
    int granary_fault(void) {
        ASM("mov 0, %rax;");
        return 1;
    }

#endif

}



namespace granary {

    static void debug_bb(app_pc *ret_addr, bool in_xmm_context) throw() {
        basic_block bb(*ret_addr);
        printf("bb native_pc = %p instrumented_pc = %p in xmm context = %d\n",
            bb.info->generating_pc,
            bb.cache_pc_start + (in_xmm_context ? 329 : 98),
            in_xmm_context);
        (void) ret_addr;
    }

#if 0
    static const char *reg_names[] = {
        "NULL",
        "RAX",
        "RCX",
        "RDX",
        "RBX",
        "RSP",
        "RBP",
        "RSI",
        "RDI",
        "R8",
        "R9",
        "R10",
        "R11",
        "R12",
        "R13",
        "R14",
        "R15"
    };


    template <dynamorio::reg_id_t reg_of_interest>
    static void print_reg_value(uint64_t new_val) throw() {
        printf("new value of %s is 0x%lx\n", reg_names[reg_of_interest], new_val);
    }


    template <dynamorio::reg_id_t reg_of_interest>
    static void track_register(
        instruction_list &ls,
        instruction in
    ) throw() {
        register_manager all_regs;
        all_regs.revive_all();
        all_regs.visit(in);

        bool reg_is_dead(false);
        for(;;) {
            dynamorio::reg_id_t dead_reg(all_regs.get_zombie());
            if(!dead_reg) {
                break;
            }

            if(dead_reg == reg_of_interest) {
                reg_is_dead = true;
                break;
            }
        }

        if(!reg_is_dead) {
            return;
        }

        all_regs.kill_all();

        IF_USER( in = ls.insert_after(in, lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )
        in = insert_save_flags_after(ls, in);

        instruction after_regs(ls.insert_after(in, label_()));

        // save register state
        in = save_and_restore_registers(all_regs, ls, in);
        in = save_and_restore_xmm_registers(all_regs, ls, in, XMM_SAVE_UNALIGNED);
        in = ls.insert_after(in, mov_ld_(reg::arg1, operand(reg_of_interest)));
        in = insert_align_stack_after(ls, in);
        in = insert_cti_after(
            ls,
            in,
            (app_pc) print_reg_value<reg_of_interest>,
            false,
            operand(),
            CTI_CALL
        );

        in = insert_restore_old_stack_alignment_after(ls, in);

        in = insert_restore_flags_after(ls, after_regs);
        IF_USER( in = ls.insert_after(in, lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )
    }
#endif

    /// Instruction a basic block.
    instrumentation_policy test_policy::visit_basic_block(
        cpu_state_handle &,
        thread_state_handle &,
        basic_block_state &,
        instruction_list &ls
    ) throw() {
#if 0
        instruction in(ls.first());
        instruction next;
        for(; in.is_valid(); in = next) {
            next = in.next();
            track_register<dynamorio::DR_REG_RDI>(ls, in);
        }
#endif
#if 0
        register_manager all_regs;
        all_regs.kill_all();

        if(!is_in_xmm_context()) {
            all_regs.revive_all_xmm();
        }

        instruction in(ls.prepend(granary::label_()));

        IF_USER( in = ls.insert_after(
            in, lea_(granary::reg::rsp, granary::reg::rsp[-REDZONE_SIZE])); )

        // save the flags
        in = ls.insert_after(in, granary::pushf_());

        // restore the flags
        instruction tail_in(ls.insert_after(in, granary::popf_()));

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

#if CONFIG_RUN_TEST_CASES

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
#endif
} /* granary */

