#ifndef INSTR_TRACE_STATE_H_
#define INSTR_TRACE_STATE_H_

#define CLIENT_cpu_state

using namespace granary;

namespace client {
  struct trace_record {
    app_pc pc;
    unsigned opcode;
    void *addr;
  };
  struct trace_stats {
    unsigned long num_loads;
    unsigned long num_stores;
    unsigned long num_bbs;
    unsigned long num_instrs;
  };

  struct cpu_state {
    int id;
    trace_stats stats;
    int log_idx;
    trace_record *log;
  };
}

#endif /* INSTR_TRACE_STATE_H_ */
