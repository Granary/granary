/*
 * instrument.h
 *
 *  Created on: 2013-04-20
 *      Author: pag
 */

#ifndef WATCHPOINT_INSTRUMENT_H_
#define WATCHPOINT_INSTRUMENT_H_

#include "clients/instrument.h"


/// Enable if %RBP should be treated as a frame pointer and not as a potential
/// watched address.
#define IF_WP_IGNORE_FRAME_POINTER(...) __VA_ARGS__


namespace client {

    namespace wp {

        /// Find memory operands that might need to be checked for watchpoints.
        /// If one is found, then num_ops is incremented, and the operand
        /// reference is stored in the passed array.
        void find_memory_operand(
            const granary::operand_ref &op,
            granary::operand_ref *&ops,
            unsigned &num_ops
        ) throw();

    }


    template <typename Watcher>
    struct watchpoints : public granary::instrumentation_policy {
    public:

        /// Instrument a basic block.
        static granary::instrumentation_policy visit_basic_block(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            using namespace granary;

            instruction prev_in;
            register_manager live_regs;
            operand_ref memory_ops[3];

            for(instruction in(ls.last()); in.is_valid(); in = prev_in) {
                prev_in = in.prev();


                //Watcher::visit_instruction(cpu, thread, bb, ls, in);

            end:
                // compute live regs for next iteration based on this
                // instruction.
                live_regs.visit(in);
            }



            return policy_for<watchpoints<Watcher>>();
        }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
        /// Handle an interrupt in module code. Returns true iff the client
        /// handles the interrupt.
        static granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::interrupt_stack_frame &isf,
            granary::interrupt_vector vector
        ) throw();
#endif

    };

}


#endif /* WATCHPOINT_INSTRUMENT_H_ */
