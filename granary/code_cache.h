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

        /// Indirect branch lookup routine.
#if CONFIG_TRACK_XMM_REGS
        static app_pc IBL_COMMON_ENTRY_ROUTINE;
#endif
        static app_pc XMM_SAFE_IBL_COMMON_ENTRY_ROUTINE;


        /// Find fast. This looks in the cpu-private cache first, and failing
        /// that, defaults to the global code cache.
        static app_pc find_on_cpu(mangled_address addr) throw()
            __attribute__((hot, flatten));

        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        GRANARY_ENTRYPOINT
        __attribute__((hot))
        inline static app_pc find(mangled_address addr) throw() {
            cpu_state_handle cpu;
            thread_state_handle thread;
            enter(cpu, thread);
            return find(cpu, thread, addr);
        }

        /// Perform both lookup and insertion of a raw address for a given
        /// policy.
        template <typename Policy>
        __attribute__((hot))
        inline static app_pc find(app_pc addr, Policy) throw() {
            cpu_state_handle cpu;
            thread_state_handle thread;
            instrumentation_policy policy((policy_for<Policy>()));
            mangled_address mangled_addr(addr, policy);
            return find(cpu, thread, mangled_addr);
        }

        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        __attribute__((hot))
        static app_pc find(
            cpu_state_handle &cpu,
            thread_state_handle &thread,
            mangled_address addr
        ) throw();

        /// Initialise the indirect branch lookup routine.
        static void init_ibl(app_pc &, bool) throw();
        static app_pc ibl_exit_for(app_pc) throw();

        static void add(app_pc, app_pc) throw();

        /*
        /// Add an entry to the code cache for later prediction.
        static void predict(cpu_state_handle &cpu,
                            thread_state_handle &thread,
                            mangled_address app_code,
                            app_pc cache_code) throw();
        */
    };

}

#endif /* Granary_CODE_CACHE_H_ */
