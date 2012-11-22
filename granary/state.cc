/*
 * state.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/state.h"

namespace granary {


    void thread_state::add_direct_branch_generation(void) throw() {
        ++generation_number;
    }


    void thread_state::clear_direct_branch_generation(void) throw() {
        for(unsigned i(0); i < NUM_DIRECT_BRANCH_SLOTS; ++i) {
            if(generation_number == used_slots[i]) {
                used_slots[i] = 0;
            }
        }
        --generation_number;
    }


    /// Find the next free branch lookup slot.
    direct_branch_slot *thread_state::add_direct_branch(void) throw() {
        for(unsigned i(0); i < NUM_DIRECT_BRANCH_SLOTS; ++i) {
            if(!used_slots[i]) {
                used_slots[i] = generation_number;
                return &(direct_branch_slots[i]);
            }
        }
        return nullptr;
    }
}
