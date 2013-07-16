#include "granary/client.h"

namespace client {
  extern unsigned long num_bbs;
  extern unsigned long num_loads;
  extern unsigned long num_stores;
  
  void report (void) throw() {
    granary::printf("[instr_trace] %lu bbs %lu loads %lu stores \n", 
		    num_bbs, num_loads, num_stores);
  }
}
