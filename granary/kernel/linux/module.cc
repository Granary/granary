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

#include "granary/kernel/linux/module.h"

#include "granary/kernel/printf.h"

#include "clients/report.h"

#ifdef GRANARY_DONT_INCLUDE_CSTDLIB
#   undef GRANARY_DONT_INCLUDE_CSTDLIB
#endif

extern "C" {


    extern void granary_before_module_init(struct kernel_module *module);


    /// Make a special init function that sets certain page permissions before
    /// executing the module's init function.
    static int (*make_init_func(
        int (*init)(void),
        kernel_module *module
    ))(void) throw() {

        using namespace granary;

        app_pc init_pc(unsafe_cast<app_pc>(init));
        app_pc init_cc(code_cache::find(init_pc, START_POLICY));

        // build a dynamic wrapper-like construct that makes sure that certain
        // data is readable/writable in the module before init() executes.
        instruction_list ls;
        ls.append(mov_imm_(reg::arg1, int64_(reinterpret_cast<uint64_t>(module))));
        ls.append(call_(pc_(unsafe_cast<app_pc>(granary_before_module_init))));
        ls.append(jmp_(pc_(init_cc)));

        // encode it
        app_pc wrapped_init_pc = global_state::WRAPPER_ALLOCATOR-> \
            allocate_array<uint8_t>(ls.encoded_size());
        ls.encode(wrapped_init_pc);

        return unsafe_cast<int (*)(void)>(wrapped_init_pc);
    }


    /// Notify granary of a state change.
    void notify_module_state_change(struct kernel_module *module) {
        using namespace granary;

        if(module->is_granary) {
            return;
        }

        printf("[granary] Notified about module (%s) state change.\n",
            module->name);

        switch(module->state) {
        case kernel_module::STATE_COMING: {
            *(module->init) = make_init_func(*(module->init), module);
            if(module->exit) {
                *(module->exit) = dynamic_wrapper_of(*(module->exit));
            }
            break;
        }

        case kernel_module::STATE_LIVE:
            break;

        case kernel_module::STATE_GOING:
            break;
        }
    }


    /// Initialise Granary. This is the bridge between the C module code and
    /// the C++ Granary code.
    void granary_initialise(void) {
        using namespace granary;

        printf("[granary] Initialising Granary...\n");
        init();
        printf("[granary] Initialised.\n");

#if CONFIG_RUN_TEST_CASES
        printf("[granary] Running test cases...\n");
        cpu_state_handle cpu;
        cpu.free_transient_allocators();

        run_tests();
        printf("[granary] All test cases ran.\n");
#endif
    }


    /// Report on Granary's activities.
    void granary_report(void) {
        IF_PERF( granary::perf::report(); )

#ifdef CLIENT_report
        client::report();
#endif
    }
}
