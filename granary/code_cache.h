/*
 * code_cache.h
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_CODE_CACHE_H_
#define Granary_CODE_CACHE_H_

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/mangle.h"

namespace granary {


    /// Represents a policy-specific code cache. A single client can be wholly
    /// represented by a policy, apply many policies (in the form of an
    /// aggregate policy), or dynamically switch between instrumentation
    /// policies.
    ///
    /// Note: This structure is tightly coupled with `instrumenter` of policy.h.
    struct code_cache {
    public:

        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        inline static app_pc find(address_mangler addr) throw() {
            cpu_state_handle cpu;
            thread_state_handle thread;
            return find(cpu, thread, addr);
        }

        /// Perform both lookup and insertion of a raw address for a given
        /// policy.
        template <typename Policy>
        inline static app_pc find(app_pc addr, Policy) throw() {
            cpu_state_handle cpu;
            thread_state_handle thread;
            address_mangler mangled_addr;
            instrumentation_policy policy((policy_for<Policy>()));

            mangled_addr.as_address = addr;
            mangled_addr.as_policy_address.policy_id = policy.policy_id;

            return find(cpu, thread, mangled_addr);
        }

        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        static app_pc find(cpu_state_handle &cpu,
                           thread_state_handle &thread,
                           address_mangler addr) throw();

        /*
        /// Add an entry to the code cache for later prediction.
        static void predict(cpu_state_handle &cpu,
                            thread_state_handle &thread,
                            address_mangler app_code,
                            app_pc cache_code) throw();
        */
    };

}

#endif /* Granary_CODE_CACHE_H_ */
