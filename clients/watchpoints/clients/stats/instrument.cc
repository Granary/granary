/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-07-18
 *      Author: Peter Goodman
 */

#include <atomic>

#include "clients/watchpoints/clients/stats/instrument.h"

using namespace granary;

namespace client {


    std::atomic<basic_block_state *> BASIC_BLOCKS = ATOMIC_VAR_INIT(nullptr);


    /// Chain the basic block into the list of basic blocks.
    void commit_to_basic_block(basic_block_state &bb) {
        basic_block_state *prev(nullptr);
        basic_block_state *curr(&bb);
        do {
            prev = BASIC_BLOCKS.load();
            curr->next = prev;
        } while(!BASIC_BLOCKS.compare_exchange_weak(prev, curr));
    }


    /// Get rid of a basic block.
    void discard_basic_block(basic_block_state &) { }
}

namespace client { namespace wp {


    /// Instrument a memory read.
    void stats_policy::visit_read(
        granary::basic_block_state &bb,
        instruction_list &ls,
        watchpoint_tracker &s,
        unsigned i
    ) {
        bb.num_memory_ops += 1;

        instruction in(s.labels[i]);

        in = ls.insert_after(in, pushf_());
        in = ls.insert_after(in,
            inc_(absmem_(&(bb.num_watched_memory_ops), dynamorio::OPSZ_8)));
        in = ls.insert_after(in, popf_());
    }


    /// Instrument a memory write.
    void stats_policy::visit_write(
        granary::basic_block_state &bb,
        instruction_list &ls,
        watchpoint_tracker &s,
        unsigned i
    ) {
        if(DEST_OPERAND == s.ops[i].kind) {
            visit_read(bb, ls, s, i);
        }
    }

}}

namespace client {

    /// Visit app instructions (module, user program)
    instrumentation_policy watchpoint_stats_policy::visit_app_instructions(
        cpu_state_handle cpu,
        granary::basic_block_state &bb,
        instruction_list &ls
    ) {
        client::watchpoints<
            wp::stats_policy, wp::stats_policy
        >::visit_host_instructions(cpu, bb, ls);

        instruction in(ls.prepend(label_()));
        in = ls.insert_after(in, pushf_());
        in = ls.insert_after(in,
            inc_(absmem_(&(bb.num_executions), dynamorio::OPSZ_8)));
        in = ls.insert_after(in, popf_());

        return policy_for<watchpoint_stats_policy>();
    }


    /// Visit host instructions (module, user program)
    instrumentation_policy watchpoint_stats_policy::visit_host_instructions(
        cpu_state_handle cpu,
        granary::basic_block_state &bb,
        instruction_list &ls
    ) {
        client::watchpoints<
            wp::stats_policy, wp::stats_policy
        >::visit_host_instructions(cpu, bb, ls);

        instruction in(ls.prepend(label_()));
        in = ls.insert_after(in, pushf_());
        in = ls.insert_after(in,
            inc_(absmem_(&(bb.num_executions), dynamorio::OPSZ_8)));
        in = ls.insert_after(in, popf_());

        return policy_for<watchpoint_stats_policy>();
    }

}

#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
namespace client { namespace wp {

    interrupt_handled_state stats_policy::handle_interrupt(
        cpu_state_handle,
        thread_state_handle,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector
    ) {
        return INTERRUPT_DEFER;
    }
}} /* wp namespace */
#endif /* CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT */

