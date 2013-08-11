/*
 * instrument.cc
 *
 *  Created on: 2013-08-09
 *      Author: akshayk
 */



#include "clients/watchpoints/clients/rcu_debugger/instrument.h"
#include "clients/watchpoints/clients/rcu_debugger/descriptor.h"
#include "clients/watchpoints/utils.h"


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

    namespace wp {

        /// Register-specific (generated) functions to mark a leak descriptor
        /// as being accessed.
        typedef void (*descriptor_accessor_type)(void);
        static descriptor_accessor_type DESCRIPTOR_READ_ACCESSORS[] = {
            ALL_REGS(DESCRIPTOR_READ_ACCESSOR_PTRS, DESCRIPTOR_READ_ACCESSOR_PTR)
        };

            /// Verify the read policy; this will get invoked before every read operation
        void verify_read_policy(
            uintptr_t watched_addr,
            app_pc addr_in_bb
        ) throw() {
            bool flag = false;

            thread_state_handle thread = safe_cpu_access_zone();
            rcu_policy_thread_state *thread_state(thread->state);

            rcu_policy_descriptor *desc = client::wp::descriptor_of(watched_addr);
            rcu_policy_descriptor *next_desc;

            if(thread_state && thread_state->local_state.is_read_critical_section) {
                //printf("Accessing watched object : %llx, %llx\n", watched_addr, desc);
                next_desc = thread_state->desc_list;
     //           if(next_desc == desc){
       //             flag = true;
         //       }
#if 1
                while(next_desc != nullptr){
                    if(next_desc == desc){
                        flag = true;
                        break;
                    }
                    next_desc = next_desc->list_next;
                }
#endif
                if(desc->state.is_rcu_object && flag){
                    printf("Accessing rcu protected object : %llx\n", watched_addr);
                }
            }
            UNUSED(addr_in_bb);
        }



        /// Add instrumentation on every read and write that marks the
        /// shadow bits for the corresponding types .
        void rcu_policy::visit_read(
            granary::basic_block_state &,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            const unsigned reg_index(register_to_index(tracker.regs[i].value.reg));
            instruction call(insert_cti_after(ls, tracker.labels[i],
                unsafe_cast<app_pc>(DESCRIPTOR_READ_ACCESSORS[reg_index]),
                CTI_DONT_STEAL_REGISTER, operand(),
                CTI_CALL));
            call.set_mangled();
        }


        void rcu_policy::visit_write(
            granary::basic_block_state &,
            granary::instruction_list &,
            watchpoint_tracker &,
            unsigned
        ) throw() {

        }

#if CONFIG_CLIENT_HANDLE_INTERRUPT
        granary::interrupt_handled_state rcu_policy::handle_interrupt(
            granary::cpu_state_handle,
            granary::thread_state_handle,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw() {
            return granary::INTERRUPT_DEFER;
        }
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

    }
}

