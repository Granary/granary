/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * policy.h
 *
 *  Created on: 2013-01-23
 *      Author: pag
 */

#ifndef GRANARY_POLICY_H_
#define GRANARY_POLICY_H_

#include "granary/globals.h"
#include "granary/state_handle.h"

namespace granary {


    /// Forward declarations.
    struct basic_block;
    struct basic_block_state;
    struct instruction_list;
    struct instruction_list_mangler;
    struct code_cache;
    union mangled_address;
    union indirect_operand;
    template <typename> struct policy_for;


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
    /// Forward declaration.
    interrupt_handled_state handle_interrupt(
        interrupt_stack_frame *isf,
        interrupt_vector vector
    ) throw();
#endif


    enum property_inherit_constraint {
        INHERIT_ALL,
        INHERIT_CALL,
        INHERIT_JMP,
        INHERIT_RETURN
    };


    /// Represents a handle on an instrumentation policy.
    struct instrumentation_policy {
    private:


        friend struct basic_block;
        friend struct code_cache;
        friend struct instruction_list_mangler;
        friend struct instruction;
        friend union mangled_address;
        friend union indirect_operand;
        template <typename> friend struct policy_for;


        /// Type of a client-specific instruction visitor.
        typedef typename detail::method_pointer<
            instrumentation_policy, // return
            instrumentation_policy, // base
            cpu_state_handle, // args...
            basic_block_state &,
            instruction_list &
        >::type basic_block_visitor;


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
        /// Global interrupt handler. Dispatches to client-specific interrupt
        /// handler functions.
        friend interrupt_handled_state handle_interrupt(
            interrupt_stack_frame *isf,
            interrupt_vector vector
        ) throw();


        /// Type of a client-specific interrupt handler.
        typedef typename detail::method_pointer<
            interrupt_handled_state, // return
            instrumentation_policy, // base
            cpu_state_handle, // args...
            thread_state_handle,
            basic_block_state &,
            interrupt_stack_frame &,
            interrupt_vector
        >::type interrupt_visitor;


        /// Client-specific interrupt handler functions.
        static interrupt_visitor INTERRUPT_VISITORS[];


        /// Dummy interrupt handler. This will be invoked if execution
        /// somehow enters into an invalid policy and is interrupted.
        static interrupt_handled_state missing_interrupt(
            cpu_state_handle,
            thread_state_handle,
            basic_block_state &,
            interrupt_stack_frame &,
            interrupt_vector
        ) throw();
#endif

#if !CONFIG_FEATURE_INSTRUMENT_HOST
        /// Should this policy auto-instrument host (kernel, libc) code?
        static bool AUTO_VISIT_HOST[];
#endif

        /// Policy basic block visitor functions for each policy. The code
        /// cache will use this array of function pointers (initialised
        /// partially at compile time and partially at run time) to determine
        /// which client-code basic block visitor functions should be called.
        static basic_block_visitor APP_VISITORS[];
        static basic_block_visitor HOST_VISITORS[];


        /// Policy ID tracker.
        static std::atomic<unsigned> NEXT_POLICY_ID;

        enum {
            NUM_TEMPORARY_PROPERTIES = 4,
            NUM_INHERITED_PROERTIES = 4,
            NUM_PROPERTIES = NUM_TEMPORARY_PROPERTIES + NUM_INHERITED_PROERTIES
        };


        union {
            /// Note: When adding properties to this list, be sure to keep
            ///       `NUM_TEMPORARY_PROPERTIES`, `NUM_INHERITED_PROERTIES`,
            ///       and `base_policy` synchronised.
            struct {

                /// Temporary property; Does this basic block begin a functional
                /// unit? The conditions for beginning a functional unit is that
                /// a basic block is either the target of a CALL or an indirect
                /// JMP.
                bool begins_functional_unit:1;

                /// Temporary property; this tells the code cache lookup
                /// function that it is coming from an indirect CTI, which
                /// causes the generation of an IBL exit stub for the
                /// targeted a basic block.
                bool is_indirect_target:1;

                /// Temporary property; this tells the code cache lookup
                /// function that it is coming from a return, which tells it
                /// that it should ignore policy behaviours (e.g. auto-
                /// instrument host code).
                bool is_return_target:1;

                /// Should we force this code to be instrumented, regardless of
                /// if it is the target of a detach point?
                bool force_attach:1;

                /// Inherited property; this tells us if we should save/restore
                /// all XMM registers.
                bool is_in_xmm_context:1;

                /// Inherited property; this tells us whether we are
                /// instrumenting host code or app code.
                bool is_in_host_context:1;

                /// Inherited property; this tells us whether or not we are in a
                /// basic block that potentially accesses user space data.
                bool accesses_user_data:1;

                /// Inherited property; this tells us if the last return address
                /// is back into an instrumentated basic block.
                bool can_direct_return:1;

                /// Policy identifier.
                uint16_t id:(16 - NUM_PROPERTIES);

            } __attribute__((packed)) u;

            /// The bit string representation of a policy.
            uint16_t as_raw_bits;

        } __attribute__((packed));


        enum {
            MAX_NUM_POLICY_IDS = 1ULL << (16 - NUM_PROPERTIES)
        };


        /// Initialise a policy given a policy id. This policy will be
        /// initialised with no properties.
        inline static instrumentation_policy from_id(uint16_t id) throw() {
            instrumentation_policy policy;
            policy.as_raw_bits = 0;
            policy.u.id = id;
            return policy;
        }

        /// Initialise a policy given a the mangled policy bits, which include
        /// the policy's id and properties.
        inline static instrumentation_policy decode(uint16_t bits) throw() {
            instrumentation_policy policy;
            policy.as_raw_bits = bits;
            return policy;
        }

    public:

        /// Invoke client code instrumentation.
        inline instrumentation_policy instrument(
            cpu_state_handle cpu,
            basic_block_state &bb,
            instruction_list &ls
        ) const throw() {
            basic_block_visitor visitor;

            if(is_in_host_context()) {
                visitor = HOST_VISITORS[u.id];
            } else {
                visitor = APP_VISITORS[u.id];
            }

            instrumentation_policy self(*this);

            ASSERT(as_raw_bits == self.as_raw_bits);

            return (self.*visitor)(cpu, bb, ls);
        }

#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
        /// Invoke client code interrupt handler.
        inline interrupt_handled_state handle_interrupt(
            cpu_state_handle cpu,
            thread_state_handle thread,
            basic_block_state &bb,
            interrupt_stack_frame &isf,
            interrupt_vector vector
        ) throw() {
            interrupt_visitor visitor(INTERRUPT_VISITORS[u.id]);
            if(!visitor) {
                return INTERRUPT_DEFER;
            }

            instrumentation_policy self(*this);
            return (self.*visitor)(cpu, thread, bb, isf, vector);
        }
#endif

        /// Do not allow default initialisations of policies: require that they
        /// have IDs through some well-defined means.
        inline instrumentation_policy(void) throw() {
            as_raw_bits = 0;
        }

        inline instrumentation_policy(const instrumentation_policy &that) throw() {
            as_raw_bits = that.as_raw_bits;
        }

        inline instrumentation_policy(const instrumentation_policy &&that) throw() {
            as_raw_bits = that.as_raw_bits;
        }

        instrumentation_policy(const mangled_address &) throw();

        inline instrumentation_policy &
        operator=(const instrumentation_policy &that) throw() {
            as_raw_bits = that.as_raw_bits;
            return *this;
        }

        inline instrumentation_policy &
        operator=(const instrumentation_policy &&that) throw() {
            as_raw_bits = that.as_raw_bits;
            return *this;
        }

        /// Weak form of equivalence defined over policies.
        inline bool operator==(const instrumentation_policy that) const throw() {
            return u.id == that.u.id;
        }

        inline bool operator!=(const instrumentation_policy that) const throw() {
            return u.id != that.u.id;
        }

        inline bool operator!(void) const throw() {
            return !u.id;
        }

        /// Does the code instrumented by this policy access user data?
        inline bool is_beginning_of_functional_unit(void) const throw() {
            return u.begins_functional_unit;
        }

        /// Mark the code instrumented by this policy as accessing user space
        /// data.
        inline void begins_functional_unit(bool val=true) throw() {
            u.begins_functional_unit = val;
        }

#if CONFIG_ENV_KERNEL
        /// Does the code instrumented by this policy access user data?
        inline bool accesses_user_data(void) const throw() {
            return u.accesses_user_data;
        }

        /// Mark the code instrumented by this policy as accessing user space
        /// data.
        inline void access_user_data(bool val=true) throw() {
            u.accesses_user_data = val;
        }
#endif

        /// Does the code instrumented by this policy access user data?
        inline bool return_address_is_in_code_cache(void) const throw() {
#if CONFIG_OPTIMISE_DIRECT_RETURN
            return true;
#else
            return u.can_direct_return;
#endif
        }

        /// Mark the code instrumented by this policy as accessing user space
        /// data.
        inline void return_address_in_code_cache(bool val=true) throw() {
#if CONFIG_OPTIMISE_DIRECT_RETURN
            (void) val;
#else
            u.can_direct_return = val;
#endif
        }

        /// Convert this policy (or pseudo policy) to the equivalent indirect
        /// CTI policy. The indirect CTI pseudo policy is used for IBL lookups.
        inline void indirect_cti_target(bool val=true) throw() {
            u.is_indirect_target = val;
        }

        /// Is this an indirect CTI pseudo policy? The indirect CTI pseudo
        /// policy is used to help us "leave" the indirect branch lookup
        /// mechanism and restore any saved state.
        inline bool is_indirect_cti_target(void) const throw() {
            return u.is_indirect_target;
        }


        /// Update the properties of this policy to be inside of an xmm context.
        inline void return_target(bool val=true) throw() {
            u.is_return_target = val;
        }


        /// Returns whether or not the policy in the current context is xmm
        /// safe.
        inline bool is_return_target(void) const throw() {
            return u.is_return_target;
        }


        /// Return the "base" policy for this policy. The effect of this is to
        /// remove temporary properties, but NOT inherited properties.
        inline instrumentation_policy base_policy(void) throw() {
            instrumentation_policy policy;
            policy.as_raw_bits = as_raw_bits;
            policy.u.begins_functional_unit = false;
            policy.u.is_indirect_target = false;
            policy.u.is_return_target = false;
            policy.u.force_attach = false;

            return policy;
        }


    public:


        /// Get the mangled bits that would represent this policy when an
        /// address is extended to include a policy (and its properties).
        inline uint16_t encode(void) const throw() {
            return as_raw_bits;
        }


        /// Get the detach context for this policy.
        inline runtime_context context(void) const throw() {
            if(is_in_host_context()) {
                return RUNNING_AS_HOST;
            } else {
                return RUNNING_AS_APP;
            }
        }


        /// Update the propertings of this policy to be inside of a host code
        /// context.
        inline void in_host_context(bool val=true) throw() {
            u.is_in_host_context = val;
        }


        /// Returns true iff we are instrumenting host code (kernel, libc).
        inline bool is_in_host_context(void) const throw() {
            return u.is_in_host_context;
        }


        /// Returns true iff we should automatically switch to the host context
        /// instead of detaching.
        inline bool is_host_auto_instrumented(void) const throw() {
#if CONFIG_FEATURE_INSTRUMENT_HOST
            return true;
#else
            return AUTO_VISIT_HOST[u.id];
#endif
        }


        /// Update the properties of this policy to be inside of an xmm context.
        inline void in_xmm_context(bool val=true) throw() {
            u.is_in_xmm_context = val;
        }


        /// Returns whether or not the policy in the current context is xmm
        /// safe.
        inline bool is_in_xmm_context(void) const throw() {
            return u.is_in_xmm_context;
        }


        /// Set this policy to forcibly attach (or not) to some code.
        inline void force_attach(bool val=true) throw() {
            u.force_attach = val;
        }


        /// Check whether or not some code, regardless of if it's a detach
        /// point, must be instrumented.
        inline bool can_detach(void) throw() {
            return !u.force_attach;
        }


        /// Inherit the properties of another policy.
        inline void inherit_properties(
            instrumentation_policy that,
            property_inherit_constraint constraint=INHERIT_ALL
        ) throw() {
            const uint16_t id(u.id);
            as_raw_bits = 0;
            if(INHERIT_ALL == constraint
            || INHERIT_JMP == constraint
            || INHERIT_RETURN == constraint) {
                as_raw_bits |= that.as_raw_bits;
            } else if(INHERIT_CALL == constraint) {
                as_raw_bits |= that.as_raw_bits;
                u.accesses_user_data = false;
            }
            u.id = id;
        }


        /// Clear all properties.
        inline void clear_properties(void) throw() {
            const uint16_t id(u.id);
            as_raw_bits = 0;
            u.id = id;
        }
    };


    extern instrumentation_policy START_POLICY;


    /// Defines a data type used to de/mangle an address that may or may not
    /// contain policy-specific bits.
    union mangled_address {
    public:

        enum {
            NUM_MANGLED_BITS = 16U
        };

    private:

        friend struct instrumentation_policy;

        /// The mangled address in terms of the policy, policy properties, and
        /// address components.
        ///
        /// Note: order of these fields is significant.
        struct {
            uint64_t _:(64 - NUM_MANGLED_BITS);     // low
            uint16_t policy_bits:NUM_MANGLED_BITS;  // high
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

        mangled_address(
            app_pc addr_,
            const instrumentation_policy policy_
        ) throw();

        /// Extract the original, unmangled address from this mangled address.
        app_pc unmangled_address(void) const throw();

    } __attribute__((packed));


    static_assert(8 == sizeof(mangled_address),
        "`mangled_address` is too big.");


    namespace detail {
        /// One method pointer kind to another.
        template <
            typename RetT,
            typename FromT,
            typename... Args
        >
        FORCE_INLINE
        typename detail::method_pointer<RetT, instrumentation_policy, Args...>::type
        unsafe_policy_method_cast(
            typename detail::method_pointer<RetT, FromT, Args...>::type v
        ) throw() {
            typename detail::method_pointer<RetT, instrumentation_policy, Args...>::type ret;
            void **v_ptr(unsafe_cast<void **>(&v));
            ret = *unsafe_cast<decltype(&ret)>(v_ptr);
            return ret;
        }
    }


    /// Gets us the policy for some client policy type.
    template <typename T>
    struct policy_for {
    private:

        static_assert(std::is_convertible<T *, instrumentation_policy *>::value,
            "Type specified in `policy_for` template must be a derived type "
            "of `instrumentation_policy`.");

        static_assert(sizeof(T) == sizeof(instrumentation_policy),
            "Derived type of instrumentation policy is larger than base type.");

        friend struct code_cache;

        static unsigned POLICY_ID;

        /// Initialise the policy `T`.
        static unsigned init_policy(void) throw() {

            unsigned policy_id(
                instrumentation_policy::NEXT_POLICY_ID.fetch_add(1));

#if !CONFIG_FEATURE_INSTRUMENT_HOST
            instrumentation_policy::AUTO_VISIT_HOST[policy_id] = \
                !!(T::AUTO_INSTRUMENT_HOST);
#endif

            instrumentation_policy::APP_VISITORS[policy_id] = \
                detail::unsafe_policy_method_cast<
                    instrumentation_policy, // ret
                    T, // policy class
                    cpu_state_handle, // args...
                    basic_block_state &,
                    instruction_list &
                >(&T::visit_app_instructions);

            instrumentation_policy::HOST_VISITORS[policy_id] = \
                detail::unsafe_policy_method_cast<
                    instrumentation_policy, // ret
                    T, // policy class
                    cpu_state_handle, // args...
                    basic_block_state &,
                    instruction_list &
                >(&T::visit_host_instructions);

#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
            instrumentation_policy::INTERRUPT_VISITORS[policy_id] = \
                detail::unsafe_policy_method_cast<
                    interrupt_handled_state, // ret
                    T, // policy class
                    cpu_state_handle, // args...
                    thread_state_handle,
                    basic_block_state &,
                    interrupt_stack_frame &,
                    interrupt_vector
                >(&T::handle_interrupt);
#endif

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
