/*
 * thread.cc
 *
 *  Created on: 2013-08-11
 *      Author: akshayk
 */


#include "granary/list.h"
#include "granary/types.h"
#include "clients/watchpoints/clients/rcu_debugger/instrument.h"
#include "clients/watchpoints/clients/rcu_debugger/descriptor.h"

using namespace granary;

namespace client { namespace wp {

        void rcu_read_lock_callback(enum read_critical_type type){
            thread_state_handle thread = safe_cpu_access_zone();
            rcu_policy_thread_state *thread_state(thread->state);

            if(!thread_state){
                thread_state = unsafe_cast<rcu_policy_thread_state*>(
                        types::__kmalloc(sizeof(rcu_policy_thread_state), 0x20));
            }

            thread_state->local_state.is_read_critical_section = true;
            thread_state->local_state.recurive_read_critcal_count++;
            thread->state = thread_state;

            USED(type);
        }

        void rcu_read_unlock_callback(enum read_critical_type type){
            thread_state_handle thread = safe_cpu_access_zone();
            rcu_policy_thread_state *thread_state(thread->state);

            if(thread_state){
                thread_state->local_state.recurive_read_critcal_count--;

                if(!thread_state->local_state.recurive_read_critcal_count){
                    thread_state->local_state.is_read_critical_section = false;
                }

                thread->state = thread_state;
            }
            USED(type);
        }

}}


