/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_auto.cc
 *
 *  Created on: 2013-05-29
 *      Author: Peter Goodman
 */

extern "C" {
    extern void granary_wp_auto_instructions_begin(void);
    extern void granary_wp_auto_instructions_end(void);
}

#include "granary/test.h"

#if CONFIG_RUN_TEST_CASES

#include "clients/watchpoints/policies/null_policy.h"
#include "clients/watchpoints/tests/pp.h"


namespace test {


    /// Code generation configuration.
    struct test_config {

        /// True iff a an error was detected when inspecting operands.
        bool found_operand_error;

        /// Information about the instruction.
        granary::app_pc test_in_addr;
        unsigned num_address_regs;

        /// Machine state. Represents both input and ouput machine state to the
        /// test.
        granary::simple_machine_state regs;
        granary::eflags flags;

        __attribute__((aligned(16))) uint64_t mem_value[4];

        /// The code for this test case.
        granary::app_pc exec_code;

        /// Configuration options that guide how to generate executable code.
        granary::register_manager dead_regs;
        bool instrument;
        bool carry_flag_live_after;
    };


    /// Visit the operand.
    static void visit_operand(
        const granary::operand_ref &ref,
        test_config &state
    ) throw() {
        using namespace granary;

        if(dynamorio::BASE_DISP_kind != ref->kind) {
            return;
        }

        const dynamorio::reg_id_t base_reg(ref->value.base_disp.base_reg);
        const dynamorio::reg_id_t index_reg(ref->value.base_disp.index_reg);
        const dynamorio::reg_id_t base_reg_64(
            register_manager::scale(base_reg, REG_64));
        const dynamorio::reg_id_t index_reg_64(
            register_manager::scale(index_reg, REG_64));

        // Make sure we've got at least one 64-bit register.
        if(base_reg) {
            state.found_operand_error = base_reg != base_reg_64;
        } else {
            state.found_operand_error = index_reg != index_reg_64;
        }

        if(state.found_operand_error) {
            return;
        }

        if(base_reg) {
            uintptr_t addr(reinterpret_cast<uintptr_t>(
                &(state.mem_value[state.num_address_regs])));

            if(index_reg == base_reg) {
                state.regs[base_reg].value_64 = addr / 2;
                state.regs[index_reg].value_64 = addr / 2;
            } else {
                state.regs[base_reg].value_64 = addr;
            }

        } else if(index_reg) {
            state.regs[index_reg].value_64 = \
                reinterpret_cast<uintptr_t>(
                    &(state.mem_value[state.num_address_regs]));
        }

        state.num_address_regs += 2;
    }


    /// Generate the code to run a test case instruction.
    static void generate_code(
        test_config &config
    ) throw() {
        using namespace granary;

        static basic_block_state fake_bb_state;

        instruction_list ls;

        // Load in the desired flags.
        ls.append(push_(reg::rax));
        ls.append(mov_imm_(reg::rax, int64_(reinterpret_cast<uintptr_t>(
            config.flags.value))));
        ls.append(push_(reg::rax));
        ls.append(popf_());
        ls.append(pop_(reg::rax));

        // Add in the instruction that we want to instrument.
        app_pc in_addr(config.test_in_addr);
        ls.append(instruction::decode(&in_addr));


        // Kill some registers, if necessary.
        dynamorio::reg_id_t dead_reg(config.dead_regs.get_zombie());
        for(; dead_reg; dead_reg = config.dead_regs.get_zombie()) {
            ls.append(mov_imm_(operand(dead_reg), int64_(0)));
        }

        // Read from the carry flag, if necessary.
        if(config.carry_flag_live_after) {
            instruction target(label_());
            ls.append(jb_(instr_(target)));
            ls.append(target);
        }

        // Revive all the registers.
        instruction target(label_());
        ls.append(jmp_(instr_(target)));
        ls.append(target);

        // Apply the watchpoint null instrumentation, if necessary.
        if(config.instrument) {
            instrumentation_policy policy = \
                policy_for<client::watchpoint_null_policy>();
            cpu_state_handle cpu;
            thread_state_handle thread;
            policy.instrument(cpu, thread, fake_bb_state, ls);
        }

        // Initialise the desired machine state from the registers. This is
        // done after potentially watching the previous memory instruction so
        // that this part is not affected by the watchpoint instrumentation.
        ls.prepend(mov_ld_(
            reg::rax,reg::rax[offsetof(simple_machine_state, rax)]));
        ls.prepend(mov_ld_(
            reg::rcx, reg::rax[offsetof(simple_machine_state, rcx)]));
        ls.prepend(mov_ld_(
            reg::rdx, reg::rax[offsetof(simple_machine_state, rdx)]));
        ls.prepend(mov_ld_(
            reg::rbx, reg::rax[offsetof(simple_machine_state, rbx)]));
        ls.prepend(mov_ld_(
            reg::rbp, reg::rax[offsetof(simple_machine_state, rbp)]));
        ls.prepend(mov_ld_(
            reg::rsi, reg::rax[offsetof(simple_machine_state, rsi)]));
        ls.prepend(mov_ld_(
            reg::rdi, reg::rax[offsetof(simple_machine_state, rdi)]));
        ls.prepend(mov_ld_(
            reg::r8, reg::rax[offsetof(simple_machine_state, r8)]));
        ls.prepend(mov_ld_(
            reg::r9, reg::rax[offsetof(simple_machine_state, r9)]));
        ls.prepend(mov_ld_(
            reg::r10, reg::rax[offsetof(simple_machine_state, r10)]));
        ls.prepend(mov_ld_(
            reg::r11, reg::rax[offsetof(simple_machine_state, r11)]));
        ls.prepend(mov_ld_(
            reg::r12, reg::rax[offsetof(simple_machine_state, r12)]));
        ls.prepend(mov_ld_(
            reg::r13, reg::rax[offsetof(simple_machine_state, r13)]));
        ls.prepend(mov_ld_(
            reg::r14, reg::rax[offsetof(simple_machine_state, r14)]));
        ls.prepend(mov_ld_(
            reg::r15, reg::rax[offsetof(simple_machine_state, r15)]));
        ls.prepend(mov_imm_(reg::rax, int64_(reinterpret_cast<uintptr_t>(
            &(config.regs)))));

        // Store the machine state as the "output".
        ls.append(push_(reg::rax));

        // Output the flags.
        ls.append(mov_imm_(reg::rax, int64_(reinterpret_cast<uintptr_t>(
            &(config.flags.value)))));
        ls.append(pushf_());
        ls.append(pop_(*reg::rax));

        // Output the register state.
        ls.append(mov_imm_(reg::rax, int64_(reinterpret_cast<uintptr_t>(
            &(config.regs)))));

        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, rcx)], reg::rcx));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, rdx)], reg::rdx));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, rbx)], reg::rbx));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, rbp)], reg::rbp));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, rsi)], reg::rsi));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, rdi)], reg::rdi));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, r8)], reg::r8));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, r9)], reg::r9));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, r10)], reg::r10));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, r11)], reg::r11));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, r12)], reg::r12));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, r13)], reg::r13));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, r14)], reg::r14));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, r15)], reg::r15));

        ls.append(pop_(reg::rcx));
        ls.append(mov_st_(
            reg::rax[offsetof(simple_machine_state, rax)], reg::rcx));

        instruction first(label_());
        register_manager regs;
        regs.kill_all();

        // Save the register state before the function runs.
        ls.prepend(first);
        save_registers(regs, ls, first);
        restore_registers(regs, ls, ls.last());

        ls.append(ret_());

        ls.encode(config.exec_code);

        printf("Emitted code %p\n", config.exec_code);
    }


    /// Compare two output configurations given the same input configuration.
    static bool compare_output_configs(
        test_config &input_config,
        test_config &desired_output,
        test_config &actual_output
    ) throw() {
        using namespace granary;

        register_manager dead_regs(input_config.dead_regs);
        dynamorio::reg_id_t dead_reg(dead_regs.get_zombie());
        for(; dead_reg; dead_reg = dead_regs.get_zombie()) {
            actual_output.regs[dead_reg].value_64 = \
                desired_output.regs[dead_reg].value_64;
        }

        int cmp(memcmp(
            &(desired_output.regs),
            &(actual_output.regs),
            sizeof actual_output.regs));
        if(0 != cmp) {
            return false;
        }

        return true;
    }


    /// Test watchpoints on an individual instruction by testing various
    /// different configurations for the same instruction.
    static void test_instruction(granary::instruction in) throw() {
        using namespace granary;

        test_config config;
        test_config input_config;
        test_config output_config;

        memset(&config, 0, sizeof config);

        // Make sure the count register has a decent value just in case this
        // instruction has a REP prefix.
        config.regs[dynamorio::DR_REG_RCX].value_64 = 1ULL;

        in.for_each_operand(visit_operand, config);

        if(config.found_operand_error) {
            return;
        }

        config.flags = granary_load_flags();
        config.flags.clear_arithmetic_flags();
        config.test_in_addr = in.pc();
        config.mem_value[0] = 0xBEEFBEEFBEEFBEEFULL;
        config.mem_value[0] = 0xDEADDEADDEADDEADULL;
        config.dead_regs.revive_all();
        config.exec_code = global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(PAGE_SIZE);

        memcpy(&input_config, &config, sizeof config);
        void (*func)(void) = unsafe_cast<void (*)(void)>(config.exec_code);

        generate_code(config);
        func();
        memcpy(&output_config, &config, sizeof config);

        // Run a basic version that is instrumented.
        memcpy(&config, &input_config, sizeof config);
        config.instrument = true;
        generate_code(config);
        func();

        ASSERT(compare_output_configs(input_config, output_config, config));
    }

    /// Test that MOV instructions are correctly watched.
    static void test_watchpoints_on_instructions(void) {
        using namespace granary;

        app_pc begin(unsafe_cast<app_pc>(&granary_wp_auto_instructions_begin));
        app_pc end(unsafe_cast<app_pc>(&granary_wp_auto_instructions_end));
        app_pc pc(begin);
        for(; pc < end; ) {
            printf("Testing %p\n", pc);
            test_instruction(instruction::decode(&pc));
        }
    }

    ADD_TEST(test_watchpoints_on_instructions,
        "Check that memory instructions from the Linux kernel can correctly "
        "be instrumented.")
}

#endif /* CONFIG_RUN_TEST_CASES */
