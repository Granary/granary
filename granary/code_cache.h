/*
 * code_cache.h
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_CODE_CACHE_H_
#define Granary_CODE_CACHE_H_

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/hash_table.h"

namespace granary {

    struct code_cache {
    private:

        static hash_table<app_pc, app_pc> CODE_CACHE;

    public:

        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        inline static app_pc find(app_pc addr) throw() {
            cpu_state_handle cpu;
            thread_state_handle thread;
            return find(cpu, thread, addr);
        }


        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        static app_pc find(cpu_state_handle &cpu,
                           thread_state_handle &thread,
                           app_pc addr) throw();
    };
}

#endif /* Granary_CODE_CACHE_H_ */
