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
            NUM_PSEUDO_POLICIES = 1,
            NUM_POLICIES = 1 + NUM_PSEUDO_POLICIES,

            INDIRECT_CTI_POLICY_INCREMENT = 1
        };

        friend struct basic_block;
        friend struct code_cache;
        friend struct instruction_list_mangler;
        friend struct instruction;
        friend union mangled_address;
        friend union indirect_operand;
        template <typename> friend struct policy_for;


        typedef instrumentation_policy (instrumentation_policy::* basic_block_visitor)(
            cpu_state_handle &cpu,
            thread_state_handle &thread,
            basic_block_state &bb,
            instruction_list &ls
        );


        /// Policy basic block visitor functions for each policy. The code
        /// cache will use this array of function pointers (initialised
        /// partially at compile time and partially at run time) to determine
        /// which client-code basic block visitor functions should be called.
        static basic_block_visitor POLICY_FUNCTIONS[];


        /// Policy ID tracker.
        static std::atomic<unsigned> NEXT_POLICY_ID;


        /// The base id of this policy.
        uint8_t id;

        /// An increment to the base id, that generates either the base policy
        /// or a pseudo policy.
        uint8_t pseudo_id_increment;

        /// policy properties.
        uint8_t properties;

        enum {
#if CONFIG_TRACK_XMM_REGS
            IS_XMM_CONTEXT = 0,
#endif
            NUM_PROPERTIES_,

            // ensures that there is at least one property (otherwise the
            // bitfield of mangled_address that depends on `NUM_PROPERTIES`
            // won't work.
            NUM_PROPERTIES = !NUM_PROPERTIES_ ? 1 : NUM_PROPERTIES_
        };

        instrumentation_policy missing_policy(
            cpu_state_handle &,
            thread_state_handle &,
            basic_block_state &,
            instruction_list &
        ) throw();

        /// Initialise a policy given a policy id. This policy will be
        /// initialised with no properties.
        inline static instrumentation_policy
        from_id(uint8_t id) throw() {
            instrumentation_policy policy;
            policy.id = id;
            policy.pseudo_id_increment = id % NUM_POLICIES;
            policy.properties = 0;
            return policy;
        }

        /// Initialise a policy given a the mangled policy bits, which include
        /// the policy's id and properties.
        inline static instrumentation_policy
        from_extension_bits(uint8_t bits) throw() {
            const uint8_t id(bits >> NUM_PROPERTIES);
            const uint8_t mask(~0);
            instrumentation_policy policy;
            policy.id = id;
            policy.pseudo_id_increment = id % NUM_POLICIES;
            policy.properties = bits & (mask >> (8 - NUM_PROPERTIES));
            return policy;
        }

        /// Initialise a policy given a policy id. This policy will be
        /// initialised to have the given properties.
        inline static instrumentation_policy
        from_id_and_properties(uint8_t id, uint8_t properties) throw() {
            instrumentation_policy policy;
            policy.id = id;
            policy.pseudo_id_increment = id % NUM_POLICIES;
            policy.properties = properties;
            return policy;
        }

        /// Invoke client code instrumentation.
        inline instrumentation_policy instrument(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            basic_block_visitor visitor(POLICY_FUNCTIONS[id]);
            if(!visitor) {
                return missing_policy(cpu, thread, bb, ls);
            }
            return (this->*(visitor))(cpu, thread, bb, ls);
        }

    public:

        /// Do not allow default initialisations of policies: require that they
        /// have IDs through some well-defined means.
        inline instrumentation_policy(void) throw()
            : id(instrumentation_policy::MISSING_POLICY_ID)
            , pseudo_id_increment(0)
            , properties(0)
        { }

        instrumentation_policy(const instrumentation_policy &) throw() = default;

        instrumentation_policy(const mangled_address &) throw();

        instrumentation_policy &
        operator=(const instrumentation_policy &) throw() = default;

        /// Weak form of equivalence defined over policies.
        bool operator==(const instrumentation_policy that) const throw() {
            return id == that.id;
        }

        bool operator!=(const instrumentation_policy that) const throw() {
            return id != that.id;
        }

        inline bool operator!(void) const throw() {
            return !id;
        }

    private:


        /// Convert this policy (or pseudo policy) to the equivalent indirect
        /// CTI policy. The indirect CTI pseudo policy is used for IBL lookups.
        inline instrumentation_policy indirect_cti_policy(void) throw() {
            const uint8_t base_policy_id(id - pseudo_id_increment);
            return instrumentation_policy::from_id_and_properties(
                base_policy_id + INDIRECT_CTI_POLICY_INCREMENT,
                properties);
        }

        /// Is this an indirect CTI pseudo policy? The indirect CTI pseudo
        /// policy is used to help us "leave" the indirect branch lookup
        /// mechanism and restore any saved state.
        inline bool is_indirect_cti_policy(void) const throw() {
            return INDIRECT_CTI_POLICY_INCREMENT == pseudo_id_increment;
        }


        /// Return the "base" policy for this policy. This converts any pseudo
        /// policies back into non-pseudo policies.
        inline instrumentation_policy base_policy(void) throw() {
            return instrumentation_policy::from_id_and_properties(
                id - pseudo_id_increment,
                properties);
        }


        /// Get the mangled bits that would represent this policy when an
        /// address is extended to include a policy (and its properties).
        inline uint8_t extension_bits(void) const throw() {
            return (id << NUM_PROPERTIES) | properties;
        }


    public:

        /// Update the properties of this policy to be inside of an xmm context.
        inline void in_xmm_context(void) throw() {
#if CONFIG_TRACK_XMM_REGS
            properties |= (1 << IS_XMM_CONTEXT);
#endif
        }

        /// Returns whether or not the policy in the current context is xmm
        /// safe.
        inline bool is_in_xmm_context(void) const throw() {
#if CONFIG_TRACK_XMM_REGS
            return properties & (1 << IS_XMM_CONTEXT);
#else
            return true;
#endif
        }

        /// Inherit the properties of another policy.
        inline void inherit_properties(instrumentation_policy that) throw() {
            properties |= that.properties;
        }


        /// Clear all properties.
        inline void clear_properties(void) throw() {
            properties = 0;
        }


        /// Return the instrumentation policy with the next possible set of
        /// properties. This is useful (in conjunction with `clear_properties`
        /// for iterating over all properties of a specific policy.
        inline instrumentation_policy next(void) const throw() {
            uint8_t next_properties(properties + 1);
            instrumentation_policy policy;

            enum {
                MAX_PROPERTIES = 1 << (NUM_PROPERTIES)
            };

            if(MAX_PROPERTIES <= next_properties) {
                return policy;
            }

            policy.id = id;
            policy.pseudo_id_increment = pseudo_id_increment;
            policy.properties = next_properties;
            return policy;
        }
    };


    /// Defines a data type used to de/mangle an address that may or may not
    /// contain policy-specific bits.
    union mangled_address {
    public:

        enum {
            NUM_MANGLED_BITS = 8U
        };

    private:

        friend struct instrumentation_policy;

        enum {
            PROPERTY_NUM_BITS = instrumentation_policy::NUM_PROPERTIES,
            POLICY_NUM_BITS = NUM_MANGLED_BITS - PROPERTY_NUM_BITS
        };

        /// The mangled address in terms of the policy, policy properties, and
        /// address components.
        /// Note: order of these fields is significant.
        struct {
            uint8_t policy_properties:PROPERTY_NUM_BITS; // low
            uint8_t policy_id:POLICY_NUM_BITS;
            uint64_t _:(64 - (POLICY_NUM_BITS + PROPERTY_NUM_BITS)); // high
        } as_policy_address __attribute__((packed));

    public:

        /// The mangled address as an actual address.
        app_pc as_address;

        /// The mangled address as an unsigned int, which is convenient for
        /// bit masking.
        int64_t as_int;
        uint64_t as_uint;

        inline mangled_address(void) throw()
            : as_uint(0ULL)
        { }

        mangled_address(app_pc addr_, instrumentation_policy policy_) throw();

        /// Extract the original, unmangled address from this mangled address.
        app_pc unmangled_address(void) const throw();

    } __attribute__((packed));


    static_assert(8 == sizeof(mangled_address),
        "`granary::mangled_address` is too big.");


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
                unsafe_cast<instrumentation_policy::basic_block_visitor>( \
                    &T::visit_basic_block);

            return policy_id;
        }

    public:

        operator instrumentation_policy(void) const throw() {

            // TODO: race condition?
            if(!POLICY_ID) {
                POLICY_ID = init_policy();
            }

            return instrumentation_policy::from_id(POLICY_ID);
        }
    };


    /// This does the actual policy initialisation. The combination of
    /// instantiations of the `policy_for` template (in client code) and the
    /// static function/field ensure that policies are initialised early on.
    template <typename T>
    unsigned policy_for<T>::POLICY_ID = 0;
}

#endif /* GRANARY_POLICY_H_ */
