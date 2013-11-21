/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * syscall.cc
 *
 *  Created on: 2013-09-23
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/instruction.h"
#include "granary/emit_utils.h"
#include "granary/state.h"
#include "granary/basic_block.h"

#include "clients/instrument.h"

namespace granary {


    /// Try to share syscall entry points across CPUs if they are common.
    static uint64_t LAST_SYSCALL_ENTRYPOINT = 0;
    static uint64_t LAST_GEN_SYSCALL_ENTRYPOINT = 0;


    /// Generate a system call entry point for the current CPU.
    uint64_t create_syscall_entrypoint(uint64_t native_msr_lstar) throw() {
        app_pc native_syscall_handler = unsafe_cast<app_pc>(native_msr_lstar);
        cpu_state_handle cpu;

        if(LAST_SYSCALL_ENTRYPOINT == native_msr_lstar) {
            ASSERT(0 != LAST_GEN_SYSCALL_ENTRYPOINT);
            return LAST_GEN_SYSCALL_ENTRYPOINT;
        }

        instruction_list ls;
        LAST_SYSCALL_ENTRYPOINT = native_msr_lstar;

        for(;;) {
            instruction in(instruction::decode(&native_syscall_handler));
            ls.append(in);

            // Found a point at which the stack is safe, i.e. the kernel has
            // switched from a user space stack to a kernel space stack.
            if(dynamorio::instr_writes_to_exact_reg(in, dynamorio::DR_REG_RSP)) {
                break;
            }
        }

        instrumentation_policy policy(START_POLICY);
        policy.in_host_context(true);
        policy.return_address_in_code_cache(true);
        policy.begins_functional_unit(true);

        mangled_address am(native_syscall_handler, policy);
        native_syscall_handler = code_cache::find(cpu, am);

        insert_cti_after(
            ls, ls.last(), native_syscall_handler,
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_JMP);

        const unsigned size(ls.encoded_size());
        native_syscall_handler = global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(size);
        ls.encode(native_syscall_handler, size);

        LAST_GEN_SYSCALL_ENTRYPOINT = reinterpret_cast<uint64_t>(
            native_syscall_handler);

        return LAST_GEN_SYSCALL_ENTRYPOINT;
    }
}
