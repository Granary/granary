#include "clients/instr_trace/instrument.h"

using namespace granary;

namespace client {

  static app_pc EVENT_TRACE_BB = nullptr;
  static app_pc EVENT_TRACE_LOAD = nullptr;
  static app_pc EVENT_TRACE_STORE = nullptr;

  unsigned long num_bbs = 0;
  unsigned long num_loads = 0;
  unsigned long num_stores = 0;

  __attribute__((hot)) void trace_bb (basic_block_state *) throw() {
      num_bbs++;
  }

  __attribute__((hot)) void trace_load (basic_block_state *) throw() {
    num_loads++;
  }

  __attribute__((hot)) void trace_store (basic_block_state *) throw() {
    num_stores++;
  }

  void init(void) throw() {
    EVENT_TRACE_BB = generate_clean_callable_address(&trace_bb);
    EVENT_TRACE_LOAD = generate_clean_callable_address(&trace_load);
    EVENT_TRACE_STORE = generate_clean_callable_address(&trace_store);
  }

  granary::instrumentation_policy instr_trace_policy::visit_app_instructions
  (
   granary::cpu_state_handle,
   granary::basic_block_state &bb,
   granary::instruction_list &ls
   ) throw() {
    using namespace granary;
    
    for(instruction in(ls.first()); in.is_valid(); in = in.next()) {
      if (dynamorio::OP_mov_ld == in.op_code()) {
	instruction x(ls.insert_before(in, label_()));
	insert_clean_call_after(ls, x, EVENT_TRACE_LOAD, &bb);
      }
      if (dynamorio::OP_mov_st == in.op_code()) {
	instruction x(ls.insert_before(in, label_()));
	insert_clean_call_after(ls, x, EVENT_TRACE_STORE, &bb);
      }
    }

    instruction in(ls.insert_before (ls.first(), label_()));
    in = insert_clean_call_after(ls, in, EVENT_TRACE_BB, &bb);
    
    return granary::policy_for<instr_trace_policy>();
  }

  granary::instrumentation_policy instr_trace_policy::visit_host_instructions
  (
   granary::cpu_state_handle,
   granary::basic_block_state &,
   granary::instruction_list &
   ) throw() {
    return granary::policy_for<instr_trace_policy>();
  }

  #if CONFIG_CLIENT_HANDLE_INTERRUPT

    /// Handle an interrupt in module code. Returns true iff the client
    /// handles the interrupt.
    granary::interrupt_handled_state instr_trace_policy::handle_interrupt(
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
