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

#if !GRANARY_IN_KERNEL
#   ifndef _GNU_SOURCE
#      define _GNU_SOURCE
#   endif
#   include <dlfcn.h>
#else
#   include "granary/kernel/linux/module.h"
#endif

namespace granary {


    /// Define two context-specific hash tables for finding detach points. These
    /// hash tables are only updated during Granary's initialisation.
    static static_data<cpu_private_code_cache> DETACH_HASH_TABLE[2];


#if !GRANARY_IN_KERNEL
    static void wrap_user_address(
        function_wrapper &wrapper,
        runtime_context context,
        app_pc wrapper_address,
        uintptr_t dlsym_address
    ) throw() {
        if(wrapper.original_address) {
            DETACH_HASH_TABLE[context]->store(
                reinterpret_cast<app_pc>(wrapper.original_address),
                wrapper_address);
        }

        auto dlsym_ = dlsym;

        // This evil beast allows us to have something like `free` resolve
        // to _GI___libc_free.
        while(dlsym_address && wrapper.original_address != dlsym_address) {
            DETACH_HASH_TABLE[context]->store(
                reinterpret_cast<app_pc>(dlsym_address),
                wrapper_address);

            auto prev_dlsym_ = dlsym_;
            dlsym_ = unsafe_cast<decltype(dlsym_)>(
                prev_dlsym_(RTLD_NEXT, "dlsym"));

            if(!dlsym_ || dlsym_ == prev_dlsym_) {
                break;
            }

            dlsym_address = unsafe_cast<uintptr_t>(
                dlsym_(RTLD_DEFAULT, wrapper.name));
        }
    }
#endif


    STATIC_INITIALISE_ID(detach_hash_table, {

        DETACH_HASH_TABLE[RUNNING_AS_APP].construct();
        DETACH_HASH_TABLE[RUNNING_AS_HOST].construct();

        // Add all wrappers to the detach hash table.
        IF_WRAPPERS( for(unsigned i(0); i < LAST_DETACH_ID; ++i) {
            const function_wrapper &wrapper(FUNCTION_WRAPPERS[i]);

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
        } )

        // Add internal dynamic symbols to the detach hash table.
        IF_USER(IF_WRAPPERS( for(unsigned i(LAST_DETACH_ID + 1); ; ++i) {
            function_wrapper &wrapper(FUNCTION_WRAPPERS[i]);

            if(!wrapper.name) {
                break;
            }

            uintptr_t dlsym_address(reinterpret_cast<uintptr_t>(
                dlsym(RTLD_DEFAULT, wrapper.name)));

            // While not necessary, this improves debugging when inspecting
            // the wrapper in `gdb` with the `p-wrapper` command.
            if(!wrapper.original_address) {
                wrapper.original_address = dlsym_address;
            }

            wrap_user_address(
                wrapper,
                RUNNING_AS_APP,
                reinterpret_cast<app_pc>(wrapper.app_wrapper_address),
                dlsym_address);

            wrap_user_address(
                wrapper,
                RUNNING_AS_HOST,
                reinterpret_cast<app_pc>(wrapper.host_wrapper_address),
                dlsym_address);
        } ))
    })


    /// Add a detach target to the hash table.
    void add_detach_target(
        app_pc detach_addr,
        app_pc redirect_addr,
        runtime_context context
    ) throw() {
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
    app_pc find_detach_target(app_pc detach_addr, runtime_context context) throw() {

#if GRANARY_IN_KERNEL
#   if !CONFIG_INSTRUMENT_WHOLE_KERNEL
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
    void detach(void) throw() {
        ASM("");
    }


    GRANARY_DETACH_POINT(detach);
    GRANARY_DETACH_POINT_ERROR(enter);
    GRANARY_DETACH_POINT_ERROR(granary_fault);
    GRANARY_DETACH_POINT_ERROR(granary_break_on_fault);

#if GRANARY_IN_KERNEL
    extern "C" {
        extern void module_load_notifier(void);
        extern void granary_report(void);
    }

    GRANARY_DETACH_POINT(notify_module_state_change);
    GRANARY_DETACH_POINT(module_load_notifier);
    GRANARY_DETACH_POINT(granary_report);
#endif
}

