/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#ifndef CFG_INSTRUMENT_H_
#define CFG_INSTRUMENT_H_

#include "granary/client.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::cfg_entry_policy())
#endif


namespace client {


    /// Initialise the basic block state and add in calls to event handlers.
    void instrument_basic_block(
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw();


    template <typename DerivedPolicy>
    struct cfg_base_policy : public granary::instrumentation_policy {
    public:


        enum {
            AUTO_INSTRUMENT_HOST = false
        };


        /// Instrument a basic block.
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle &,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            using namespace granary;
            instrument_basic_block(bb, ls);
            return granary::policy_for<DerivedPolicy>();
        }


        /// Instrument a basic block.
        static granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle &,
            granary::basic_block_state &,
            granary::instruction_list &
        ) throw() {
            return granary::policy_for<DerivedPolicy>();
        }
    };


    /// Used to instrument the first basic block of a function.
    struct cfg_entry_policy : public cfg_base_policy<cfg_entry_policy> {
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle &cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();
    };


    /// Used to instrument all non-entry basic blocks.
    struct cfg_exit_policy : public cfg_base_policy<cfg_exit_policy> {
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle &cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();
    };
}


#endif /* CFG_INSTRUMENT_H_ */
