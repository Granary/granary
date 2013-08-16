/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * events.cc
 *
 *  Created on: 2013-08-15
 *      Author: Peter Goodman
 */

#include "clients/watchpoints/clients/rcudbg/instrument.h"
#include "clients/watchpoints/utils.h"

#include "granary/detach.h"

#ifndef DETACH_ADDR___granary_rcu_read_lock
#   error "RCU debugging requires that `__granary_rcu_read_lock` be defined."
#endif


#define DECLARE_READ_ACCESSOR(reg) \
    extern void CAT(granary_rcu_policy_read_, reg)(void);


#define DECLARE_READ_ACCESSORS(reg, rest) \
    DECLARE_READ_ACCESSOR(reg) \
    rest

#define DESCRIPTOR_READ_ACCESSOR_PTR(reg) \
    &CAT(granary_rcu_policy_read_, reg)


#define DESCRIPTOR_READ_ACCESSOR_PTRS(reg, rest) \
    DESCRIPTOR_READ_ACCESSOR_PTR(reg), \
    rest


extern "C" {
    ALL_REGS(DECLARE_READ_ACCESSORS, DECLARE_READ_ACCESSOR)
}



using namespace granary;


namespace client {

    /// Invoked when an RCU read-side critical section extend beyond the
    /// function in which the read-side critical section was started. This seems
    /// like bad practice as it splits up the read-critical sections, and makes
    /// it more likely for bugs to creep in. This could also be evidence of a
    /// read-side critical section being left locked.
    app_pc EVENT_READ_THROUGH_RET = nullptr;


    /// A trailing call to a read-side critical section unlock point is invoked.
    /// This unlock does not match a lock.
    app_pc EVENT_TRAILING_RCU_READ_UNLOCK = nullptr;


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

        const unsigned start_depth(curr_depth);

        // Here we have a read-critical section started in one function and
        // expanding out into another function through a function return. This
        // seems like a potential place where a read-side critical section is
        // left open, so we report the issue. It is also likely bad practice to
        // extend read-side critical sections in this way because they should be
        // short.
        if(in.is_return()) {
            instruction ret(label_());

            ls.insert_before(in, ret);
            ls.remove(in);

            ret = insert_cti_after(
                ls, ret,
                EVENT_READ_THROUGH_RET,
                CTI_STEAL_REGISTER, reg::r10,
                CTI_CALL);

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

            if(0 == curr_depth) {
                instruction call(ls.insert_before(in, label_()));
                call = insert_cti_after(
                    ls, call,
                    EVENT_TRAILING_RCU_READ_UNLOCK,
                    CTI_DONT_STEAL_REGISTER, operand(),
                    CTI_CALL);
            } else {
                curr_policy = policy_for_depth(--curr_depth);
                if(0 < curr_depth) {
                    curr_policy.force_attach(true);
                }
            }

            break;

        // RCU dereference.
        case DETACH_ADDR___granary_rcu_dereference:
            // TODO
            break;

        // RCU assign pointer. Likely shouldn't be in a read-critical section,
        // but can be outside of one.
        case DETACH_ADDR___granary_rcu_assign_pointer:
            // TODO
            break;

        /// Not an event that we care about; make sure to attach to all code
        /// running, including kernel code.
        default:
            in.set_policy(curr_policy);
            break;
        }

        return in;
    }


    /// "Null" RCU debugging policy. This instruments any code executing
    /// outside of a read-side critical section. This code is upgrades to
    /// writer code on a fault.
    instrumentation_policy rcu_null::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        instruction_list &ls
    ) throw() {

        // Upgrade patch point for write-side debugging. If this basic block
        // faults while running (i.e. because it writes to a RCU-protected
        // data structure) then it is either a
        ls.prepend(patchable(nop_()));

        unsigned curr_depth(0);
        instrumentation_policy curr_policy(policy_for_depth(curr_depth));

        for(instruction in(ls.first()); in.is_valid(); ) {
            instruction next_in(in.next());

            if(in.is_cti()) {
                in = rcu_instrument_cti(curr_policy, curr_depth, ls, in);
            }

            // Chop the instruction list short because we've transitioned
            // from null to the watchpoints-based RCU debugger.
            if(0 < curr_depth) {
                ASSERT(next_in.is_valid() && next_in.pc());
                const app_pc fall_through_pc(next_in.pc());
                ls.remove_tail_at(next_in);
                in = ls.append(jmp_(pc_(fall_through_pc)));
                in.set_policy(curr_policy);
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
        visit_app_instructions(cpu, bb, ls);
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

}

