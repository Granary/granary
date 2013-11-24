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
#include "granary/basic_block_info.h"
#include "granary/utils.h"
#include "granary/register.h"
#include "granary/emit_utils.h"
#include "granary/detach.h"
#include "granary/ibl.h"


#if CONFIG_DEBUG_ASSERTIONS
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
#endif /* CONFIG_DEBUG_ASSERTIONS */


namespace granary {


    struct ibl_profiled_stub {
        app_pc source;
        app_pc dest;
        bool operator==(const ibl_profiled_stub that) const throw() {
            return source == that.source && dest == that.dest;
        }
    };


    /// The globally shared code cache. This maps policy-mangled code
    /// code addresses to translated addresses.
    static static_data<
        locked_hash_table<app_pc, app_pc>
    > CODE_CACHE;


    STATIC_INITIALISE_ID(code_cache_hash_table, {
        CODE_CACHE.construct();
    });


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
        const mangled_address addr,
        app_pc indirect_cache_source_addr
    ) throw() {
        IF_TEST( cpu->last_find_address = addr.unmangled_address(); )
        IF_PERF( perf::visit_address_lookup(); )

        // Find the actual targeted address, independent of the policy.
        instrumentation_policy policy(addr);
        app_pc app_target_addr(addr.unmangled_address());
        app_pc target_addr(nullptr);

        // Try to load the target address from the global code cache.
        if(CODE_CACHE->load(addr.as_address, target_addr)) {
            if(policy.is_indirect_cti_target() || policy.is_return_target()) {
                IF_PERF( perf::visit_ibl_miss(app_target_addr); )
            }
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
        if(is_code_cache_address(app_target_addr)
        || is_wrapper_address(app_target_addr)
        || is_gencode_address(app_target_addr)) {
            target_addr = app_target_addr;
        }

        // Ensure that we're in the correct policy context. This might cause
        // a policy conversion. We also need to keep track of the auto-
        // instrument protocol for IBLs, i.e. that if auto-instrumenting is on
        // then we'll mark the CTI target as being in the host context. If the
        // mark was wrong (i.e. the IBL targets app code) then that's fine, and
        // if it targets host code then auto instrument.
#if !CONFIG_FEATURE_INSTRUMENT_HOST
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
        if(!target_addr && base_addr.as_address != addr.as_address) {
            CODE_CACHE->load(base_addr.as_address, target_addr);
        }

        // Can we detach to a known target?
        if(!target_addr && policy.can_detach()) {
            target_addr = find_detach_target(app_target_addr, policy.context());

#if !CONFIG_FEATURE_INSTRUMENT_HOST
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

#if CONFIG_ENABLE_TRACE_ALLOCATOR && !CONFIG_TRACE_ALLOCATE_FUNCTIONAL_UNITS
        // Allocator inheritance through indirect control flow instructions.
        // The direct branch lookup patcher (`granary/dbl.cc`) manages
        // allocator inheritance for direct control flow instructions.
        if(indirect_cache_source_addr) {
            ASSERT(is_code_cache_address(indirect_cache_source_addr));
            ASSERT(policy.is_indirect_cti_target() || policy.is_return_target());
            const basic_block_info * const source_bb_info(
                find_basic_block_info(indirect_cache_source_addr));
            cpu->current_fragment_allocator = source_bb_info->allocator;
        }
#endif
        UNUSED(indirect_cache_source_addr);

        // If this basic block begins a new functional unit then locate its
        // basic blocks in a new allocator.
        if(policy.is_beginning_of_functional_unit()) {
            IF_PERF( perf::visit_functional_unit(); )
#if CONFIG_ENABLE_TRACE_ALLOCATOR && CONFIG_TRACE_ALLOCATE_FUNCTIONAL_UNITS
            cpu->current_fragment_allocator = \
                allocate_memory<generic_fragment_allocator>();
#endif
        }

        cpu->current_fragment_allocator->lock_coarse(IF_TEST(cpu->id));

        // If we don't have a target yet then translate the target assuming it's
        // app or host code.
        unsigned num_translated_bbs(0);
        if(!target_addr) {

            target_addr = basic_block::translate(
                base_policy, cpu, app_target_addr, num_translated_bbs);

#if CONFIG_DEBUG_ASSERTIONS
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

            const basic_block_info *info(find_basic_block_info(target_addr));

            // If we've only translated a single basic block, then we'll prefer
            // to keep what was already in the code cache, because it might be
            // part of a trace, and because then we can clean up the basic
            // block's memory.
            if(1 == num_translated_bbs) {
                bool stored_base_addr(CODE_CACHE->store(
                    base_addr.as_address, target_addr, HASH_KEEP_PREV_ENTRY));

                if(!stored_base_addr) {
                    client::discard_basic_block(*info->state);
                    remove_basic_block_info(target_addr);

                    cpu->current_fragment_allocator->free_last();
                    cpu->stub_allocator.free_last();
                    cpu->block_allocator.free_last();

                    IF_TEST( target_addr = nullptr; );
                    CODE_CACHE->load(base_addr.as_address, target_addr);
                    ASSERT(target_addr);

                } else {
                    client::commit_to_basic_block(*info->state);
                }

            // If we've built a trace, then we'll assume it's better than what's
            // already in the code cache (e.g. another trace, or only an
            // individual basic block), and also we can't reliably clean up
            // memory for multiple basic blocks.
            //
            // TODO: Make the allocators and code cache transactional, so
            //       that multiple things can be committed / cleaned up.
            } else {
                CODE_CACHE->store(
                    base_addr.as_address, target_addr,
                    HASH_OVERWRITE_PREV_ENTRY
                );

                client::commit_to_basic_block(*info->state);
            }
        }

        cpu->current_fragment_allocator->unlock_coarse();

        // If this code cache lookup is the result of an indirect CALL/JMP, or
        // from a RET, then we need to generate an IBL/RBL exit stub.
        if(policy.is_indirect_cti_target() || policy.is_return_target()) {
            ibl_lock();
            target_addr = ibl_exit_routine(
                addr.as_address,
                target_addr);

            if(!CODE_CACHE->store(addr.as_address, target_addr, HASH_KEEP_PREV_ENTRY)) {
                CODE_CACHE->load(addr.as_address, target_addr);
            }
            ibl_unlock();
        }

        return target_addr;
    }


    /// Add some illegal detach points.
    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(app_pc, instrumentation_policy)) code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(cpu_state_handle, mangled_address, app_pc))
            code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(mangled_address, app_pc)) code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(
            mangled_address,
            prediction_table **
        )) code_cache::find_on_cpu)
}

