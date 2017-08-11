/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * report.cc
 *
 *  Created on: 2013-10-21
 *      Author: Peter Goodman
 */

#include "clients/watchpoints/clients/user/instrument.h"


extern "C" {
    extern int sprintf(char *buf, const char *fmt, ...);
}


using namespace granary;

namespace client {

    /// Head of the basic blocks linked list.
    extern std::atomic<basic_block_state *> BASIC_BLOCKS;


    enum {
        BUFF_SIZE = 1024,
        BUFF_FLUSH = 1000
    };


    /// Output buffer.
    static char BUFF[BUFF_SIZE] = {'\0'};


    /// Report on watchpoints statistics.
    void report(void) {
        basic_block_state *bb(BASIC_BLOCKS.load());

        int n(0);

        for(; bb; bb = bb->next) {
            if(!bb->accesses_user_data) {
                continue;
            }
            n += sprintf(&(BUFF[n]), "%p\n", bb->instruction_address);
            if(n >= BUFF_FLUSH) {
                granary::log(BUFF, n);
                n = 0;
            }
        }

        if(0 < n) {
            granary::log(BUFF, n);
        }
    }

}

