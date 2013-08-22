/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.cc
 *
 *  Created on: 2013-08-15
 *      Author: Peter Goodman
 */


#ifndef RCUDBG_STATE_H_
#define RCUDBG_STATE_H_

namespace client {

#   define CLIENT_cpu_state
    struct cpu_state {

    };

#   define CLIENT_thread_state
    struct thread_state {

        /// Current depth of read-side critical sections.
        unsigned section_depth;

        /// Read-side critical section id for the outermost critical section.
        /// This implies that rcudbg implicitly flattens RCU critical sections.
        unsigned section_id;

        /// Source code location of the most recently executed RCU read lock.
        const char *section_carat_backtrace[3];


        /// Source code of the most recently entered read-side critical section.
        const char *section_carat;
    };

}

#endif /* RCUDBG_STATE_H_ */
