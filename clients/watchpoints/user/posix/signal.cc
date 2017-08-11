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

using namespace granary;

namespace client { namespace wp {


    static struct sigaction GRANARY_SIGSEGV;
    static struct sigaction NATIVE_SIGSEGV;


    /// Handle a segfault by trying to attach instrumentation to native code.
    static void handle_fault(int sig, siginfo_t *info, void *context_) {
        detach();

        ucontext *context = unsafe_cast<ucontext *>(context_);
        app_pc faulted_addr(unsafe_cast<app_pc>(
            context->uc_mcontext.gregs[REG_RIP]));

        if(is_code_cache_address(faulted_addr)) {
            if(NATIVE_SIGSEGV.sa_sigaction) {
                return NATIVE_SIGSEGV.sa_sigaction(sig, info, context_);
            }
            return;
        }

        // handle the fault.
        instrumentation_policy policy(START_POLICY);
        policy.in_host_context(true);
        policy.force_attach(true);

        cpu_state_handle cpu;
        mangled_address target(faulted_addr, policy);

        context->uc_mcontext.gregs[REG_RIP] = reinterpret_cast<uintptr_t>(
            code_cache::find(cpu, target));
    }


    /// Attach the signal handler when the program starts up.
    STATIC_INITIALISE({
        memset(&GRANARY_SIGSEGV, 0, sizeof GRANARY_SIGSEGV);
        memset(&NATIVE_SIGSEGV, 0, sizeof NATIVE_SIGSEGV);

        GRANARY_SIGSEGV.sa_flags = SA_SIGINFO;
        sigemptyset(&GRANARY_SIGSEGV.sa_mask);
        GRANARY_SIGSEGV.sa_sigaction = &handle_fault;

        ::sigaction(SIGSEGV, &GRANARY_SIGSEGV, &NATIVE_SIGSEGV);
    })
}}

