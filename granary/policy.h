/*
 * policy.h
 *
 *  Created on: 2012-11-30
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_POLICY_H_
#define Granary_POLICY_H_

#include "granary/globals.h"
#include "clients/instrument.h"


/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define CASE_DIRECT_JUMP_MANGLER(opcode, size) \
    case dynamorio::OP_ ## opcode: return direct_branch_ ## opcode;


/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define DECLARE_DIRECT_JUMP_MANGLER(opcode, size) \
    static app_pc direct_branch_ ## opcode;


/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define DEFINE_DIRECT_JUMP_MANGLER(opcode, size) \
    template <typename Policy> \
    app_pc instrumenter<Policy>::direct_branch_ ## opcode = nullptr;


namespace granary {


    /// Represents null instrumentation, i.e. nothing extra added.
    struct null_policy { };


    /// Forward declarations.
    struct cpu_state_handle;
    struct thread_state_handle;
    struct basic_block_state;
    struct instruction_list;
    template <typename> struct code_cache;

    /// Defines an abstract instrumentation policy.
    struct instrumentation_policy {
    public:


        virtual ~instrumentation_policy(void) throw() { }


        /// Invoke client code to instrument an instruction list.
        virtual void
        instrument(cpu_state_handle &cpu,
                    thread_state_handle &thread,
                    basic_block_state *bb,
                    instruction_list &ls) throw() = 0;


        /// Find the policy-specific direct CTI patch function.
        virtual app_pc
        get_direct_cti_patch_func(int opcode) throw() = 0;
    };


    /// Defines a concrete instrumentation policy in terms of client
    /// instrumentation.
    ///
    /// Note: this structure is tightly coupled to the code_cache, in that the
    ///       code cache itself is reponsible for initializing all of the
    ///       static fields of the policy.
    template <typename Policy>
    struct instrumenter : public instrumentation_policy {
    private:

        template <typename> friend struct code_cache;

        DECLARE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(DECLARE_DIRECT_JUMP_MANGLER)

    public:

        virtual ~instrumenter(void) throw() { }

        /// Invoke client instrumentation on an instruction list that represents
        /// a basic block.
        virtual void
        instrument(cpu_state_handle &cpu,
                    thread_state_handle &thread,
                    basic_block_state *bb,
                    instruction_list &ls) throw()  {

            client::instrument<Policy>::basic_block(cpu, thread, bb, ls);
        }


        /// Look up and return the assembly patch (see asm/direct_branch.asm)
        /// function needed to patch an instruction that originally had opcode as
        /// `opcode`.
        virtual app_pc
        get_direct_cti_patch_func(int opcode) throw() {
            switch(opcode) {
            CASE_DIRECT_JUMP_MANGLER(call, 5)
            FOR_EACH_DIRECT_JUMP(CASE_DIRECT_JUMP_MANGLER);
            default: return nullptr;
            }
        }
    };

    FOR_EACH_DIRECT_JUMP(DEFINE_DIRECT_JUMP_MANGLER);
    DEFINE_DIRECT_JUMP_MANGLER(call, 5)
}

#undef CASE_DIRECT_JUMP_MANGLER
#undef DECLARE_DIRECT_JUMP_MANGLER
#undef DEFINE_DIRECT_JUMP_MANGLER
#endif /* Granary_POLICY_H_ */
