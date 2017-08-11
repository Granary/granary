/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * dynamic_wrapper.cc
 *
 *  Created on: 2013-03-20
 *      Author: pag
 */

#include "granary/globals.h"
#include "granary/code_cache.h"
#include "granary/hash_table.h"
#include "granary/policy.h"
#include "granary/instruction.h"
#include "granary/emit_utils.h"


namespace granary {

    static static_data<locked_hash_table<app_pc, app_pc>> wrappers;


    STATIC_INITIALISE_ID(dynamic_wrappers, {
        wrappers.construct();
    })


    /// Return the dynamic wrapper address for something to be wrapped.
    ///
    /// This function is partially implemented in x86/dynamic_wrapper.asm,
    /// which is responsible for switching stacks, disabling interrupts, and
    /// invoking this code.
    GRANARY_ENTRYPOINT
    extern "C" app_pc granary_dynamic_wrapper_of_impl(
        app_pc wrapper,
        app_pc wrappee
    ) {

        // Enter Granary.
        cpu_state_handle cpu;
        enter(cpu);

        instrumentation_policy policy(START_POLICY);
        policy.in_host_context(is_host_address(wrappee));
        policy.begins_functional_unit(true);

        // Enable us to both wrap *and* instrument some code.
        if(policy.is_in_host_context() && policy.is_host_auto_instrumented()) {
            policy.force_attach(true);
        }

        // Will directly return to:
        //      1) Wrapper code if the wrapper CALLs the code cache.
        //      2) Code cache if the wrapper tailcalls (JMPs) to the code cache.
        policy.return_address_in_code_cache(true);

        app_pc target_code_cache(nullptr);

        // In order to avoid checks for whether this function is wrapped or
        // not, we will just pretend that `wrappee` is a code cache target.
        if(policy.is_in_host_context() && !policy.is_host_auto_instrumented()) {
            if(wrappers->load(wrappee, target_code_cache)) {
                return target_code_cache;
            }
            target_code_cache = wrappee;
        } else {
            if(wrappers->load(wrappee, target_code_cache)) {
                return target_code_cache;
            }

            mangled_address am(wrappee, policy);
            target_code_cache = code_cache::find(cpu, am);
        }

        // Build the list to jump to that entry. This will put the address of
        // the code cache version of `wrappee` into `%r10` and then jump to
        // the generic wrapper (`wrapper`), which pulls its target out of
        // `%r10`.
        instruction_list ls;
        instruction in = ls.append(mov_imm_(
            reg::r10, int64_(reinterpret_cast<uint64_t>(target_code_cache))));
        insert_cti_after(
            ls, in, wrapper,
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_JMP);

        // Encode the wrapper.
        const unsigned size(ls.encoded_size());
        app_pc target_wrapper(global_state::WRAPPER_ALLOCATOR-> \
            allocate_array<uint8_t>(size));
        ls.encode(target_wrapper, size);

        // Store it for later and return.
        wrappers->store(wrappee, target_wrapper);

        // Add the wrapper to the code cache so that if we're instrumenting
        // host code, then we temporarily go native, and then go back to
        // instrumented code when the host code invokes a module dynamic
        // wrapper.
        //
        // Note: There is an asymmetry here where kernel wrappers are not
        //       invoked when host code is instrumented.
        mangled_address mangled_wrappee(wrappee, policy);
        code_cache::add(mangled_wrappee.as_address, target_wrapper);

        return target_wrapper;
    }
}
