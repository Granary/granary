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


    /// Try to find the first write to %rsp, which we take as a signal that
    /// we're now on a kernel stack, where push/pop are safe.
    static void find_safe_stack(const operand_ref &op, bool &is_safe) {
        if(DEST_OPERAND == op.kind
        && dynamorio::REG_kind == op->kind
        && dynamorio::DR_REG_RSP == op->value.reg) {
            is_safe = true;
        }
    }


    /// Generate a system call entry point for the current CPU.
    app_pc create_syscall_entrypoint(void) throw() {
        app_pc native_syscall_handler = unsafe_cast<app_pc>(get_msr(MSR_LSTAR));
        cpu_state_handle cpu;

        if(LAST_SYSCALL_ENTRYPOINT == native_syscall_handler) {
            return LAST_GEN_SYSCALL_ENTRYPOINT;
        }

        instruction_list ls;
        bool stack_is_safe(false);

        cpu->native_syscall_handler = native_syscall_handler;
        LAST_SYSCALL_ENTRYPOINT = native_syscall_handler;

        for(;;) {
            instruction in(instruction::decode(&native_syscall_handler));
            in.for_each_operand(find_safe_stack, stack_is_safe);

            ls.append(in);

            if(stack_is_safe) {
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
