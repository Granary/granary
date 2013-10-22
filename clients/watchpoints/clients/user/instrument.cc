/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-10-21
 *      Author: Peter Goodman
 */


#include "clients/watchpoints/clients/user/instrument.h"

using namespace granary;

namespace client {


    /// Linked list of basic blocks that have been instrumented.
    std::atomic<basic_block_state *> BASIC_BLOCKS = ATOMIC_VAR_INIT(nullptr);


    DEFINE_READ_VISITOR(user_policy, {
        if(!tracker.policy.accesses_user_data()) {

            instruction not_a_user_address(label_());
            ls.insert_before(pre_label,
                bt_(op, int8_(DISTINGUISHING_BIT_OFFSET - 1)));
            ls.insert_before(pre_label,
                mangled(IF_USER_ELSE(jnb_, jb_)(instr_(not_a_user_address))));

            ls.insert_before(pre_label, push_(reg::rax));
            ls.insert_before(pre_label, mov_imm_(reg::al, int8_(1)));
            ls.insert_before(pre_label, mov_st_(
                absmem_(&(bb.accesses_user_data), dynamorio::OPSZ_1),
                reg::al));
            ls.insert_before(pre_label, pop_(reg::rax));
            ls.insert_before(pre_label, not_a_user_address);

            if(!bb.instruction_address) {
                bb.instruction_address = tracker.in.pc();
            }
        }
    })


    DEFINE_WRITE_VISITOR(user_policy, {
        if(!(SOURCE_OPERAND & tracker.ops[i].kind)) {
            visit_read(bb, ls, tracker, i);
        }
    })


    DEFINE_INTERRUPT_VISITOR(user_policy, { })


    /// Commit to instrumenting this basic block. Chain the basic block into
    /// the list of basic blocks.
    void commit_to_basic_block(basic_block_state &bb) throw() {
        basic_block_state *prev(nullptr);
        basic_block_state *curr(&bb);
        do {
            prev = BASIC_BLOCKS.load();
            curr->next = prev;
        } while(!BASIC_BLOCKS.compare_exchange_weak(prev, curr));
    }


    /// Invoked if/when Granary discards a basic block (e.g. due to a race
    /// condition when two cores compete to translate the same basic block).
    void discard_basic_block(basic_block_state &) throw() { }

} /* client namespace */



