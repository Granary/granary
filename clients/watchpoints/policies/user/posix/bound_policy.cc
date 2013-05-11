/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_policy.cc
 *
 *  Created on: May 10, 2013
 *      Author: Peter Goodman
 */

#if 0

#include <signal.h>

#ifdef sigaddset
#   undef sigaddset
#endif

#ifdef sigdelset
#   undef sigdelset
#endif

#ifdef sigismember
#   undef sigismember
#endif

#ifdef sigemptyset
#   undef sigemptyset
#endif

#ifdef sigfillset
#   undef sigfillset
#endif

#include "clients/watchpoints/policies/bound_policy.h"

#include "granary/globals.h"
#include "granary/types.h"

using namespace granary;

namespace client { namespace wp {


    /// Found registers that are watched.
    struct watched_addresses {
        register_manager seen;
        uint64_t *regs[MAX_NUM_OPERANDS * 2];
        unsigned num_regs;
    };


#ifdef __APPLE__


    /// Try to doing if a specific machine register is a watched address.
    static void find_watched_register(
        watched_addresses &tracker,
        types::__darwin_x86_thread_state64 &regs,
        dynamorio::reg_id_t reg
    ) throw() {
        uint64_t *reg_ptr(nullptr);
        switch(register_manager::scale(reg, REG_64)) {
        case dynamorio::DR_REG_RAX:     reg_ptr = &(regs.__rax); break;
        case dynamorio::DR_REG_RCX:     reg_ptr = &(regs.__rcx); break;
        case dynamorio::DR_REG_RDX:     reg_ptr = &(regs.__rdx); break;
        case dynamorio::DR_REG_RBX:     reg_ptr = &(regs.__rbx); break;
        case dynamorio::DR_REG_RSP:     reg_ptr = &(regs.__rsp); break;
        case dynamorio::DR_REG_RBP:     reg_ptr = &(regs.__rbp); break;
        case dynamorio::DR_REG_RSI:     reg_ptr = &(regs.__rsi); break;
        case dynamorio::DR_REG_RDI:     reg_ptr = &(regs.__rdi); break;
        case dynamorio::DR_REG_R8:      reg_ptr = &(regs.__r8);  break;
        case dynamorio::DR_REG_R9:      reg_ptr = &(regs.__r9);  break;
        case dynamorio::DR_REG_R10:     reg_ptr = &(regs.__r10); break;
        case dynamorio::DR_REG_R11:     reg_ptr = &(regs.__r11); break;
        case dynamorio::DR_REG_R12:     reg_ptr = &(regs.__r12); break;
        case dynamorio::DR_REG_R13:     reg_ptr = &(regs.__r13); break;
        case dynamorio::DR_REG_R14:     reg_ptr = &(regs.__r14); break;
        case dynamorio::DR_REG_R15:     reg_ptr = &(regs.__r15); break;
        default: break;
        }

        if(is_watched_address(*reg_ptr) && tracker.seen.is_live(reg)) {
            tracker.regs[tracker.num_regs++] = reg_ptr;
        }
    }


    /// Try to detect if we are doing a memory operation that directly
    /// dereferences a watched address.
    static void find_watched_registers(
        const operand_ref &op,
        watched_addresses &tracker,
        types::__darwin_x86_thread_state64 &regs
    ) throw() {
        if(dynamorio::BASE_DISP_kind != op->kind) {
            return;
        }

        find_watched_register(tracker, regs, op->value.base_disp.base_reg);
        find_watched_register(tracker, regs, op->value.base_disp.index_reg);
    }


    /// Handle a segfault by looking to see if a watched address is the cause
    /// and try to fix-up only the specific address(es).
    ///
    /// Note: The behaviour of handling a signal in such a way is undefined due
    ///       to the fact that fixed-up registers might later participate in
    ///       comparisons against unfixed versions of themselves, etc.
    static void handle_signal(
        int,
        __siginfo *,
        void *state_
    ) throw() {
        detach();

        types::__darwin_ucontext *context(
            unsafe_cast<types::__darwin_ucontext *>(state_));
        types::__darwin_x86_thread_state64 &regs(context->uc_mcontext->__ss);

        app_pc pc(unsafe_cast<app_pc>(regs.__rip));
        instruction in(instruction::decode(&pc));

        watched_addresses tracker;
        in.for_each_operand(find_watched_registers, tracker, regs);

        // Only replace things if they appear to be an issue.
        if(tracker.num_regs) {
            ::printf("fixup!\n");
            for(unsigned i(0); i < tracker.num_regs; ++i) {
                *(tracker.regs[i]) = unwatched_address(*(tracker.regs[i]));
            }
        }
    }


    /// Attach the signal handler when the program starts up.
    STATIC_INITIALISE({
        static struct sigaction sa;

        sa.sa_flags = SA_SIGINFO;
        types::sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = &handle_signal;

        sigaction(SIGSEGV, &sa, nullptr);
    })


#endif /* __APPLE__ */

}}

#endif
