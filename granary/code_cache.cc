/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * code_cache.cc
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/code_cache.h"
#include "granary/hash_table.h"
#include "granary/basic_block.h"
#include "granary/utils.h"
#include "granary/register.h"
#include "granary/emit_utils.h"
#include "granary/detach.h"
#include "granary/mangle.h"
#include "granary/predict.h"
#include "granary/trace_log.h"

#include "granary/kernel/linux/module.h"
extern "C" {
    extern kernel_module *modules;

    __attribute__((noinline, optimize("O0")))
    void granary_break_on_self(granary::app_pc addr) {
        ASM("" :: "m"(addr));
        (void) addr;
    }
}

/// Logging for the trace log. This is helpful for kernel-space debugging, where
/// the in-memory tracing data structure can be inspected to see the history of
/// code cache lookups.
#if CONFIG_TRACE_EXECUTION && 0
#   define LOG(...) trace_log::log_entry(__VA_ARGS__)
#else
#   define LOG(...)
#endif

namespace granary {

    namespace {

        /// The globally shared code cache. This maps policy-mangled code
        /// code addresses to translated addresses.
#if CONFIG_LOCK_GLOBAL_CODE_CACHE
        static static_data<locked_hash_table<app_pc, app_pc>> CODE_CACHE;
#else
        static static_data<rcu_hash_table<app_pc, app_pc>> CODE_CACHE;
#endif
    }


    STATIC_INITIALISE_ID(code_cache, {
        CODE_CACHE.construct();
    })


    /// Find fast. This looks in the cpu-private cache first, and failing
    /// that, defaults to the global code cache.
    app_pc code_cache::find_on_cpu(
        mangled_address addr,
        prediction_table **IF_IBL_PREDICT(predict_table)
    ) throw() {

        IF_KERNEL( kernel_preempt_disable(); )

        cpu_state_handle cpu;
        app_pc ret(cpu->code_cache.find(addr.as_address));

        IF_PERF( perf::visit_address_lookup_cpu(nullptr != ret); )

#if CONFIG_ENABLE_IBL_PREDICTION_STUBS
        if(ret && predict_table) {
            prediction_table::instrument(
                predict_table, cpu, addr.unmangled_address(), ret);
        }
#endif

        IF_KERNEL( kernel_preempt_enable(); )

        return ret;
    }


    /// Add a custom mapping to the code cache.
    void code_cache::add(app_pc source, app_pc dest) throw() {
        CODE_CACHE->store(source, dest);
    }


    /// Perform both lookup and insertion (basic block translation) into
    /// the code cache.
    app_pc code_cache::find(
        cpu_state_handle &cpu,
        thread_state_handle &thread,
        mangled_address addr
    ) throw() {
        IF_PERF( perf::visit_address_lookup(); )

        // find the actual targeted address, independent of the policy.
        instrumentation_policy policy(addr);
        app_pc app_target_addr(addr.unmangled_address());
        app_pc target_addr(nullptr);

        // Try to load the target address from the global code cache.
        if(CODE_CACHE->load(addr.as_address, target_addr)) {
            cpu->code_cache.store(addr.as_address, target_addr);
            IF_PERF( perf::visit_address_lookup_hit(); )
            LOG(app_target_addr, target_addr, TARGET_ALREADY_IN_CACHE);
            return target_addr;
        }

        // Do a return address IBL-like lookup to see if this might be a
        // return address into the code cache. This issue comes up if a
        // copied return address is jmp/called to.
        //
        // TODO: This isn't a perfect solution: if some code inspects a code
        //       cache return address and then displaces it then we will
        //       have a problem (moreso in user space; kernel space is easier
        //       to detect code cache addresses).
#if !GRANARY_IN_KERNEL
        const uintptr_t addr_uint(reinterpret_cast<uintptr_t>(app_target_addr));
        uint32_t *header_addr(reinterpret_cast<uint32_t *>(
            addr_uint + 16 - RETURN_ADDRESS_OFFSET));
        if(RETURN_ADDRESS_OFFSET == (addr_uint % 8)
        && basic_block_info::HEADER == *header_addr) {
#else
        if(is_code_cache_address(app_target_addr)
        || is_wrapper_address(app_target_addr)
        || is_gencode_address(app_target_addr)) {
        //|| (modules->text_begin <= app_target_addr && app_target_addr < modules->text_end)) {
#endif /* GRANARY_IN_KERNEL */
            target_addr = app_target_addr;
        }

        if(!target_addr) {
            target_addr = find_detach_target(app_target_addr, policy.context());
        }

#if GRANARY_IN_KERNEL
        // Not a detach target; figure out if we're in the wrong policy context.
        if(!target_addr) {

            // App code.
            if(!policy.is_in_host_context()) {
                if(is_host_address(app_target_addr)) {
                    policy.in_host_context(true);
                }

            // Host code, going to app code.
            } else if(is_app_address(app_target_addr)) {
                policy.in_host_context(false);
            }
        }
#endif /* GRANARY_IN_KERNEL */

        // Figure out the non-policy-mangled target address, and get our policy.
        instrumentation_policy base_policy(policy.base_policy());
        mangled_address base_addr(app_target_addr, base_policy);

        // Either a pseudo policy vs. a base policy, or the policy has gone
        // through a property conversion.
        bool base_addr_exists(false);
        if(base_addr.as_address != addr.as_address) {
            app_pc existing_target_addr(nullptr);
            if(CODE_CACHE->load(base_addr.as_address, existing_target_addr)) {
                base_addr_exists = true;
                target_addr = existing_target_addr;
            }
        }

        // If we don't have a target yet then translate the target assuming its
        // app or host code.
        bool created_bb(false);
        if(!target_addr) {
            basic_block bb(basic_block::translate(
                base_policy, cpu, thread, app_target_addr));
            target_addr = bb.cache_pc_start;
            created_bb = true;
        }

        // If the base policy address isn't in the code cache yet, put it there.
        // If there's a race condition, i.e. two threads/cores both put the same
        // base address in, then one will fail (according to
        // `HASH_KEEP_PREV_ENTRY`). If it fails and we built a basic block, then
        // free up some memory.
        if(!base_addr_exists) {
            bool stored_base_addr(CODE_CACHE->store(
                base_addr.as_address, target_addr, HASH_KEEP_PREV_ENTRY));

            if(!stored_base_addr && created_bb) {
                cpu->fragment_allocator.free_last();
                cpu->block_allocator.free_last();

                // TODO: minor memory leak with basic block state and vtables.
                //       consider switching to a "transactional" allocator.
                CODE_CACHE->load(base_addr.as_address, target_addr);
            }
        }

        // Propagate the base target to the CPU-private code cache.
        cpu->code_cache.store(base_addr.as_address, target_addr);

        // If the original policy was an indirect psudo-policy, then add an
        // exit routine to it.
        if(policy.is_indirect_cti_policy()) {
            target_addr = instruction_list_mangler::ibl_exit_routine(target_addr);
            if(!CODE_CACHE->store(addr.as_address, target_addr, HASH_KEEP_PREV_ENTRY)) {
                cpu->fragment_allocator.free_last();
                CODE_CACHE->load(addr.as_address, target_addr);
            }

            cpu->code_cache.store(addr.as_address, target_addr);
        }

        LOG(app_target_addr, target_addr, TARGET_TRANSLATED);
        return target_addr;
    }


    /// Add some illegal detach points.
    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(app_pc, instrumentation_policy)) code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(cpu_state_handle &, thread_state_handle &, mangled_address))
            code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(mangled_address)) code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(
            mangled_address,
            prediction_table **
        )) code_cache::find_on_cpu)
}

