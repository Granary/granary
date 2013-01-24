/*
 * code_cache.cc
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/code_cache.h"
#include "granary/detach.h"
#include "granary/hash_table.h"
#include "granary/basic_block.h"

namespace granary {

    namespace {


        /// The globally shared code cache. This maps policy-mangled code
        /// code addresses to translated addresses.
        static shared_hash_table<app_pc, app_pc> CODE_CACHE;


        /// This is a bit of a hack. The idea here is that we want an example
        /// of an address within the current address space, so that we can use
        /// its high-order bits to "fix up" an address that encodes a policy in
        /// its high-order bits. This should be independent of user/kernel
        /// space, at least for typical 48-bit address space implementations
        /// use the high-order bits should be all 0 or all 1.
        static std::remove_reference<
            decltype(**(new app_pc))
        >::type SOME_ADDRESS;


        /// This is an "unmangled" mangled version of the address of
        /// SOME_ADDRESS. It exists only for convenient extraction of the high-
        /// order bits through the `address_mangler` fields.
        static address_mangler UNMANGLED_ADDRESS;
    }


    /// Initialise the above hack.
    STATIC_INITIALISE({
        UNMANGLED_ADDRESS.as_address = &SOME_ADDRESS;
    });


    /// Perform both lookup and insertion (basic block translation) into
    /// the code cache.
    app_pc code_cache::find(cpu_state_handle &cpu,
                            thread_state_handle &thread,
                            address_mangler addr) throw() {

        // find the actual targeted address, independent of the policy.
        address_mangler app_target_addr(addr);
        app_target_addr.as_policy_address.policy_id = \
            UNMANGLED_ADDRESS.as_policy_address.policy_id;


        // Determine if this is actually a detach point. This is only relevant
        // for indirect calls/jumps because direct calls and jumps will have
        // inlined this check at basic block translation time.
        app_pc target_addr(find_detach_target(app_target_addr.as_address));
        if(nullptr != target_addr) {
            return target_addr;
        }

        // Try to load the target address from the global code cache.
        if(CODE_CACHE.load(addr.as_address, target_addr)) {
            return target_addr;
        }

        // figure out the non-policy-mangled target address, and get our policy.
        app_pc decode_addr(app_target_addr.as_address);
        instrumentation_policy policy(addr.as_policy_address.policy_id);

        // translate the basic block according to the policy.
        basic_block bb(basic_block::translate(
            policy, cpu, thread, &decode_addr));

        target_addr = bb.cache_pc_start;

        // store the translated block in the code cache; if the store fails,
        // then that means the block already exists in the code cache (e.g.
        // because a concurrent thread "won" when translating the same
        // block). If the latter is the case, implicitly free the block
        // by freeing the last thing used in the fragment allocator.
        if(!CODE_CACHE.store(addr.as_address, target_addr, false)) {
            cpu->fragment_allocator.free_last();

            // TODO: minor memory leak with basic block state and vtables.
            //       consider switching to a "transactional" allocator.
            CODE_CACHE.load(addr.as_address, target_addr);
        }

        // TODO: try to pre-load the cache with internal jump targets of the
        //       just-stored basic block (if config option permits).

        return target_addr;
    }

    /*
    /// Add an entry to the code cache for later prediction.
    void code_cache::predict(cpu_state_handle &cpu,
                             thread_state_handle &thread,
                             app_pc app_code,
                             app_pc cache_code) throw() {

        CODE_CACHE.store(app_code, cache_code);
        //CODE_CACHE.store(cache_code, cache_code);

        (void) cpu;
        (void) thread;
    }
    */
}
