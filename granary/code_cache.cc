/*
 * code_cache.cc
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/code_cache.h"
#include "granary/basic_block.h"
#include "granary/detach.h"

namespace granary {

    /// Static initialization.
    hash_table<app_pc, app_pc> code_cache::CODE_CACHE;


    app_pc code_cache::find(cpu_state_handle &cpu,
                              thread_state_handle &thread,
                              app_pc addr) throw() {

        app_pc target_addr(find_detach_target(addr));
        if(nullptr != target_addr) {
            return target_addr;
        }

        if(CODE_CACHE.load(addr, &target_addr)) {
            return target_addr;
        }

        app_pc decode_addr(addr);
        basic_block bb(basic_block::translate(cpu, thread, &decode_addr));
        CODE_CACHE.store(addr, bb.cache_pc_start);

        return bb.cache_pc_start;
    }

}
