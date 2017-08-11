/* Copyright 2017 Peter Goodman (peter@trailofbits.com), all rights reserved. */

#include "clients/tracer/instrument.h"

using namespace granary;

namespace client {

// Versions of the above functions that are safe to call from within
// instrumented code.
app_pc print_pc = nullptr;
app_pc on_write_func = nullptr;

static void do_print_pc(uintptr_t addr) {
  granary::printf("0x%lx\n", addr);
}

// Initialize this client / tool. This creates the safe/clean-callable versions
// of the `on_read` and `on_write` callback functions.
void init(void) {
  cpu_state_handle cpu;
  cpu.free_transient_allocators();
  print_pc = generate_clean_callable_address(do_print_pc);
  cpu.free_transient_allocators();
}

/// Instruction a basic block.
granary::instrumentation_policy tracer_policy::visit_app_instructions(
    granary::cpu_state_handle, granary::basic_block_state &,
    granary::instruction_list &ls) {
  for (instruction in(ls.first()); in.is_valid(); in = in.next()) {
    insert_clean_call_after(ls, in, print_pc, in.pc());
  }
  return *this;
}

/// Instruction a basic block.
granary::instrumentation_policy tracer_policy::visit_host_instructions(
    granary::cpu_state_handle cpu, granary::basic_block_state &bb,
    granary::instruction_list &ls) {
  return visit_app_instructions(cpu, bb, ls);
}

}  // namespace client

