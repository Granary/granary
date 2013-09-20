/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * events.cc
 *
 *  Created on: 2013-08-15
 *      Author: Peter Goodman
 */

#include "clients/watchpoints/clients/rcudbg/instrument.h"
#include "clients/watchpoints/clients/rcudbg/events.h"
#include "clients/watchpoints/utils.h"

#include "granary/detach.h"

#ifndef DETACH_ADDR___granary_rcu_read_lock
#   error "RCU debugging requires that `__granary_rcu_read_lock` be defined."
#endif

using namespace granary;


namespace client {

    enum {
        IS_DEREF_BIT = 63
    };


    /// Generates and returns a function to be called when a watched address
    /// is dereferenced.
    ///
    /// Note: we can freely clobber the CF.
    static app_pc instrumented_access(operand reg, bool is_read) throw() {
        static app_pc WATCHED_VISITORS[16][2] = {nullptr};
        const unsigned reg_id(wp::register_to_index(reg.value.reg));

        if(WATCHED_VISITORS[reg_id][is_read]) {
            return WATCHED_VISITORS[reg_id][is_read];
        }

        instruction_list ls;
        instruction label_is_deref = label_();
        instruction cti;

        if(reg::arg1.value.reg != reg.value.reg) {
            ls.append(push_(reg::arg1));
            ls.append(mov_ld_(reg::arg1, reg));
        }

        ls.append(bt_(reg::arg1, int8_(IS_DEREF_BIT)));
        ls.append(jnb_(instr_(label_is_deref)));

        // Accessing a watched object that is the result of an
        // `rcu_assign_pointer`.

        insert_cti_after(
            ls, ls.last(), EVENT_ACCESS_ASSIGNED_POINTER,
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_CALL).set_mangled();

        if(reg::arg1.value.reg != reg.value.reg) {
            ls.append(pop_(reg::arg1));
        }
        ls.append(ret_());

        // Accessing a watched object that is hte result of an
        // `rcu_dereference`.
        ls.append(label_is_deref);

        if(!is_read) {
            insert_cti_after(
                ls, ls.last(), EVENT_WRITE_TO_DEREF_POINTER,
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL).set_mangled();
        } else {
            insert_cti_after(
                ls, ls.last(), EVENT_READ_FROM_DEREF_POINTER,
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL).set_mangled();
        }

        if(reg::arg1.value.reg != reg.value.reg) {
            ls.append(pop_(reg::arg1));
        }
        ls.append(ret_());

        app_pc ret(global_state::FRAGMENT_ALLOCATOR->allocate_array<uint8_t>(
            ls.encoded_size()));

        ls.encode(ret);
        WATCHED_VISITORS[reg_id][is_read] = ret;

        return ret;
    }


    /// Returns the policy for a given read-side critical section nesting
    /// depth.
    instrumentation_policy policy_for_depth(
        const unsigned depth
    ) throw() {
        switch(depth) {
        case 0: return policy_for<rcu_null>();
        case 1: return policy_for<read_critical_section<1>>();
        case 2: return policy_for<read_critical_section<2>>();
        case 3: return policy_for<read_critical_section<3>>();
        default:
            static_assert(4 == MAX_SECTION_DEPTH,
                "Update this function to match `MAX_SECTION_DEPTH`.");
        }

        ASSERT(false);
        return policy_for<rcu_null>();
    }


    /// Add CTI-specific instrumentation for an RCU read-side critical section.
    instruction rcu_instrument_cti(
        instrumentation_policy &curr_policy,
        unsigned &curr_depth,
        instruction_list &ls,
        instruction in
    ) throw() {

        // Here we have a read-critical section started in one function and
        // expanding out into another function through a function return. This
        // seems like a potential place where a read-side critical section is
        // left open, so we report the issue. It is also likely bad practice to
        // extend read-side critical sections in this way because they should be
        // short.
        if(curr_depth && in.is_return()) {
            instruction ret(label_());

            ls.insert_before(in, ret);
            ls.remove(in);

            ret = insert_cti_after(
                ls, ret,
                EVENT_READ_THROUGH_RET,
                CTI_STEAL_REGISTER, reg::r10,
                CTI_CALL);
            ret.set_mangled();

            // Emulated RET.
            ret = ls.insert_after(ret, mangled(pop_(reg::r10)));
            ret = ls.insert_after(ret, jmp_ind_(reg::r10));
            ret.set_policy(curr_policy);

            // TODO: We should also emit a check on the return address to see
            //       if it's executing under a policy that's not `rcu_null` or
            //       not native.

            return ret;
        }

        // Assumes that all indirect CTIs don't target RCU functions of
        // interest.
        const operand target(in.cti_target());
        if(!dynamorio::opnd_is_pc(target)) {
            in.set_policy(curr_policy);
            return in;
        }

        const app_pc target_pc(target.value.pc);
        switch(reinterpret_cast<uintptr_t>(target_pc)) {

        // Read lock case; don't change the policy of the CTI,
        // but change the policy to increase the depth.
        case DETACH_ADDR___granary_rcu_read_lock:
            curr_policy = policy_for_depth(++curr_depth);
            curr_policy.force_attach(true);
            break;

        // Read unlock case; don't change the policy of the CTI,
        // but change the policy to decrease the depth.
        case DETACH_ADDR___granary_rcu_read_unlock:
            if(0 < curr_depth) {
                curr_policy = policy_for_depth(--curr_depth);
                curr_policy.force_attach(true);
            }
            break;

        /// Not an event that we care about; make sure to attach to all code
        /// running, including kernel code.
        default:
            in.set_policy(curr_policy);
            break;
        }

        return in;
    }


    /// Return the next valid PC in the instruction list, starting at
    /// instruction `in`.
    static app_pc next_pc(instruction in) throw() {
        for(; in.is_valid(); in = in.next()) {
            if(in.pc()) {
                return in.pc();
            }
        }
        return nullptr;
    }


    /// "Null" RCU debugging policy. This instruments any code executing
    /// outside of a read-side critical section. This code upgrades to
    /// writer code on a fault.
    instrumentation_policy rcu_null::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        instruction_list &ls
    ) throw() {

        // Upgrade patch point for write-side debugging. If this basic block
        // faults while running (i.e. because it writes to an RCU-protected
        // data structure) then it is either in an area where the writer is
        // updating the data structure, or doing something wrong.

        instruction first(ls.first());

        instruction patch_target(ls.prepend(label_()));
        ls.prepend(mangled(patchable(jmp_short_(instr_(patch_target)))));

        unsigned curr_depth(0);
        instrumentation_policy curr_policy(policy_for_depth(curr_depth));

        for(instruction in = first; in.is_valid(); ) {
            instruction next_in(in.next());

            if(in.is_cti()) {
                in = rcu_instrument_cti(curr_policy, curr_depth, ls, in);
            }

            // Chop the instruction list short because we've transitioned
            // from null to the watchpoints-based RCU debugger.
            if(0 < curr_depth) {
                const app_pc fall_through_pc(next_pc(next_in));
                ASSERT(fall_through_pc);
                ls.remove_tail_at(next_in);

                in = ls.append(jmp_(pc_(fall_through_pc)));
                in.set_policy(curr_policy);
                in.set_pc(fall_through_pc);
                break;
            }

            in = next_in;
        }

        return curr_policy;
    }


    /// Host/app are the same.
    instrumentation_policy rcu_null::visit_host_instructions(
        granary::cpu_state_handle cpu,
        granary::basic_block_state &bb,
        instruction_list &ls
    ) throw() {
        return visit_app_instructions(cpu, bb, ls);
    }


    /// Interrupted in `rcu_null`, upgrade to an RCU-writer protocol. We will
    /// hot-patch the original basic block to redirect control-flow to the
    /// upgraded basic block. We will also try to instrument the tail of the
    /// interrupted basic block.
    interrupt_handled_state rcu_null::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector
    ) throw() {
        // TODO: upgrade basic block!
        return INTERRUPT_DEFER;
    }

    namespace wp {
        void rcu_read_policy::visit_read(
            granary::basic_block_state &,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            if(DEST_OPERAND & tracker.ops[i].kind) {
                return; // Don't double-instrument a read&write access.
            }
            insert_cti_after(
                ls, tracker.labels[i],
                instrumented_access(tracker.regs[i], true),
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL).set_mangled();
        }


        void rcu_read_policy::visit_write(
            granary::basic_block_state &,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            insert_cti_after(
                ls, tracker.labels[i],
                instrumented_access(tracker.regs[i], false),
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL).set_mangled();
        }


        interrupt_handled_state rcu_read_policy::handle_interrupt(
            cpu_state_handle,
            thread_state_handle,
            granary::basic_block_state &,
            interrupt_stack_frame &,
            interrupt_vector
        ) throw() {
            return INTERRUPT_DEFER;
        }
    }
}

