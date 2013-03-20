/*
 * CODE_CACHE->cc
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/code_cache.h"
#include "granary/detach.h"
#include "granary/hash_table.h"
#include "granary/basic_block.h"
#include "granary/utils.h"
#include "granary/register.h"
#include "granary/emit_utils.h"
#include "granary/detach.h"
#include "granary/mangle.h"
#include "granary/predict.h"

#define D(...)
//__VA_ARGS__

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


    STATIC_INITIALISE({
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

        D( instrumentation_policy p(addr); )
        D( printf("find-cpu(%p, %x, xmm=%d)\n",
            addr.unmangled_address(),
            p.extension_bits(),
            p.is_in_xmm_context()); )
        D( printf(" -> %p\n", ret); )

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
        D( printf("add(%p, %p)\n", source, dest); )
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

        D( printf("find(%p, %x, xmm=%d)\n",
            app_target_addr,
            policy.extension_bits(),
            policy.is_in_xmm_context()); )

        // Try to load the target address from the global code cache.
        if(CODE_CACHE->load(addr.as_address, target_addr)) {
            cpu->code_cache.store(addr.as_address, target_addr);
            D( printf(" -> %p (hit)\n", target_addr); )
            IF_PERF( perf::visit_address_lookup_hit(); )
            return target_addr;
        }

#if !CONFIG_ENABLE_TRANSPARENT_RETURN_ADDRESSES
        // do a return address ibl-like lookup to see if this might be a
        // return address into the code cache. This issue comes up if a
        // copied return address is jmp/called to.
        //
        // TODO: this isn't a perfect solution: if some code inspects a code
        //       cache return address and then displaces it then we will
        //       have a problem (moreso in user space; kernel space is easier
        //       to detect code cache addresses).
        uint64_t addr_uint(reinterpret_cast<uint64_t>(app_target_addr));
        uint32_t *header_addr(reinterpret_cast<uint32_t *>(
            addr_uint + 16 - RETURN_ADDRESS_OFFSET));

        if(is_code_cache_address(app_target_addr)
        && (RETURN_ADDRESS_OFFSET == (addr_uint % 8)
            && basic_block_info::HEADER == *header_addr)) {

            target_addr = instruction_list_mangler::ibl_exit_routine(
                app_target_addr);
            CODE_CACHE->store(addr.as_address, target_addr);
            D( printf(" -> %p (return)\n", target_addr); )
            return target_addr;
        }
#endif

        // Determine if this is actually a detach point. This is only relevant
        // for indirect calls/jumps because direct calls and jumps will have
        // inlined this check at basic block translation time.
        target_addr = find_detach_target(app_target_addr);

        // handle detaching, e.g. through a wrapper or through granary::detach.
        if(nullptr != target_addr) {
            if(policy.is_indirect_cti_policy()) {
                target_addr = instruction_list_mangler::ibl_exit_routine(
                    target_addr);
            }

            CODE_CACHE->store(addr.as_address, target_addr);
            cpu->code_cache.store(addr.as_address, target_addr);
            D( printf(" -> %p (detach)\n", target_addr); )
            return target_addr;
        }

        // figure out the non-policy-mangled target address, and get our policy.
        instrumentation_policy base_policy(policy.base_policy());
        mangled_address base_addr(app_target_addr, base_policy);

        // translate the basic block according to the policy.
        basic_block bb(basic_block::translate(
            base_policy, cpu, thread, app_target_addr));

        target_addr = bb.cache_pc_start;

        // store the translated block in the code cache; if the store fails,
        // then that means the block already exists in the code cache (e.g.
        // because a concurrent thread "won" when translating the same
        // block). If the latter is the case, implicitly free the block
        // by freeing the last thing used in the fragment allocator.
        if(!CODE_CACHE->store(base_addr.as_address, target_addr, HASH_KEEP_PREV_ENTRY)) {
            cpu->fragment_allocator.free_last();
            cpu->block_allocator.free_last();

            // TODO: minor memory leak with basic block state and vtables.
            //       consider switching to a "transactional" allocator.
            CODE_CACHE->load(base_addr.as_address, target_addr);
        }

        D( printf(" -> %p (translated)\n", target_addr); )

        // TODO: try to pre-load the cache with internal jump targets of the
        //       just-stored basic block (if config option permits).

        // propagate down to the CPU-private code cache.
        cpu->code_cache.store(base_addr.as_address, target_addr);

        // was this an indirect entry? If so, we need to direct control flow
        // to the corresponding IBL exit routine.
        if(policy.is_indirect_cti_policy()) {
            target_addr = instruction_list_mangler::ibl_exit_routine(target_addr);
            if(!CODE_CACHE->store(addr.as_address, target_addr, HASH_KEEP_PREV_ENTRY)) {
                cpu->fragment_allocator.free_last();
                CODE_CACHE->load(addr.as_address, target_addr);
            }

            cpu->code_cache.store(addr.as_address, target_addr);

            D( printf(" -> %p (indirect)\n", target_addr); )
        }

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

