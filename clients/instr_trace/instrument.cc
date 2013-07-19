#include "clients/instr_trace/instrument.h"

extern "C" {
#   include "granary/kernel/linux/module.h"
  extern void kernel_run_on_each_cpu(void (*func)(void *), void *thunk);
}

using namespace granary;

namespace client {

  #define NUM_TRACE_RECORDS 1024
  static app_pc EVENT_TRACE_INSTR = nullptr;

  __attribute__((hot)) void trace_instruction (app_pc pc, unsigned opcode, uint64_t opnd1, uint64_t opnd2) throw() {
    cpu_state_handle cpu;
    trace_record *log = &cpu->log[cpu->log_idx % NUM_TRACE_RECORDS];

    log->opcode = opcode;
    log->pc = pc; 
    log->opnd1 = opnd1;
    log->opnd2 = opnd2;

    cpu->stats.num_instrs++;
    cpu->log_idx++;
  }

  static void init_cpu_state (int *idx) throw() {
    cpu_state_handle cpu;
    cpu->id = (*idx)++;
    cpu->log = allocate_memory<trace_record>(NUM_TRACE_RECORDS);
    ASSERT(cpu->log);
    printf("[instr_trace] Initialized cpu %d\n", cpu->id);
  }
  
  void init(void) throw() {
    EVENT_TRACE_INSTR = generate_clean_callable_address(&trace_instruction);
    int num_cpus = 0;
    kernel_run_on_each_cpu(unsafe_cast<void (*) (void *)>(init_cpu_state), 
			   reinterpret_cast<void *>(&num_cpus));
    printf("[instr_trace] Finished initializing %d cpus \n", num_cpus);
  }

  static void load_operand (const operand_ref &, instruction_list &, instruction &) throw() {
  }

  granary::instrumentation_policy instr_trace_policy::visit_app_instructions
  (
   granary::cpu_state_handle,
   granary::basic_block_state &,
   granary::instruction_list &ls
   ) throw() {
    using namespace granary;
    
    for(instruction in(ls.first()); in.is_valid(); in = in.next()) {
      instruction x(ls.insert_before(in, label_()));
      x = ls.insert_after(x, push_(reg::arg3));
      x = ls.insert_after(x, push_(reg::arg4));
      in.for_each_operand(load_operand, ls, x);
      x = insert_clean_call_after(ls, x, EVENT_TRACE_INSTR, in.pc(), static_cast<uint64_t>(in.op_code()));
      x = ls.insert_after(x, pop_(reg::arg4));
      x = ls.insert_after(x, pop_(reg::arg3));
    }
    
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
