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

                thread_state->desc_list = nullptr;
            }

            thread_state->local_state.is_read_critical_section = true;
            thread_state->local_state.recurive_read_critcal_count++;
            thread->state = thread_state;

            USED(type);
        }

        void rcu_read_unlock_callback(enum read_critical_type type){
            thread_state_handle thread = safe_cpu_access_zone();
            rcu_policy_thread_state *thread_state(thread->state);
            rcu_policy_descriptor *desc;
            rcu_policy_descriptor *next_desc;

            if(thread_state){
                thread_state->local_state.recurive_read_critcal_count--;

                if(!thread_state->local_state.recurive_read_critcal_count){
                    thread_state->local_state.is_read_critical_section = false;
                    desc = thread_state->desc_list;
                    thread_state->desc_list = nullptr;
                    while(desc->list_next){
                        next_desc = desc->list_next;
                        desc->list_next = nullptr;
                        desc = next_desc;
                    }
                }

                thread->state = thread_state;
            }
            USED(type);
        }

        void rcu_dereference_callback(void *addr){
            thread_state_handle thread = safe_cpu_access_zone();
            rcu_policy_thread_state *thread_state(thread->state);

            if(thread_state){
                rcu_policy_descriptor *desc = descriptor_of(addr);
                rcu_policy_descriptor *desc_list = thread_state->desc_list;
                if(desc_list) {
                    while(desc_list->list_next != nullptr){
                        if(desc_list == desc){
                            thread->state = thread_state;
                            return;
                        }

                        desc_list = desc_list->list_next;
                    }
                    if(desc_list != desc) {
                        desc->list_next = nullptr;
                        desc_list->list_next = desc;
                    }
                }else{
                    desc->list_next = nullptr;
                    thread_state->desc_list = desc;
                }
                thread->state = thread_state;
            }
        }

}}


