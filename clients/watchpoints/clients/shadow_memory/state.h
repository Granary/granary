/*
 * state.h
 *
 *  Created on: 2013-08-05
 *      Author: akshayk
 */

#ifndef _SHADOW_POLICY_STATE_H_
#define _SHADOW_POLICY_STATE_H_

namespace client {

    namespace wp {

        enum thread_private_state{
            NONE = 0x0ULL,
            MODULE_RUNNING,
            MODULE_EXIT,
        };

        struct shadow_policy_descriptor;
        struct shadow_policy_thread_state {
            uint64_t local_state;
        };
    }


#   define CLIENT_cpu_state
    struct cpu_state {

        /// List of free bounds checking objects for this CPU.
        client::wp::shadow_policy_descriptor *free_list;
    };

#   define CLIENT_thread_state
    /// Extensions to Granary's internal thread-local storage.
    struct thread_state {

        client::wp::shadow_policy_thread_state *state;

    };

}




#endif /* STATE_H_ */
