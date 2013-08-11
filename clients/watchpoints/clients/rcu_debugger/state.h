/*
 * state.h
 *
 *  Created on: 2013-08-09
 *      Author: akshayk
 */

#ifndef _RCU_STATE_H_
#define _RCU_STATE_H_

namespace client {

    namespace wp {

        enum read_critical_type {
            READ_CRITICAL = 0x0,
            READ_CRITICAL_BH,
            READ_CRITICAL_SCHED
        };

    /* Update thread private state based on the requirements*/
        struct thread_private_state{

            /*set when read critical section starts*/
            bool is_read_critical_section;

            enum read_critical_type type;

            /*count the number of recursive read critical section*/
            uint16_t recurive_read_critcal_count;
        };

        struct rcu_policy_descriptor;
        struct rcu_policy_thread_state {
            thread_private_state local_state;
            rcu_policy_descriptor *desc_list;
        };
    }


#   define CLIENT_cpu_state
    struct cpu_state {

        /// List of free bounds checking objects for this CPU.
        client::wp::rcu_policy_descriptor *free_list;
    };

#   define CLIENT_thread_state
    /// Extensions to Granary's internal thread-local storage.
    struct thread_state {

        client::wp::rcu_policy_thread_state *state;

    };

}

#endif /* _RCU_STATE_H_ */
