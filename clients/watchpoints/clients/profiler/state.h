/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *  Created on: 2013-06-24
 *      Author: Peter Goodman
 */

#ifndef LEAK_DETECTOR_STATE_H_
#define LEAK_DETECTOR_STATE_H_

namespace client {

    namespace wp {
        enum thread_private_state{
            NONE = 0x0ULL,
            MODULE_RUNNING,
            MODULE_EXIT,
        };

        struct profiler_descriptor;
        struct profiler_thread_state {
            uint64_t local_state;
        };
    }


#   define CLIENT_cpu_state
    struct cpu_state {

        /// List of free bounds checking objects for this CPU.
        client::wp::profiler_descriptor *free_list;
    };

#   define CLIENT_thread_state
    /// Extensions to Granary's internal thread-local storage.
    struct thread_state {

        client::wp::profiler_thread_state *state;

    };

}


#endif /* LEAK_DETECTOR_STATE_H_ */
