/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * signal.cc
 *
 *  Created on: 2013-06-20
 *      Author: Peter Goodman
 */

#include <csignal>
#include <ucontext.h>

#include "clients/instrument.h"

#include "granary/globals.h"

extern "C" void exit(int);

using namespace granary;

namespace client { namespace wp {


    /// Handle a segfault by trying to attach instrumentation to native code.
    static void handle_fault(int, siginfo_t *info, void *context_) throw() {
        USED(info);
        detach();

        granary::printf("faulted on watched address!\n");

        // handle the fault.
        instrumentation_policy policy(START_POLICY);
        policy.in_host_context(true);
        policy.force_attach(true);

        cpu_state_handle cpu;
        ucontext *context = unsafe_cast<ucontext *>(context_);
        app_pc faulted_addr(unsafe_cast<app_pc>(
            context->uc_mcontext.gregs[REG_RIP]));
        mangled_address target(faulted_addr, policy);

        context->uc_mcontext.gregs[REG_RIP] = reinterpret_cast<uintptr_t>(
            code_cache::find(cpu, target));
    }


    /// Attach the signal handler when the program starts up.
    STATIC_INITIALISE({
        static struct sigaction sa;

        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = &handle_fault;

        sigaction(SIGSEGV, &sa, nullptr);
    })
}}

