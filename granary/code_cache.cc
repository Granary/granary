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


#if CONFIG_ENABLE_ASSERTIONS
extern "C" {


    /// Runtime conditional value for triggering a breakpoint on the following
    /// function.
    bool granary_do_break_on_translate = false;


    /// Auto-added GDB breakpoint.
    DONT_OPTIMISE
    void granary_break_on_translate(void *addr) {
        //granary::printf("%p\n", addr);
        USED(addr);
    }
}
#endif /* CONFIG_ENABLE_ASSERTIONS */


namespace granary {


    extern "C" {
        /// Hash an address and return the associated IBL code cache entry.
        extern ibl_code_cache_table_entry *granary_ibl_hash(
            ibl_code_cache_table_entry *base, app_pc address
        );
    }


    /// The globally shared code cache. This maps policy-mangled code
    /// code addresses to translated addresses.
    static static_data<locked_hash_table<app_pc, app_pc>> CODE_CACHE;


    enum {
        NUM_ACTUAL_IBL_CODE_CACHE_ENTRIES = NUM_IBL_CODE_CACHE_ENTRIES
                                          + CONFIG_NUM_IBL_HASH_TABLE_CHECKS
    };

    /// The globally shared, fixed-sized, atomic code cache hash table that
    /// is used for indirect branch lookups.
    ibl_code_cache_table_entry IBL_CODE_CACHE[NUM_ACTUAL_IBL_CODE_CACHE_ENTRIES];


    STATIC_INITIALISE_ID(code_cache, {
        CODE_CACHE.construct();

        memset(
            &(IBL_CODE_CACHE[0]),
            0,
            sizeof(ibl_code_cache_table_entry) * NUM_ACTUAL_IBL_CODE_CACHE_ENTRIES);
    })


    /// Update the IBL hash table to add an entry.
    static bool update_ibl_hash_table(
        app_pc addr,
        app_pc mangled_addr,
        app_pc target_addr
    ) throw() {
        ibl_code_cache_table_entry *entry(
            granary_ibl_hash(&(IBL_CODE_CACHE[0]), addr));

        for(unsigned i(0); i < CONFIG_NUM_IBL_HASH_TABLE_CHECKS; ++i, ++entry) {

            // Entry is already there.
            if(entry->mangled_address->load(std::memory_order_relaxed) == addr) {
                return true;
            }

            if(!entry->instrumented_address->load(std::memory_order_relaxed)) {
                app_pc expected(nullptr);
                const bool changed(entry->instrumented_address-> \
                    compare_exchange_strong(expected, target_addr));

                // We own this entry now.
                if(changed) {
                    IF_PERF( perf::visit_ibl_add_entry(); )
                    entry->mangled_address->store(mangled_addr);
                    return true;

                // Changed out from under us, but to the desired value.
                } else if(expected == target_addr) {
                    return true;
                }
            }
        }

        IF_PERF( perf::visit_ibl_cant_add_entry(addr); )

        return false;
    }


    /// Find fast. This looks in the cpu-private cache first, and failing
    /// that, defaults to the global code cache.
    app_pc code_cache::find_on_cpu(mangled_address addr) throw() {
        cpu_state_handle cpu;
        app_pc ret(cpu->code_cache.find(addr.as_address));
        IF_PERF( perf::visit_address_lookup_cpu(nullptr != ret); )
        return ret;
    }


    /// Add a custom mapping to the code cache.
    ///
    /// Note: If in the kernel, must be called with interrupts disabled!
    void code_cache::add(app_pc source, app_pc dest) throw() {
        CODE_CACHE->store(source, dest);
    }


    /// Look-up an entry in the code cache. This will not do translation.
    app_pc code_cache::lookup(app_pc addr) throw() {
        app_pc target_addr(nullptr);
        if(CODE_CACHE->load(addr, target_addr)) {
            IF_PERF( perf::visit_address_lookup_hit(); )
        }
        return target_addr;
    }


    /// Perform both lookup and insertion (basic block translation) into
    /// the code cache.
    app_pc code_cache::find(
        cpu_state_handle cpu,
        const mangled_address addr
    ) throw() {
        IF_PERF( perf::visit_address_lookup(); )

        // Find the actual targeted address, independent of the policy.
        instrumentation_policy policy(addr);
        app_pc app_target_addr(addr.unmangled_address());
        app_pc target_addr(nullptr);

        // Try to load the target address from the global code cache.
        if(CODE_CACHE->load(addr.as_address, target_addr)) {
            cpu->code_cache.store(addr.as_address, target_addr);
            IF_PERF( perf::visit_address_lookup_hit(); )
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
#if !CONFIG_ENABLE_DIRECT_RETURN
        const uintptr_t addr_uint(reinterpret_cast<uintptr_t>(app_target_addr));
        uint32_t *header_addr(reinterpret_cast<uint32_t *>(
            addr_uint + 16 - RETURN_ADDRESS_OFFSET));
        if(RETURN_ADDRESS_OFFSET == (addr_uint % 8)
        && basic_block_info::HEADER == *header_addr) {
#else
        if(is_code_cache_address(app_target_addr)
        || is_wrapper_address(app_target_addr)
        || is_gencode_address(app_target_addr)) {
#endif /* GRANARY_IN_KERNEL */
            target_addr = app_target_addr;
        }

        // Ensure that we're in the correct policy context. This might cause
        // a policy conversion. We also need to keep track of the auto-
        // instrument protocol for IBLs, i.e. that if auto-instrumenting is on
        // then we'll mark the CTI target as being in the host context. If the
        // mark was wrong (i.e. the IBL targets app code) then that's fine, and
        // if it targets host code then auto instrument.
#if !CONFIG_INSTRUMENT_HOST
        const bool policy_was_in_host_context(policy.is_in_host_context());
#endif
        policy.in_host_context(
            IF_USER_ELSE(false, is_host_address(app_target_addr)));

        // Figure out the non-policy-mangled target address, and find the base
        // policy (absent any temporary properties).
        instrumentation_policy base_policy(policy.base_policy());
        mangled_address base_addr(app_target_addr, base_policy);

        // Policy has gone through a property conversion (e.g. host->app,
        // app->host, indirect->direct, return->direct). Check to see if we
        // actually have the converted version in the code cache.
        bool base_addr_exists(false);
        if(!target_addr && base_addr.as_address != addr.as_address) {
            if(CODE_CACHE->load(base_addr.as_address, target_addr)) {
                base_addr_exists = true;
            }
        }

        // Can we detach to a known target?
        if(!target_addr && policy.can_detach()) {
            target_addr = find_detach_target(app_target_addr, policy.context());

#if !CONFIG_INSTRUMENT_HOST
            // Application of the auto-instrument host protocol. If we weren't
            // auto-instrumenting or already in host code, and now we've
            // switched to host code, then set the target address so that we
            // can detach.
            if(!target_addr
            && !policy_was_in_host_context
            && policy.is_in_host_context()) {
                target_addr = app_target_addr;
            }
#endif
        }

        // If we don't have a target yet then translate the target assuming it's
        // app or host code.
        bool created_bb(false);
        basic_block_state *created_bb_state(nullptr);
        if(!target_addr) {

            basic_block bb(basic_block::translate(
                base_policy, cpu, app_target_addr));
            target_addr = bb.cache_pc_start;
            created_bb = true;
            created_bb_state = bb.state();

#if CONFIG_ENABLE_ASSERTIONS
            // The trick here is that if we've got a particular buggy
            // instrumentation policy and we're trying to debug it then we can
            // "narrow down" on a bug by excluding certain cases (i.e. divide
            // and conquer). We can tell GDB that we've narrowed down on a bug
            // without constantly having to add conditional breakpoints by
            // setting `granary_do_break_on_translate` to `true` and then
            // letting the automatically added breakpoint in GDB do the rest.
            if(granary_do_break_on_translate) {
                granary_break_on_translate(target_addr);
                granary_do_break_on_translate = false;
            }
#endif
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
                client::discard_basic_block(*created_bb_state);

                cpu->fragment_allocator.free_last();
                cpu->block_allocator.free_last();

                CODE_CACHE->load(base_addr.as_address, target_addr);

            // Commit to the basic block state.
            } else if(stored_base_addr && created_bb) {
                client::commit_to_basic_block(*created_bb_state);
            }
        }

        // Propagate the base target to the CPU-private code cache.
        cpu->code_cache.store(base_addr.as_address, target_addr);

        // If this code cache lookup is the result of an indirect CALL/JMP, or
        // from a RET, then we need to generate an IBL/RBL exit stub.
        if(policy.is_indirect_cti_target() || policy.is_return_target()) {
            target_addr = instruction_list_mangler::ibl_exit_routine(target_addr);
            if(!CODE_CACHE->store(addr.as_address, target_addr, HASH_KEEP_PREV_ENTRY)) {
                cpu->fragment_allocator.free_last();
                CODE_CACHE->load(addr.as_address, target_addr);
            }

            bool store_in_cpu_private(true);

            // Update the global IBL hash table.
            if(policy.is_indirect_cti_target()) {
                store_in_cpu_private = update_ibl_hash_table(
                    addr.unmangled_address(),
                    addr.as_address,
                    target_addr
                );
            }

            // Don't store redundant entries into the CPU-private hash table.
            if(store_in_cpu_private) {
                cpu->code_cache.store(addr.as_address, target_addr);
            }
        }

        return target_addr;
    }


    /// Add some illegal detach points.
    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(app_pc, instrumentation_policy)) code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(cpu_state_handle, mangled_address))
            code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(mangled_address)) code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(
            mangled_address,
            prediction_table **
        )) code_cache::find_on_cpu)
}

