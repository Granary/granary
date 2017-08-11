/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * report.cc
 *
 *  Created on: 2013-10-22
 *      Author: Peter Goodman
 */

#include "clients/instr_dist/instrument.h"

extern "C" {
    extern int sprintf(char *buf, const char *fmt, ...);
}

using namespace granary;

namespace client {


    instruction_log INSTRUCTION_DIST[dynamorio::OP_AFTER_LAST] = {
        {ATOMIC_VAR_INIT(0),{nullptr}}
    };


    enum {
       BUFF_SIZE = 1500,
       BUFF_FLUSH = 1000
   };


   /// Output buffer.
   static char BUFF[BUFF_SIZE] = {'\0'};


   /// Report on watchpoints statistics.
   void report(void) {
       int n(0);

       for(unsigned i(0); i < dynamorio::OP_AFTER_LAST; ++i) {
           const instruction_log &log(INSTRUCTION_DIST[i]);

           n += sprintf(&(BUFF[n]), "%u\t%u", i, log.num_instances.load());
           for(unsigned j(0); j < instruction_log::NUM_RECORDED_INSTANCES; ++j) {
               if(log.instances[j]) {
                   n += sprintf(&(BUFF[n]), "\t%p", log.instances[j]);
               }
           }
           n += sprintf(&(BUFF[n]), "\n");

           if(BUFF_FLUSH <= n) {
               granary::log(BUFF, n);
               n = 0;
           }
       }

       if(0 < n) {
           granary::log(BUFF, n);
       }
   }
}


