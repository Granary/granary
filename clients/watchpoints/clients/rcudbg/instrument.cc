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
        ls.append(jb_(instr_(label_is_deref)));

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

        const unsigned size(ls.encoded_size());
        app_pc ret(global_state::FRAGMENT_ALLOCATOR->allocate_array<uint8_t>(
            size));

        ls.encode(ret, size);
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

        // Previous and/or current policy, but with force attach as false.
        instrumentation_policy in_policy = curr_policy;
        in_policy.force_attach(false);
        bool set_in_policy(true);

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

        case DETACH_ADDR___granary_rcu_assign_pointer:
        case DETACH_ADDR___granary_rcu_dereference:
            break;

        /// Not an event that we care about; make sure to attach to all code
        /// running, including kernel code.
        default:
            set_in_policy = false;
            in.set_policy(curr_policy);
            break;
        }

        if(set_in_policy) {
            in.set_policy(in_policy);
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

        static_assert(16 <= detail::fragment_allocator_config::MIN_ALIGN,
            "Basic block alignment must be at least 16 bytes to enable "
            "easy patching of RCU-upgradable basic blocks.");

        // Don't add a patch point into recursive invocations that are used to
        // instrument memory loads for indirect control flow.
        if(ls.is_stub()) {
            return *this;
        }

        instruction first(ls.first());
        ASSERT(dynamorio::OP_LABEL == first.op_code());
        ASSERT(first.pc());

        // Upgrade patch point for write-side debugging. If this basic block
        // faults while running (i.e. because it writes to an RCU-protected
        // data structure) then it is either in an area where the writer is
        // updating the data structure, or doing something wrong.
        instruction after_patch(label_());
        instruction upgrade_jump(jmp_(pc_(first.pc())));

        ls.insert_before(first, mangled(jmp_short_(instr_(after_patch))));
        ls.insert_before(first, upgrade_jump);
        ls.insert_before(first, after_patch);

        instrumentation_policy upgrade_policy = policy_for<rcu_watched>();
        upgrade_policy.inherit_properties(*this, INHERIT_JMP);
        upgrade_jump.set_policy(upgrade_policy);

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


    /// Re-encode some of the instructions in the faulting basic block, up to
    /// the first CTI, which will either be a CTI into a stub or a CTI to a
    /// wrapped address.
    ///
    /// This is used as part of the basic block upgrade process. A rcu_null
    /// basic block is upgraded to an rcu_watched basic block, and then control
    /// is returned to this "watched tail" of the faulting basic block.
    ///
    /// TODO: Handle case of indirect CTIs through watched memory address.
    app_pc rcu_watched_tail(
        cpu_state_handle cpu,
        granary::basic_block_state &bb,
        app_pc cache_pc,
        instrumentation_policy policy
    ) throw() {
        instruction_list ls;
        for(;;) {
            const app_pc in_pc(cache_pc);
            instruction in(instruction::decode(&cache_pc));

            if(in.is_cti()) {
                ls.append(mangled(jmp_(pc_(in_pc))));
                break;
            }

            ls.append(in);
        }

        // Run the watchpoints instrumentation on the tail.
        policy.instrument(cpu, bb, ls);

        // Emit as gencode.
        const unsigned size(ls.encoded_size());
        app_pc tail_pc(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(size));
        ls.encode(tail_pc, size);

        return tail_pc;
    }


    /// Interrupted in `rcu_null`, upgrade to an RCU-writer protocol. We will
    /// hot-patch the original basic block to redirect control-flow to the
    /// upgraded basic block. We will also try to instrument the tail of the
    /// interrupted basic block.
    interrupt_handled_state rcu_null::handle_interrupt(
        granary::cpu_state_handle cpu,
        granary::thread_state_handle,
        granary::basic_block_state &bb,
        interrupt_stack_frame &isf,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_GENERAL_PROTECTION != vec) {
            return INTERRUPT_DEFER;
        }

        enum {
            OP_JMP_SHORT = 0xEB
        };

        basic_block faulting_bb(isf.instruction_pointer);

        // Figure out the location of the hot-patchable entry-point. If the
        // trace logger is being used then we need to shift by 8 bytes.
        // Double check that we find a short/near JMP instruction.
        app_pc patch_pc(faulting_bb.cache_pc_start);
#if CONFIG_DEBUG_TRACE_EXECUTION
        patch_pc += 5; // Size of a `CALL`.
#endif

        ASSERT(OP_JMP_SHORT == *patch_pc);

        app_pc instruction_to_patch(patch_pc);
        app_pc first_ins(nullptr);
        instruction jmp_short(instruction::decode(&instruction_to_patch));

        // Patch the near JMP at the beginning of the basic block to have a
        // zero relative displacement, so that execution falls through to the
        // JMP that brings execution to the next basic block, but under the
        // `rcu_watched` policy.
        ++patch_pc;
        if(0 != *patch_pc) {

            // Only compute the actual first instruction target when we know
            // that the instruction wasn't already patched, where the check
            // MUST happen *after* we decode the instruction.
            first_ins = jmp_short.cti_target().value.pc;

            std::atomic_thread_fence(std::memory_order_acquire);
            *patch_pc = 0;
            std::atomic_thread_fence(std::memory_order_release);
        }

        // Build the tail if we didn't fault at the first instruction.
        // Redirect the interrupt to resume where watchpoints are tested
        // for.
        if(first_ins == isf.instruction_pointer) {
            isf.instruction_pointer = faulting_bb.cache_pc_start;
        } else {
            instrumentation_policy policy = policy_for<rcu_watched>();
            policy.force_attach(true);
            policy.access_user_data(faulting_bb.policy.accesses_user_data());
            policy.in_host_context(faulting_bb.policy.is_in_host_context());
            policy.in_xmm_context(faulting_bb.policy.is_in_xmm_context());
            policy.begins_functional_unit(
                faulting_bb.policy.is_beginning_of_functional_unit());

            isf.instruction_pointer = rcu_watched_tail(
                cpu, bb, isf.instruction_pointer, policy);
        }

        return INTERRUPT_IRET;
    }


    /// "Null" RCU debugging policy. This instruments any code executing
    /// outside of a read-side critical section. This code upgrades to
    /// writer code on a fault.
    instrumentation_policy rcu_watched::visit_app_instructions(
        granary::cpu_state_handle cpu,
        granary::basic_block_state &bb,
        instruction_list &ls
    ) throw() {

        // Only apply the watchpoints instrumentation up until the first CTI.
        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {

            if(!in.is_cti()) {
                continue;
            }

            // Keep RET instructions in.
            if(in.is_return()) {
                continue;
            }

            operand target(in.cti_target());
            const app_pc fall_through_pc(next_pc(in));

            if(!dynamorio::opnd_is_pc(target)) {
                ASSERT(fall_through_pc);

                // CTI with a memory operand.
                if(dynamorio::REG_kind != target.kind) {
                    ls.remove_tail_at(in);
                    ls.append(jmp_(pc_(fall_through_pc)));
                    break;
                }

                // CTI with a register operand.
                continue;
            }

            switch(reinterpret_cast<uintptr_t>(target.value.pc)) {
            case DETACH_ADDR___granary_rcu_read_lock:
            case DETACH_ADDR___granary_rcu_read_unlock:
                ASSERT(fall_through_pc);
                ls.remove_tail_at(in);
                ls.append(jmp_(pc_(fall_through_pc)));
                goto done;
            default: break;
            }
        }
    done:

        // Run the watchpoints instrumentation.
        client::watchpoints<
            wp::rcu_read_policy, wp::rcu_read_policy
        >::visit_app_instructions(cpu, bb, ls);

        return policy_for<rcu_null>();
    }


    /// Host/app are the same.
    instrumentation_policy rcu_watched::visit_host_instructions(
        granary::cpu_state_handle cpu,
        granary::basic_block_state &bb,
        instruction_list &ls
    ) throw() {
        return visit_app_instructions(cpu, bb, ls);
    }


    interrupt_handled_state rcu_watched::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector
    ) throw() {
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

