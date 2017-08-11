/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * detach.cc
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */


#include "granary/detach.h"
#include "granary/utils.h"
#include "granary/state.h"
#include "granary/cpu_code_cache.h"

namespace granary {


    /// Define two context-specific hash tables for finding detach points. These
    /// hash tables are only updated during Granary's initialisation.
    static static_data<cpu_private_code_cache> DETACH_HASH_TABLE[2];


    STATIC_INITIALISE_ID(detach_hash_table, {

        DETACH_HASH_TABLE[RUNNING_AS_APP].construct();
        DETACH_HASH_TABLE[RUNNING_AS_HOST].construct();

        // Add all wrappers to the detach hash table.
        for(unsigned i(0); i < LAST_DETACH_ID; ++i) {
            const function_wrapper &wrapper(FUNCTION_WRAPPERS[i]);

            if(!wrapper.original_address) {
                continue;
            }

            if(wrapper.app_wrapper_address) {
                DETACH_HASH_TABLE[RUNNING_AS_APP]->store(
                    reinterpret_cast<app_pc>(wrapper.original_address),
                    reinterpret_cast<app_pc>(wrapper.app_wrapper_address));
            }

            if(wrapper.host_wrapper_address) {
                DETACH_HASH_TABLE[RUNNING_AS_HOST]->store(
                    reinterpret_cast<app_pc>(wrapper.original_address),
                    reinterpret_cast<app_pc>(wrapper.host_wrapper_address));
            }
        }
    })


    /// Add a detach target to the hash table.
    void add_detach_target(
        app_pc detach_addr,
        app_pc redirect_addr,
        runtime_context context
    ) {
        DETACH_HASH_TABLE[context]->store(detach_addr, redirect_addr);
    }


    /// Returns the address of a detach point. For example, in the
    /// kernel, if pc == &printk is a detach point then this will
    /// return the address of the printk wrapper (which might itself
    /// be printk).
    ///
    /// Returns:
    ///     A translated target address, or nullptr if this isn't a
    ///     detach target.
    app_pc find_detach_target(
        app_pc detach_addr,
        runtime_context context
    ) {

#if CONFIG_ENV_KERNEL
#   if !CONFIG_FEATURE_INSTRUMENT_HOST
        if(likely(RUNNING_AS_APP == context)) {
#   else
        if(unlikely(RUNNING_AS_APP == context)) {
#   endif
            if(!is_host_address(detach_addr)) {
                return nullptr;
            }
        }
#endif

        app_pc redirect_addr(nullptr);
        if(DETACH_HASH_TABLE[context]->load(detach_addr, redirect_addr)) {
            ASSERT(unsafe_cast<app_pc>(&granary_fault) != redirect_addr);
            return redirect_addr;
        }

        return nullptr;
    }


    /// Detach Granary.
    void detach(void) {
        ASM("");
    }


    GRANARY_DETACH_POINT(detach);
    GRANARY_DETACH_POINT_ERROR(enter);
    GRANARY_DETACH_POINT_ERROR(granary_fault);
    GRANARY_DETACH_POINT_ERROR(granary_break_on_fault);

#if !CONFIG_ENV_KERNEL && GRANARY_USE_PIC
    extern "C" void granary_end_program(void);
    GRANARY_DETACH_POINT(granary_end_program)
#endif

}

