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

#include "granary/types.h"

extern "C" {
    void notify_module_state_change(struct kernel_module *module) {
        using namespace granary;

        if(module->is_granary) {
            return;
        }

        types::module *mod(unsafe_cast<types::module *>(module->address));

        granary::printf("        Notified about module (%s) state change!!\n",
            mod->name);

        (void) mod;
    }

    static void on_cpu(void *) throw() {
        granary::types::printk("I am on a CPU\n");
    }

    void granary_initialise(void) {
        granary::printf("Initialising Granary...\n");
        granary::init();

#if CONFIG_RUN_TEST_CASES
        granary::printf("Running test cases...\n");
        granary::run_tests();
#endif


        granary::printf("Running dynamic wrapper and detach test...\n");
        auto func_ptr = granary::dynamic_wrapper_of(on_cpu);
        granary::attach(granary::START_POLICY);
        granary::types::on_each_cpu(func_ptr, nullptr, 1);
        granary::detach();
        granary::printf("Wooooo!\n");
    }
}
