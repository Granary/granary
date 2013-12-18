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
        } while(!BASIC_BLOCKS.compare_exchange_strong(prev, curr));
    }


    /// Invoked if/when Granary discards a basic block (e.g. due to a race
    /// condition when two cores compete to translate the same basic block).
    void discard_basic_block(basic_block_state &bb) throw() {
#if CFG_RECORD_INDIRECT_TARGETS
        if(bb.indirect_ctis) {
            free_memory<indirect_cti>(bb.indirect_ctis, bb.num_indirect_ctis);
        }
#else
        UNUSED(bb);
#endif
    }


#if CFG_RECORD_INDIRECT_TARGETS
    /// Add an entry to a basic block's set of indirect CALL/JMP targets.
    void add_indirect_target(app_pc target_addr, indirect_cti *cti) throw() {
        cti->update_lock.acquire();

        app_pc *new_targets(nullptr);

        // Search and possibly add.
        for(unsigned i(0); i < cti->num_indirect_targets; ++i) {
            if(!cti->indirect_targets[i]) {
                cti->indirect_targets[i] = target_addr;
                goto done;
            } else if(target_addr == cti->indirect_targets[i]) {
                goto done;
            }
        }

        // Resize and copy.
        new_targets = allocate_memory<app_pc>(cti->num_indirect_targets * 2);
        memcpy(
            new_targets, cti->indirect_targets,
            cti->num_indirect_targets * sizeof(app_pc));

        // Free
        free_memory<app_pc>(cti->indirect_targets, cti->num_indirect_targets);

        // Update
        cti->num_indirect_targets *= 2;
        cti->indirect_targets = new_targets;

    done:
        cti->update_lock.release();
    }


    static app_pc EVENT_ADD_INDIRECT_TARGET = nullptr;


    STATIC_INITIALISE_ID(event_add_indirect_target, {
        EVENT_ADD_INDIRECT_TARGET = generate_clean_callable_address(
            &add_indirect_target, EXIT_REGS_ABI_COMPATIBLE);
    })
#endif


    /// Insert an execution counter that saves and restores the flags, as well
    /// as protects against the user space red zone.
    static void insert_exec_count_before(
        instruction_list &ls,
        instruction in,
        uint64_t *counter
    ) throw() {
        IF_USER( ls.insert_before(in,
            lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )
        ls.insert_before(in, pushf_());
        ls.insert_before(in, inc_(absmem_(counter, dynamorio::OPSZ_8)));
        ls.insert_before(in, popf_());
        IF_USER( ls.insert_before(in,
            lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )
    }


#if CFG_RECORD_INDIRECT_TARGETS
    /// Add in the instrumentation for an individual indirect CTI.
    static void instrument_indirect_cti(
        instruction_list &ls,
        indirect_cti &cti
    ) throw() {
        instruction hit_in_last_indirect_target(label_());
        ls.append(lea_(
            reg::indirect_clobber_reg,
            data_label_(&(cti.last_indirect_target))));

        // Check if our current target is in our one-item cache. If not, then
        // update the cache and ensure that it's in the set, otherwise jump
        // around the update and clean call and go right to the IBL.
        ls.append(cmp_(reg::indirect_target_addr, *reg::indirect_clobber_reg));
        ls.append(jz_(instr_(hit_in_last_indirect_target)));

        // Update the `last_indirect_target` in the `indirect_cti` with the
        // current target.
        ls.append(mov_st_(*reg::indirect_clobber_reg, reg::indirect_target_addr));

        // Call out and update the indirect target.
        insert_clean_call_after(ls, ls.last(), EVENT_ADD_INDIRECT_TARGET,
            reg::indirect_target_addr, data_label_(&cti));

        ls.append(hit_in_last_indirect_target);

        // Initialize the `indirect_cti` data structure for this jump.
        cti.indirect_targets = allocate_memory<app_pc>(4);
        cti.num_indirect_targets = 4;

    }
#endif


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
#if CFG_RECORD_INDIRECT_TARGETS
            if(!bb.num_indirect_ctis) {
                return *this;
            }

            ASSERT(nullptr != bb.indirect_ctis);
            for(unsigned i(0); i < bb.num_indirect_ctis; ++i) {
                if(!bb.indirect_ctis[i].indirect_targets) {
                    instrument_indirect_cti(ls, bb.indirect_ctis[i]);
                    break;
                }
            }
#endif
            return *this;
        }

        // This isn't a recursive invocation for an indirect CTI, instrument
        // the basic block.

        instruction in;
        UNUSED(in);

#if CFG_RECORD_INDIRECT_TARGETS
        // Count the number of indirect CTIs.
        unsigned num_indirect_ctis(0);
        for(in = ls.first(); in.is_valid(); in = in.next()) {
            if(!in.is_cti()) {
                continue;
            }

            const operand target(in.cti_target());
            if(dynamorio::PC_kind != target.kind) {
                ++num_indirect_ctis;
            }
        }

        indirect_cti *ctis(nullptr);
        if(0 < num_indirect_ctis) {
            ctis = allocate_memory<indirect_cti>(num_indirect_ctis);
        }
        bb.num_indirect_ctis = num_indirect_ctis;
        bb.indirect_ctis = ctis;
#endif

        // Add in a special label into the instruction list. This will give us
        // access later on to the encoded location of this basic block, which
        // will allow us to access its internal meta-info.
        instruction label(ls.prepend(persistent_label_(bb.label)));

#if CFG_RECORD_EXEC_COUNT

        // We found a good place to insert the increment where the flags
        // modifications done by the increment won't alter the behaviour
        // of the instrumented program.
        if(find_arith_flags_dead_after(ls, in)) {
            ls.insert_before(in,
                inc_(absmem_(&(bb.num_executions), dynamorio::OPSZ_8)));

        // We didn't find a good place to add in the execution counter; place
        // it at the beginning of the basic block.
        } else {
            insert_exec_count_before(ls, label, &(bb.num_executions));
        }

#   if CFG_RECORD_FALL_THROUGH_COUNT
        // Count how many times we skip over a conditional branch and execute
        // the fall-through basic block.
        for(in = ls.last(); in.is_valid(); in = in.prev()) {
            if(!in.is_cti() || !dynamorio::instr_is_cbr(in)) {
                continue;
            }

            in = in.next();
            insert_exec_count_before(ls, in, &(bb.num_fall_through_executions));
            break;
        }
#   endif
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

