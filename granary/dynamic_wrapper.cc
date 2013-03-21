/*
 * dynamic_wrapper.cc
 *
 *  Created on: 2013-03-20
 *      Author: pag
 */

#include "granary/globals.h"
#include "granary/code_cache.h"
#include "granary/hash_table.h"
#include "granary/policy.h"
#include "granary/instruction.h"
#include "granary/emit_utils.h"


namespace granary {

    static static_data<locked_hash_table<app_pc, app_pc>> wrappers;


    STATIC_INITIALISE({
        wrappers.construct();
    })


    /// Return the dynamic wrapper address for a wrapper / wrappee.
    app_pc dynamic_wrapper_of(app_pc wrapper, app_pc wrappee) throw() {
        app_pc target_wrapper(nullptr);
        if(wrappers->load(wrappee, target_wrapper)) {
            return target_wrapper;
        }

        app_pc target_code_cache(code_cache::find(wrappee, START_POLICY));

        // build the list to jump to that entry. This will put the address of
        // the code cache version of `wrappee` into `%r10` and then jump to
        // the generic wrapper (`wrapper`), which pulls its target out of
        // `%r10`.
        instruction_list ls;
        instruction in = ls.append(mov_imm_(
            reg::r10, int64_(reinterpret_cast<uint64_t>(target_code_cache))));
        insert_cti_after(
            ls, in, wrapper, false, operand(), CTI_JMP);

        // encode it
        target_wrapper = global_state::WRAPPER_ALLOCATOR-> \
            allocate_array<uint8_t>(ls.encoded_size());
        ls.encode(target_wrapper);

        // store it for later and return
        wrappers->store(wrappee, target_wrapper);
        return target_wrapper;
    }

}
