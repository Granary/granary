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
        struct leak_detector_descriptor;
    }


#   define CLIENT_cpu_state
    struct cpu_state {

        /// List of free bounds checking objects for this CPU.
        wp::leak_detector_descriptor *free_list;
    };

}

#endif /* LEAK_DETECTOR_STATE_H_ */
