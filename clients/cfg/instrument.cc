/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#include "clients/cfg/instrument.h"
#include "clients/cfg/config.h"

#include "granary/hash_table.h"

using namespace granary;


#if CONFIG_ENV_KERNEL
#   include "granary/kernel/linux/module.h"
extern "C" {
    /// Returns the kernel module information for a given app address.
    extern const kernel_module *kernel_get_module(app_pc addr);
}
#endif


namespace client {


    /// Used to link together all basic blocks.
    std::atomic<basic_block_state *> BASIC_BLOCKS = \
        ATOMIC_VAR_INIT(nullptr);


    /// Chain the basic block into the list of basic blocks.
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


    /// Add an entry to a basic block's set of indirect CALL/JMP targets.
    void add_indirect_target(app_pc target_addr, app_pc source_bb) throw() {
        //basic_block bb(source_bb);

        (void) target_addr;
        (void) source_bb;

        //printf("%p\n", target_addr);
    }


    static app_pc EVENT_ADD_INDIRECT_TARGET = nullptr;


    STATIC_INITIALISE_ID(event_add_indirect_target, {
        EVENT_ADD_INDIRECT_TARGET = generate_clean_callable_address(
            &add_indirect_target, EXIT_REGS_ABI_COMPATIBLE);
    })


    /// Add in instrumentation so that we can later figure out the control-flow
    /// graph.
    granary::instrumentation_policy cfg_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        // This is a recursive invocation of instrumentation on an indirect
        // CTI that loads from memory. We can find the CTI's address in
        // `reg::indirect_target_addr`.
        if(ls.is_stub()) {
            instruction in(ls.prepend(label_()));
            insert_clean_call_after(ls, in, EVENT_ADD_INDIRECT_TARGET,
                reg::indirect_target_addr, mem_instr_(&bb.label));

            return *this;
        }

        bool added_exec_count(false);
        IF_USER( bool redzone_safe(false); )
#if CFG_RECORD_EXEC_COUNT

        unsigned eflags(0);
        for(instruction in(ls.last()); in.is_valid(); in = in.prev()) {

            // Assumes flags are dead before CALL/RET.
            if(in.is_call() || in.is_return()) {
                IF_USER( redzone_safe = true; )
                goto found_insert_point;
            }

            if(in.is_jump() && dynamorio::PC_kind != in.cti_target().kind) {
                goto found_insert_point;
            }

            eflags = dynamorio::instr_get_eflags(in);
            if(!(eflags & EFLAGS_WRITE_ALL) || (eflags & EFLAGS_READ_ALL)) {
                continue;
            }

            // We've found a good insertion point for the execution counter
            // increment.
        found_insert_point:

            IF_USER(
                if(!redzone_safe) {
                    ls.insert_before(in, lea_(reg::rsp, reg::rsp[-REDZONE_SIZE]));
                }
            )

            ls.insert_before(in,
                inc_(absmem_(&(bb.num_executions), dynamorio::OPSZ_8)));

            IF_USER(
                if(!redzone_safe) {
                    ls.insert_before(in, lea_(reg::rsp, reg::rsp[REDZONE_SIZE]));
                }
            )

            added_exec_count = true;
            break;
        }
#endif

        // Add in a special label into the instruction list. This will give us
        // access later on to the encoded location of this basic block, which
        // will allow us to access its internal meta-info.
        bb.label.opcode = dynamorio::OP_LABEL;
        bb.label.flags |= dynamorio::INSTR_OPERANDS_VALID;
        instruction label(&(bb.label));
        ls.prepend(label);

#if CFG_RECORD_EXEC_COUNT
        // We didn't find a good place to add in the execution counter; place
        // it at the beginning of the basic block.
        if(!added_exec_count) {
            IF_USER( ls.insert_before(label,
                lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )
            ls.insert_before(label, pushf_());
            ls.insert_before(label,
                inc_(absmem_(&(bb.num_executions), dynamorio::OPSZ_8)));
            ls.insert_before(label, popf_());
            IF_USER( ls.insert_before(label,
                lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )
        }
#endif

        return *this;
    }


    granary::instrumentation_policy cfg_policy::visit_host_instructions(
        granary::cpu_state_handle cpu,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        return visit_app_instructions(cpu, bb, ls);
    }


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT

    granary::interrupt_handled_state cfg_policy::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }


    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }

#endif

}

