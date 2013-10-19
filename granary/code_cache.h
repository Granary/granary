/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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


        /// Find fast. This looks in the cpu-private cache first, and failing
        /// that, defaults to the global code cache.
        GRANARY_ENTRYPOINT
        __attribute__((hot, optimize("Os")))
        static app_pc find_on_cpu(
            mangled_address addr,
            prediction_table **IF_IBL_PREDICT(predict_table)
        ) throw();


        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        GRANARY_ENTRYPOINT
        __attribute__((hot))
        inline static app_pc find(
            mangled_address addr
        ) throw() {
            IF_KERNEL( kernel_preempt_disable(); )

            cpu_state_handle cpu;

            enter(cpu);

            app_pc ret(find(cpu, addr));

            IF_KERNEL( kernel_preempt_enable(); )

            return ret;
        }


        /// Perform both lookup and insertion of a raw address for a given
        /// policy.
        GRANARY_ENTRYPOINT
        __attribute__((hot))
        inline static app_pc find(
            app_pc addr,
            instrumentation_policy policy
        ) throw() {

            IF_KERNEL( eflags flags(granary_disable_interrupts()); )
            IF_KERNEL( kernel_preempt_disable(); )

            cpu_state_handle cpu;
            enter(cpu);

            mangled_address mangled_addr(addr, policy);
            app_pc ret(find(cpu, mangled_addr));

            IF_KERNEL( kernel_preempt_enable(); )
            IF_KERNEL( granary_store_flags(flags); )
            return ret;
        }

        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        __attribute__((hot))
        static app_pc find(
            cpu_state_handle cpu,
            mangled_address addr
        ) throw();


        /// Look-up an entry in the code cache. This will not do translation.
        ///
        /// Note: This must be invoked with interrupts disabled (if called
        ///       from kernel space).
        ///
        /// Note: This is a sister function of `code_cache::add`, where together
        ///       one can directly add and find entries in the global code
        ///       cache.
        static app_pc lookup(app_pc) throw();


        /// Force add an entry into the code cache.
        static void add(app_pc, app_pc) throw();
    };

}

#endif /* Granary_CODE_CACHE_H_ */
