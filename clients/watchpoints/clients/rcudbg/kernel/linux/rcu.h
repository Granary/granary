/*
 * rcu.h
 *
 *  Created on: 2013-08-13
 *      Author: pgoodman
 */

#ifndef _RCU_API_H_
#define _RCU_API_H_

#include "clients/watchpoints/instrument.h"


namespace client {

    namespace wp {
        extern void rcu_read_lock_callback(enum read_critical_type type);

        extern void rcu_read_unlock_callback(enum read_critical_type type);

        extern void rcu_dereference_callback(void *addr);
    }
}

#if defined(CAN_WRAP_rcu_watch_assign_pointer) && CAN_WRAP_rcu_watch_assign_pointer
#   define APP_WRAPPER_FOR_rcu_watch_assign_pointer
    FUNCTION_WRAPPER(APP, rcu_watch_assign_pointer, (void*), (void **p, void *v, const char* s_ptr), {
        granary::printf("rcu_watch_assign_pointer\n");
        if(is_watched_address(v)){
            client::wp::rcu_policy_descriptor *desc = descriptor_of(v);
            client::wp::rcu_policy_state state;
            state.is_rcu_object = 0x1;
            desc->state.is_rcu_object = 0x1;
            desc->state.set_state(state);
            granary::printf("%llx : %llx\n", v, desc);
        }
        return rcu_watch_assign_pointer(p, v, s_ptr);
    })
#endif

#if defined(CAN_WRAP_rcu_watch_dereference) && CAN_WRAP_rcu_watch_dereference
#   define APP_WRAPPER_FOR_rcu_watch_dereference
    FUNCTION_WRAPPER(APP, rcu_watch_dereference, (void*), (void *addr, const char* s_ptr), {
        //granary::printf("rcu_watch_dereference\n");
        client::wp::rcu_policy_descriptor *desc;
        client::wp::rcu_policy_descriptor *new_desc;
        void* deref_addr;
        if(is_watched_address(addr)){
            rcu_dereference_callback(addr);
#if 0
            deref_addr = unwatched_address(addr);
            desc = descriptor_of(addr);
            add_watchpoint(deref_addr, deref_addr, desc->size);
            new_desc = descriptor_of(deref_addr);
            if(new_desc && desc) {
                memcpy(new_desc, desc, sizeof(client::wp::rcu_policy_descriptor));
                new_desc->state.is_rcu_dereference = 1;
                return rcu_watch_dereference((void*)deref_addr, s_ptr);
            }
#endif
        }
        return rcu_watch_dereference(addr, s_ptr);
    })
#endif

#if defined(CAN_WRAP_rcu_watch_read_lock) && CAN_WRAP_rcu_watch_read_lock
#   define APP_WRAPPER_FOR_rcu_watch_read_lock
    FUNCTION_WRAPPER_VOID(APP, rcu_watch_read_lock, (const char* s_ptr), {
         client::wp::rcu_read_lock_callback(client::wp::READ_CRITICAL);
         rcu_watch_read_lock(s_ptr);
        //granary::printf("rcu_watch_read_lock\n");
    })
#endif

#if defined(CAN_WRAP_rcu_watch_read_unlock) && CAN_WRAP_rcu_watch_read_unlock
#   define APP_WRAPPER_FOR_rcu_watch_read_unlock
    FUNCTION_WRAPPER_VOID(APP, rcu_watch_read_unlock, (const char* s_ptr), {
        rcu_watch_read_unlock(s_ptr);
        //granary::printf("rcu_watch_read_unlock\n");
    })
#endif

#if defined(CAN_WRAP_rcu_watch_read_lock_bh) && CAN_WRAP_rcu_watch_read_lock_bh
#   define APP_WRAPPER_FOR_rcu_watch_read_lock_bh
    FUNCTION_WRAPPER_VOID(APP, rcu_watch_read_lock_bh, (const char* s_ptr), {
        rcu_watch_read_lock_bh(s_ptr);
        //granary::printf("rcu_watch_read_lock_bh\n");
    })
#endif

#if defined(CAN_WRAP_rcu_watch_read_unlock_bh) && CAN_WRAP_rcu_watch_read_unlock_bh
#   define APP_WRAPPER_FOR_rcu_watch_read_unlock_bh
    FUNCTION_WRAPPER_VOID(APP, rcu_watch_read_unlock_bh, (const char* s_ptr), {
        rcu_watch_read_unlock_bh(s_ptr);
        //granary::printf("rcu_watch_read_unlock_bh\n");
    })
#endif

#if defined(CAN_WRAP_rcu_watch_read_lock_sched) && CAN_WRAP_rcu_watch_read_lock_sched
#   define APP_WRAPPER_FOR_rcu_watch_read_lock_sched
    FUNCTION_WRAPPER_VOID(APP, rcu_watch_read_lock_sched, (const char* s_ptr), {
        rcu_watch_read_lock_sched(s_ptr);
        //granary::printf("rcu_watch_read_lock_sched\n");
    })
#endif

#if defined(CAN_WRAP_rcu_watch_read_unlock_sched) && CAN_WRAP_rcu_watch_read_unlock_sched
#   define APP_WRAPPER_FOR_rcu_watch_read_unlock_sched
    FUNCTION_WRAPPER_VOID(APP, rcu_watch_read_unlock_sched, (const char* s_ptr), {
        rcu_watch_read_unlock_sched(s_ptr);
        //granary::printf("rcu_watch_read_unlock_sched\n");
    })
#endif


#endif /* RCU_H_ */
