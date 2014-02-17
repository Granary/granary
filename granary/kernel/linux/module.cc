/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * module.cc
 *
 *  Created on: 2012-11-08
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/globals.h"
#include "granary/policy.h"
#include "granary/code_cache.h"
#include "granary/basic_block.h"
#include "granary/attach.h"
#include "granary/detach.h"
#include "granary/perf.h"
#include "granary/test.h"
#include "granary/wrapper.h"
#include "granary/types.h"

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"

#include "granary/kernel/linux/module.h"

#include "granary/kernel/printf.h"

#include "clients/report.h"

#ifdef GRANARY_DONT_INCLUDE_CSTDLIB
#   undef GRANARY_DONT_INCLUDE_CSTDLIB
#endif

#define printf types::printk

using namespace granary;

extern "C" {

    extern int set_memory_rw ( unsigned long addr , int numpages ) ;

    extern void granary_before_module_bootstrap(struct kernel_module *module);
    extern void granary_before_module_init(struct kernel_module *module);


#if CONFIG_DELAYED_TAKEOVER
    extern bool granary_takeover_next_syscall_entry(void);
#endif

#if 0
    /// Make a special init function that sets certain page permissions before
    /// executing the module's init function.
    GRANARY_ENTRYPOINT
    void granary_replace_init_func(kernel_module *module) throw() {

        using namespace granary;

        cpu_state_handle cpu;
        granary::enter(cpu);

        instrumentation_policy policy(START_POLICY);
        policy.begins_functional_unit(true);
        policy.in_host_context(false);
        policy.return_address_in_code_cache(true);

        app_pc init_pc(unsafe_cast<app_pc>(*(module->init)));
        app_pc init_cc(code_cache::find(init_pc, policy));

        // Build a dynamic wrapper-like construct that makes sure that certain
        // data is readable/writable in the module before init() executes.
        instruction_list ls;
        ls.append(mov_imm_(reg::arg1, int64_(reinterpret_cast<uint64_t>(module))));
        ls.append(call_(pc_(unsafe_cast<app_pc>(granary_before_module_init))));
        ls.append(jmp_(pc_(init_cc)));

        // Encode.
        const unsigned size(ls.encoded_size());
        app_pc wrapped_init_pc = global_state::WRAPPER_ALLOCATOR-> \
            allocate_array<uint8_t>(size);
        ls.encode(wrapped_init_pc, size);

#if !CONFIG_DEBUG_INITIALISE
        *(module->init) = unsafe_cast<int (*)(void)>(wrapped_init_pc);
#endif
    }
#endif


    extern void granary_enter_private_stack(void);
    extern void granary_exit_private_stack(void);


    /// Notify granary of a state change.
    void notify_module_state_change(struct kernel_module *module) {
        using namespace granary;

        if(module->is_granary) {
            return;
        }

        switch(module->state) {
        case kernel_module::STATE_COMING: {
            printf("[granary] Notified about module (%s) state change: COMING.\n",
                module->name);

            granary_before_module_bootstrap(module);
#if 0
            if(module->init) {
                eflags flags = granary_disable_interrupts();

                ASM(
                    "movq %0, %%" TO_STRING(ARG1) ";"
                    TO_STRING(PUSHA_ASM_ARG)
                    "callq " TO_STRING(SHARED_SYMBOL(granary_enter_private_stack)) ";"
                    "callq " TO_STRING(SHARED_SYMBOL(granary_replace_init_func)) ";"
                    "callq " TO_STRING(SHARED_SYMBOL(granary_exit_private_stack)) ";"
                    TO_STRING(POPA_ASM_ARG)
                    :
                    : "m"(module)
                    : "%" TO_STRING(ARG1)
                );

                granary_store_flags(flags);
            }
#endif

            if(module->exit) {
                *(module->exit) = dynamic_wrapper_of(*(module->exit));
            }
            break;
        }

        case kernel_module::STATE_LIVE:
            printf("[granary] Notified about module (%s) state change: LIVE.\n",
                module->name);
            break;

        case kernel_module::STATE_GOING:
            printf("[granary] Notified about module (%s) state change: GOING.\n",
                module->name);
            break;
        }
    }


    /// Initialise Granary. This is the bridge between the C module code and
    /// the C++ Granary code.
    void granary_initialise(void) {

        printf("[granary] Initialising Granary...\n");
        init();
        printf("[granary] Initialised.\n");

#if CONFIG_DELAYED_TAKEOVER
        printf("[granary] Beginning delayed takeover...\n");
        while(granary_takeover_next_syscall_entry()) {
            types::msleep(CONFIG_DELAYED_TAKEOVER);
        }
        printf("[granary] Done delayed takeover.");
#endif
    }


    /// Report on Granary's activities.
    void granary_report(void) {
        IF_PERF( perf::report(); )

#ifdef CLIENT_report
        client::report();
#endif
    }
}

extern "C" {
    extern void module_load_notifier(void);
}

GRANARY_DETACH_POINT(notify_module_state_change);
GRANARY_DETACH_POINT(module_load_notifier);
GRANARY_DETACH_POINT(granary_report);

