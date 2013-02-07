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
    union mangled_address;
    union indirect_operand;
    template <typename> struct policy_for;


    /// Represents a handle on an instrumentation policy.
    struct instrumentation_policy {
    private:

        enum {
            MISSING_POLICY_ID = 0,
            NUM_PSEUDO_POLICIES = 2,
            NUM_POLICIES = 1 + NUM_PSEUDO_POLICIES,

            INDIRECT_CTI_POLICY_INCREMENT = 1,
            DETACH_ON_RETURN_POLICY_INCREMENT = 2
        };

        friend struct basic_block;
        friend struct code_cache;
        friend struct instruction_list_mangler;
        friend struct instruction;
        friend union mangled_address;
        friend union indirect_operand;
        template <typename> friend struct policy_for;


        typedef instrumentation_policy (basic_block_visitor)(
            cpu_state_handle &cpu,
            thread_state_handle &thread,
            basic_block_state &bb,
            instruction_list &ls
        );


        /// Policy basic block visitor functions for each policy. The code
        /// cache will use this array of function pointers (initialised
        /// partially at compile time and partially at run time) to determine
        /// which client-code basic block visitor functions should be called.
        static basic_block_visitor *POLICY_FUNCTIONS[];


        /// Policy ID tracker.
        static std::atomic<unsigned> NEXT_POLICY_ID;


        /// The identifier for this policy.
        uint8_t policy_id;
        uint8_t policy_increment;


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
        inline instrumentation_policy(uint16_t policy_id_) throw()
            : policy_id(policy_id_)
            , policy_increment(policy_id_ % NUM_POLICIES)
        { }


        /// Invoke client code instrumentation.
        inline instrumentation_policy instrument(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            basic_block_visitor *visitor(POLICY_FUNCTIONS[policy_id]);
            if(!visitor) {
                return missing_policy(cpu, thread, bb, ls);
            }
            return visitor(cpu, thread, bb, ls);
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

        inline bool operator!(void) const throw() {
            return !policy_id;
        }

    private:

        /// Is this an indirect CTI pseudo policy? The indirect CTI pseudo
        /// policy is used to help us "leave" the indirect branch lookup
        /// mechanism and restore any saved state.
        inline bool is_indirect_cti_policy(void) const throw() {
            return INDIRECT_CTI_POLICY_INCREMENT == policy_increment;
        }

        /// Is this a detach-on-return pseudo policy? The detach-on-return
        /// pseudo policy is used when invoking basic_block::call, with the
        /// intention that
        inline bool is_detach_on_return_policy(void) const throw() {
            return DETACH_ON_RETURN_POLICY_INCREMENT == policy_increment;
        }


        /// Convert this policy (or pseudo policy) to the equivalent indirect
        /// CTI policy. The indirect CTI pseudo policy is used for IBL lookups.
        inline instrumentation_policy indirect_cti_policy(void) throw() {
            const uint8_t base_policy_id(policy_id - policy_increment);
            return instrumentation_policy(
                base_policy_id + INDIRECT_CTI_POLICY_INCREMENT);
        }

    public:

        /// Return the "base" policy for this policy. This converts any pseudo
        /// policies back into non-pseudo policies.
        inline instrumentation_policy base_policy(void) throw() {
            return instrumentation_policy(
                policy_id - (policy_id % NUM_POLICIES));
        }
    };


    /// Gets us the policy for some client policy type.
    template <typename T>
    struct policy_for {
    private:

        friend struct code_cache;

        static unsigned POLICY_ID;

        /// Initialise the policy `T`.
        static unsigned init_policy(void) throw() {

            // get IDs for one policy and its pseudo-policies
            unsigned policy_id(
                instrumentation_policy::NEXT_POLICY_ID.fetch_add(
                    instrumentation_policy::NUM_POLICIES));

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
