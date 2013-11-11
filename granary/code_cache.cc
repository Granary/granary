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


    struct ibl_profiled_stub {
        app_pc source;
        app_pc dest;
        bool operator==(const ibl_profiled_stub that) const throw() {
            return source == that.source && dest == that.dest;
        }
    };


    IF_PROFILE_IBL( static static_data<hash_set<ibl_profiled_stub>> PROFILED_IBL_EXIT_STUBS; )

    /// The globally shared code cache. This maps policy-mangled code
    /// code addresses to translated addresses.
    static static_data<locked_hash_table<app_pc, app_pc>> CODE_CACHE;


    STATIC_INITIALISE_ID(code_cache_hash_table, {
        CODE_CACHE.construct();
        IF_PROFILE_IBL( PROFILED_IBL_EXIT_STUBS.construct(); )
    });


#if CONFIG_PROFILE_IBL
    static bool is_new_ibl_target(app_pc source, app_pc dest) throw() {
        ibl_lock();
        const ibl_profiled_stub key = {source, dest};
        const bool was_in(PROFILED_IBL_EXIT_STUBS->contains(key));
        if(!was_in) {
            PROFILED_IBL_EXIT_STUBS->add(key);
        }
        ibl_unlock();
        return !was_in;
    }
#endif


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
        _IF_PROFILE_IBL(app_pc source_addr)
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

#if CONFIG_PROFILE_IBL
            // If we're doing IBL profiling then we want to re-generate the
            // exit stub for every (source,dest) pair.
            if((policy.is_indirect_cti_target() || policy.is_return_target())) {
                if(!is_new_ibl_target(addr.as_address, target_addr)) {
                    return target_addr;
                } else {
                    // Need to null it out because `target_addr` is the address
                    // of an IBL exit stub, and if we left it set, then we'd
                    // end up with an IBL stub to an IBL stub to a basic block,
                    // which would screw things up a lot.
                    target_addr = nullptr;
                }
            } else {
                return target_addr;
            }
#else
            return target_addr;
#endif
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

#if CONFIG_ENABLE_TRACE_ALLOCATOR && CONFIG_TRACE_FUNCTIONAL_UNITS
        // If this basic block begins a new functional unit then locate its
        // basic blocks in a new allocator.
        if(policy.is_beginning_of_functional_unit()) {
            IF_PERF( perf::visit_functional_unit(); )
            cpu->current_fragment_allocator = \
                allocate_memory<generic_fragment_allocator>();
        }
#endif

        cpu->current_fragment_allocator->lock_coarse(IF_TEST(cpu->id));

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
#if !CONFIG_FOLLOW_CONDITIONAL_BRANCHES
                // !!!!!TODO!!!!!!

                client::discard_basic_block(*created_bb_state);

                // Try to clean up the shared memory.
                if(try_remove_basic_block_info(target_addr)) {
                    cpu->current_fragment_allocator->free_last();
                }

                // Try to clean up the private memory.
                cpu->stub_allocator.free_last();
                cpu->block_allocator.free_last();
#endif

                IF_TEST( target_addr = nullptr; );
                CODE_CACHE->load(base_addr.as_address, target_addr);
                ASSERT(target_addr);

            // Commit to the basic block state.
            } else if(stored_base_addr && created_bb) {
                client::commit_to_basic_block(*created_bb_state);
            }
        }

        cpu->current_fragment_allocator->unlock_coarse();

        // Propagate the base target to the CPU-private code cache.
        cpu->code_cache.store(base_addr.as_address, target_addr);

        // If this code cache lookup is the result of an indirect CALL/JMP, or
        // from a RET, then we need to generate an IBL/RBL exit stub.
        if(policy.is_indirect_cti_target() || policy.is_return_target()) {
            ibl_lock();
            target_addr = ibl_exit_routine(
                addr.as_address,
                target_addr
                _IF_PROFILE_IBL(source_addr));

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
        (app_pc (*)(cpu_state_handle, mangled_address _IF_PROFILE_IBL( app_pc )))
            code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(mangled_address _IF_PROFILE_IBL( app_pc ))) code_cache::find)


    GRANARY_DETACH_POINT_ERROR(
        (app_pc (*)(
            mangled_address,
            prediction_table **
        )) code_cache::find_on_cpu)
}

