/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * events.cc
 *
 *  Created on: 2013-08-15
 *      Author: Peter Goodman
 */


#ifndef _RCU_STATE_H_
#define _RCU_STATE_H_

namespace client {

#   define CLIENT_cpu_state
    struct cpu_state {

    };

#   define CLIENT_thread_state
    struct thread_state {

        /// Read-side critical section id for the outermost critical section.
        /// This implies that rcudbg implicitly flattens RCU critical sections.
        uint16_t read_section_id;

        /// Source code location of the most recently executed RCU read lock.
        const char *read_lock_carat_backtrace[3];
        const char *active_carat;
    };

}

#endif /* _RCU_STATE_H_ */
