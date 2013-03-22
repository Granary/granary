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

#include "granary/kernel/module.h"
#include "granary/kernel/printf.h"

#ifdef GRANARY_DONT_INCLUDE_CSTDLIB
#   undef GRANARY_DONT_INCLUDE_CSTDLIB
#endif


extern "C" {
    void notify_module_state_change(struct kernel_module *module) {
        using namespace granary;

        if(module->is_granary) {
            return;
        }

        printf("[granary] Notified about module (%s) state change.\n",
            module->name);

        switch(module->state) {
        case kernel_module::STATE_COMING:
            *(module->init) = dynamic_wrapper_of(*(module->init));
            if(module->exit) {
                *(module->exit) = dynamic_wrapper_of(*(module->exit));
            }
            break;

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
        run_tests();
        printf("[granary] All test cases ran.\n");
#endif
    }


    /// Report on Granary's activities.
    void granary_report(void) {
        IF_PERF( granary::perf::report(); )
    }
}
