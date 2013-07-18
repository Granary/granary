#include "granary/client.h"

extern "C" {
#   include "granary/kernel/linux/module.h"
  extern void kernel_run_on_each_cpu(void (*func)(void *), void *thunk);
}

using namespace granary;
namespace client {
  
  static void print_cpu_stats (void *) throw() {
    cpu_state_handle cpu;
    printf("[instr_trace] CPU%d %lu instrs\n",  cpu->id, cpu->stats.num_instrs);
  }

  void report (void) throw() {
    kernel_run_on_each_cpu(print_cpu_stats, nullptr);
  }
}
