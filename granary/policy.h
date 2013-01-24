/*
 * policy.h
 *
 *  Created on: 2013-01-23
 *      Author: pag
 */

#ifndef GRANARY_POLICY_H_
#define GRANARY_POLICY_H_

#include "granary/globals.h"

namespace granary {


    /// Forward declarations.
    struct cpu_state_handle;
    struct thread_state_handle;
    struct basic_block;
    struct basic_block_state;
    struct instruction_list;
    struct instruction_list_mangler;
    struct code_cache;
    template <typename> struct policy_for;


    /// Represents a handle on an instrumentation policy.
    struct instrumentation_policy {
    private:

        friend struct basic_block;
        friend struct code_cache;
        friend struct instruction_list_mangler;
        template <typename> friend struct policy_for;

        typedef instrumentation_policy (basic_block_visitor)(
            cpu_state_handle &cpu,
            thread_state_handle &thread,
            basic_block_state &bb,
            instruction_list &ls
        );

        friend struct instruction_list_mangler;
        friend struct instruction;

        /// Policy basic block visitor functions for each policy. The code
        /// cache will use this array of function pointers (initialised
        /// partially at compile time and partially at run time) to determine
        /// which client-code basic block visitor functions should be called.
        static basic_block_visitor *POLICY_FUNCTIONS[256];


        /// Policy ID tracker.
        static std::atomic<unsigned> NEXT_POLICY_ID;


        /// The identifier for this policy.
        const uint8_t policy_id;


        static instrumentation_policy missing_policy(
            cpu_state_handle &,
            thread_state_handle &,
            basic_block_state &,
            instruction_list &
        ) throw();


        /// Do not allow default initialisations of policies: require that they
        /// have IDs through some well-defined means.
        instrumentation_policy(void) throw() = delete;


        /// Takes in a policy ID directly.
        instrumentation_policy(uint16_t policy_id_) throw()
            : policy_id(policy_id_)
        { }


        /// Invoke client code instrumentation.
        inline instrumentation_policy instrument(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            return POLICY_FUNCTIONS[policy_id](cpu, thread, bb, ls);
        }

    public:

        instrumentation_policy(const instrumentation_policy &) throw() = default;

        instrumentation_policy &
        operator=(const instrumentation_policy &) throw() = default;

        bool operator==(const instrumentation_policy &that) const throw() {
            return policy_id == that.policy_id;
        }

        bool operator!=(const instrumentation_policy &that) const throw() {
            return policy_id != that.policy_id;
        }
    };


    /// Gets us the policy for some client policy type.
    template <typename T>
    struct policy_for {
    private:

        static unsigned POLICY_ID;

        /// Initialise the policy `T`.
        static unsigned init_policy(void) throw() {
            unsigned policy_id(
                instrumentation_policy::NEXT_POLICY_ID.fetch_add(1U));

            instrumentation_policy::POLICY_FUNCTIONS[policy_id] = \
                &(T::visit_basic_block);

            return policy_id;
        }

    public:

        operator instrumentation_policy(void) const throw() {
            return instrumentation_policy(POLICY_ID);
        }
    };


    /// This does the actual policy initialisation. The combination of
    /// instantiations of the `policy_for` template (in client code) and the
    /// static function/field ensure that policies are initialised early on.
    template <typename T>
    unsigned policy_for<T>::POLICY_ID = init_policy();
}

#endif /* GRANARY_POLICY_H_ */
