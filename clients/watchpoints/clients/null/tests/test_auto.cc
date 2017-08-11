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

#define RUN_AUTO_TESTS 0

#if RUN_AUTO_TESTS && CONFIG_DEBUG_RUN_TEST_CASES

#include "clients/watchpoints/clients/null/instrument.h"
#include "clients/watchpoints/clients/null/tests/pp.h"


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
    ) {
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
    ) {
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

        // Read from the carry flag (revive it).
        if(config.carry_flag_live_after) {
            instruction target(label_());
            ls.append(jb_(instr_(target)));
            ls.append(target);

        // Write to the carry flag (kill it).
        } else {
            ls.append(bt_(reg::rax, int8_(0)));
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
            policy.instrument(cpu, fake_bb_state, ls);
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
    }


    /// Compare two output configurations given the same input configuration.
    static bool compare_output_configs(
        test_config &input_config,
        test_config &desired_output,
        test_config &actual_output
    ) {
        using namespace granary;

        register_manager dead_regs(input_config.dead_regs);
        dynamorio::reg_id_t dead_reg(dead_regs.get_zombie());

        // Compare the flags.
        if(!input_config.carry_flag_live_after) {
            actual_output.flags.carry = desired_output.flags.carry;
        }

        // Print out the flags state on a failure.
        if(actual_output.flags.value != desired_output.flags.value) {

            printf("Tested instruction: %p\n", input_config.test_in_addr);
            printf("Emitted code: %p\n", input_config.exec_code);

            printf("          desired  actual\n");
            printf("carry     %d       %d\n",
                desired_output.flags.carry, actual_output.flags.carry);
            printf("parity    %d       %d\n",
                desired_output.flags.parity, actual_output.flags.parity);
            printf("aux_carry %d       %d\n",
                desired_output.flags.aux_carry, actual_output.flags.aux_carry);
            printf("zero      %d       %d\n",
                desired_output.flags.zero, actual_output.flags.zero);
            printf("sign      %d       %d\n",
                desired_output.flags.sign, actual_output.flags.sign);
            return false;
        }

        // Compare the memory operands.
        int cmp(memcmp(
            &(desired_output.mem_value[0]),
            &(actual_output.mem_value[0]),
            (sizeof desired_output.mem_value[0]) * 4));

        // Print out the memory value differences of the failure.
        if(0 != cmp) {
            printf("Tested instruction: %p\n", input_config.test_in_addr);
            printf("Emitted code: %p\n", input_config.exec_code);

            printf("        desired      actual\n");
            printf("mem[0]  0x%016x  0x%016x\n",
                desired_output.mem_value[0],
                actual_output.mem_value[0]);
            printf("mem[1]  0x%016x  0x%016x\n",
                desired_output.mem_value[1],
                actual_output.mem_value[1]);
            printf("mem[2]  0x%016x  0x%016x\n",
                desired_output.mem_value[2],
                actual_output.mem_value[2]);
            printf("mem[3]  0x%016x  0x%016x\n",
                desired_output.mem_value[3],
                actual_output.mem_value[3]);
            return false;
        }

        // Force some registers to be equivalent.
        for(; dead_reg; dead_reg = dead_regs.get_zombie()) {
            actual_output.regs[dead_reg].value_64 = \
                desired_output.regs[dead_reg].value_64;
        }

        // Compare the registers.
        cmp = memcmp(
            &(desired_output.regs),
            &(actual_output.regs),
            sizeof actual_output.regs);

        // Pint out the register state of the failure.
        if(0 != cmp) {
            printf("Tested instruction: %p\n", input_config.test_in_addr);
            printf("Emitted code: %p\n", input_config.exec_code);
            printf("      desired       actual\n");
            printf("rax:  0x%016x  0x%016x\n",
                desired_output.regs.rax, actual_output.regs.rax);
            printf("rcx:  0x%016x  0x%016x\n",
                desired_output.regs.rcx, actual_output.regs.rcx);
            printf("rdx:  0x%016x  0x%016x\n",
                desired_output.regs.rdx, actual_output.regs.rdx);
            printf("rbx:  0x%016x  0x%016x\n",
                desired_output.regs.rbx, actual_output.regs.rbx);
            printf("rbp:  0x%016x  0x%016x\n",
                desired_output.regs.rbp, actual_output.regs.rbp);
            printf("rsi:  0x%016x  0x%016x\n",
                desired_output.regs.rsi, actual_output.regs.rsi);
            printf("rdi:  0x%016x  0x%016x\n",
                desired_output.regs.rdi, actual_output.regs.rdi);
            printf("r8:   0x%016x  0x%016x\n",
                desired_output.regs.r8, actual_output.regs.r8);
            printf("r9:   0x%016x  0x%016x\n",
                desired_output.regs.r9, actual_output.regs.r9);
            printf("r10:  0x%016x  0x%016x\n",
                desired_output.regs.r10, actual_output.regs.r10);
            printf("r11:  0x%016x  0x%016x\n",
                desired_output.regs.r11, actual_output.regs.r11);
            printf("r12:  0x%016x  0x%016x\n",
                desired_output.regs.r12, actual_output.regs.r12);
            printf("r13:  0x%016x  0x%016x\n",
                desired_output.regs.r13, actual_output.regs.r13);
            printf("r14:  0x%016x  0x%016x\n",
                desired_output.regs.r14, actual_output.regs.r14);
            printf("r15:  0x%016x  0x%016x\n",
                desired_output.regs.r15, actual_output.regs.r15);
            return false;
        }

        return true;
    }


    /// Run a test.
    static void run_test(
        test_config &input_config,
        test_config &desired_output,
        test_config &actual_output
    ) {
        using namespace granary;
        test_config restore;

        memcpy(&restore, &input_config, sizeof input_config);

        void (*func)(void) = unsafe_cast<void (*)(void)>(input_config.exec_code);

        // Get the expected output and test it against the basic instrumented
        // output.
        desired_output.instrument = false;
        generate_code(input_config);
        func();
        memcpy(&desired_output, &input_config, sizeof actual_output);

        // Run a basic version that is instrumented.
        memcpy(&input_config, &restore, sizeof input_config);
        actual_output.instrument = true;
        generate_code(input_config);
        func();
        memcpy(&actual_output, &input_config, sizeof actual_output);

        ASSERT(compare_output_configs(
            input_config, desired_output, actual_output));

        memcpy(&input_config, &restore, sizeof actual_output);
    }


    /// Test watchpoints on an individual instruction by testing various
    /// different configurations for the same instruction.
    static void test_instruction(
        granary::instruction in,
        granary::app_pc exec_code
    ) {
        using namespace granary;

        test_config input_config;
        test_config desired_output;
        test_config actual_output;

        memset(&input_config, 0, sizeof input_config);

        // Make sure the count register has a decent value just in case this
        // instruction has a REP prefix.
        input_config.regs[dynamorio::DR_REG_RCX].value_64 = 1ULL;

        in.for_each_operand(visit_operand, input_config);

        if(input_config.found_operand_error) {
            return;
        }

        input_config.flags = granary_load_flags();
        input_config.flags.clear_arithmetic_flags();
        input_config.test_in_addr = in.pc();
        input_config.mem_value[0] = 0xBEEFBEEFBEEFBEEFULL;
        input_config.mem_value[0] = 0xDEADDEADDEADDEADULL;
        input_config.exec_code = exec_code;
        printf("Testing %p\n", in.pc());

        // Run 1.1
        input_config.dead_regs.revive_all();
        input_config.carry_flag_live_after = true;
        run_test(input_config, desired_output, actual_output);

        // Run 1.2
        input_config.dead_regs.revive_all();
        input_config.dead_regs.kill(in);
        input_config.carry_flag_live_after = true;
        run_test(input_config, desired_output, actual_output);

        // Run 1.3
        input_config.dead_regs.revive_all();
        input_config.dead_regs.kill_sources(in);
        input_config.carry_flag_live_after = true;
        run_test(input_config, desired_output, actual_output);

        // Run 1.4
        input_config.dead_regs.revive_all();
        input_config.dead_regs.kill_dests(in);
        input_config.carry_flag_live_after = true;
        run_test(input_config, desired_output, actual_output);

        // Run 2.1
        input_config.dead_regs.revive_all();
        input_config.carry_flag_live_after = false;
        run_test(input_config, desired_output, actual_output);

        // Run 2.2
        input_config.dead_regs.revive_all();
        input_config.dead_regs.kill(in);
        input_config.carry_flag_live_after = false;
        run_test(input_config, desired_output, actual_output);

        // Run 2.3
        input_config.dead_regs.revive_all();
        input_config.dead_regs.kill_sources(in);
        input_config.carry_flag_live_after = false;
        run_test(input_config, desired_output, actual_output);

        // Run 2.4
        input_config.dead_regs.revive_all();
        input_config.dead_regs.kill_dests(in);
        input_config.carry_flag_live_after = false;
        run_test(input_config, desired_output, actual_output);

        // Run 3.1
        input_config.dead_regs.kill_all();
        input_config.carry_flag_live_after = true;
        run_test(input_config, desired_output, actual_output);

        // Run 3.2
        input_config.dead_regs.kill_all();
        input_config.dead_regs.revive(in);
        input_config.carry_flag_live_after = true;
        run_test(input_config, desired_output, actual_output);

        // Run 3.3
        input_config.dead_regs.kill_all();
        input_config.dead_regs.revive_sources(in);
        input_config.carry_flag_live_after = true;
        run_test(input_config, desired_output, actual_output);

        // Run 3.3
        input_config.dead_regs.kill_all();
        input_config.dead_regs.revive_dests(in);
        input_config.carry_flag_live_after = true;
        run_test(input_config, desired_output, actual_output);

        // Run 4.1
        input_config.dead_regs.kill_all();
        input_config.carry_flag_live_after = false;
        run_test(input_config, desired_output, actual_output);

        // Run 4.2
        input_config.dead_regs.kill_all();
        input_config.dead_regs.revive(in);
        input_config.carry_flag_live_after = false;
        run_test(input_config, desired_output, actual_output);

        // Run 4.3
        input_config.dead_regs.kill_all();
        input_config.dead_regs.revive_sources(in);
        input_config.carry_flag_live_after = false;
        run_test(input_config, desired_output, actual_output);

        // Run 4.4
        input_config.dead_regs.kill_all();
        input_config.dead_regs.revive_dests(in);
        input_config.carry_flag_live_after = false;
        run_test(input_config, desired_output, actual_output);
    }


    /// Test that instructions derived from the Linux kernel, Python, Clang, and
    /// KCacheGrind are correctly instrumented.
    static void test_watchpoints_on_instructions(void) {
        using namespace granary;

        app_pc begin(unsafe_cast<app_pc>(&granary_wp_auto_instructions_begin));
        app_pc end(unsafe_cast<app_pc>(&granary_wp_auto_instructions_end));
        app_pc pc(begin);
        app_pc exec_code(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(PAGE_SIZE));
        cpu_state_handle cpu;

        for(; pc < end; ) {
            granary::enter(cpu);
            test_instruction(instruction::decode(&pc), exec_code);
        }
    }


    ADD_TEST(test_watchpoints_on_instructions,
        "Check that memory instructions from the Linux kernel can correctly "
        "be instrumented.")
}

#endif /* CONFIG_DEBUG_RUN_TEST_CASES */
