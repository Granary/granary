#ifndef CLIENT_INSTR_TRACE_H_
#define CLIENT_INSTR_TRACE_H_

#include "granary/client.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::instr_trace_policy())
#endif

namespace client {
  struct instr_trace_policy : public granary::instrumentation_policy {
    public:

    enum {AUTO_INSTRUMENT_HOST = false};
    static granary::instrumentation_policy visit_app_instructions
    (
     granary::cpu_state_handle,
     granary::basic_block_state &,
     granary::instruction_list &
     ) throw();

    static granary::instrumentation_policy visit_host_instructions
    (
     granary::cpu_state_handle,
     granary::basic_block_state &,
     granary::instruction_list &
     ) throw();
								  
#if CONFIG_CLIENT_HANDLE_INTERRUPT
    static granary::interrupt_handled_state handle_interrupt
    (
     granary::cpu_state_handle,
     granary::thread_state_handle,
     granary::basic_block_state &,
     granary::interrupt_stack_frame &,
     granary::interrupt_vector
     ) throw();
#endif

  };

#if CONFIG_CLIENT_HANDLE_INTERRUPT
    granary::interrupt_handled_state handle_kernel_interrupt
    (
     granary::cpu_state_handle,
     granary::thread_state_handle,
     granary::interrupt_stack_frame &,
     granary::interrupt_vector
     ) throw();
#endif
}

#endif /* CLIENT_INSTR_TRACE_H_ */
