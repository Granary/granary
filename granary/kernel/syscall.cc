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
    static app_pc LAST_SYSCALL_ENTRYPOINT = nullptr;
    static app_pc LAST_GEN_SYSCALL_ENTRYPOINT = nullptr;


    /// Generate a system call entry point for the current CPU.
    app_pc create_syscall_entrypoint(void) throw() {
        app_pc native_syscall_handler = unsafe_cast<app_pc>(get_msr(MSR_LSTAR));
        cpu_state_handle cpu;

        if(LAST_SYSCALL_ENTRYPOINT == native_syscall_handler) {
            return LAST_GEN_SYSCALL_ENTRYPOINT;
        }

        instruction_list ls;
        cpu->native_syscall_handler = native_syscall_handler;
        LAST_SYSCALL_ENTRYPOINT = native_syscall_handler;

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

        mangled_address am(native_syscall_handler, policy);
        native_syscall_handler = code_cache::find(cpu, am);

        insert_cti_after(
            ls, ls.last(), native_syscall_handler,
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_JMP);

        native_syscall_handler = global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(ls.encoded_size());

        ls.encode(native_syscall_handler);

        LAST_GEN_SYSCALL_ENTRYPOINT = native_syscall_handler;
        return native_syscall_handler;
    }
}
