/*
 * thread.cc
 *
 *  Created on: 2013-08-05
 *      Author: akshayk
 */


#include "granary/list.h"
#include "granary/types.h"
#include "clients/watchpoints/clients/shadow_memory/descriptor.h"
#include "clients/watchpoints/clients/shadow_memory/instrument.h"

using namespace granary;
using namespace granary::types;

#define GFP_ATOMIC 0x20U
#define PAGE_SIZE 4096
#define THREAD_SIZE 4*PAGE_SIZE

namespace client { namespace wp {

    static static_data<locked_hash_table<app_pc, app_pc>> object_scan_list;
    static static_data<hash_set<uintptr_t>> watchpoint_collect_list;

    static static_data<list<uintptr_t>> watchpoint_list;
    typedef list_item_handle<uintptr_t> handle_type;


    STATIC_INITIALISE_ID(shadow_policy, {
        object_scan_list.construct();
        watchpoint_list.construct();
        watchpoint_collect_list.construct();
    })


    bool init_thread_private_slot(void){
        return true;
    }

    void set_thread_private_info(shadow_policy_thread_state *state){
        thread_state_handle thread = safe_cpu_access_zone();
        shadow_policy_thread_state *thread_state(thread->state);
        thread_state->local_state = state->local_state;
    }

    shadow_policy_thread_state *get_thread_private_info(void){
        thread_state_handle thread = safe_cpu_access_zone();
        shadow_policy_thread_state *thread_state(thread->state);
        return thread_state;
    }

    bool add_to_scanlist(granary::app_pc, granary::app_pc){
        return true;
    }

    bool remove_from_scanlist(granary::app_pc, granary::app_pc){
        return true;
    }

    bool is_active_watchpoint(void *addr){
        if(is_watched_address(addr)){
            descriptor_type<void*>::type *desc = descriptor_of(addr);
            if(desc != nullptr) {
                granary::printf("descriptor : %llx\n", desc);
                if(desc->state.is_active){
                    return true;
                }
            }
        }
        return false;
    }


    /// Notify the leak detector that this thread's execution has entered the
    /// code cache.
    void notify_thread_enter_module(void) throw() {
        thread_state_handle thread = safe_cpu_access_zone();
        shadow_policy_thread_state *thread_state(thread->state);

        if(!thread_state){
            granary::printf("ENTER::thread state is not initialised\n");
            thread_state = unsafe_cast<shadow_policy_thread_state*>(__kmalloc(sizeof(shadow_policy_thread_state), GFP_ATOMIC));
            thread_state->local_state = MODULE_RUNNING;
            thread->state = thread_state;
        }

        //granary::printf("Entering the code cache.\n");
    }


    /// Notify the leak detector that this thread's execution is leaving the
    /// code cache.
    ///
    /// Note: Entry/exits can be nested in the case of the kernel calling the
    ///       module calling the kernel calling the module.
    void notify_thread_exit_module(void) throw() {
        thread_state_handle thread = safe_cpu_access_zone();

        shadow_policy_thread_state *thread_state(thread->state);

        if(thread_state){
            thread_state->local_state = MODULE_EXIT;
        }else {
            granary::printf("EXIT::thread state is not initialised\n");
        }
        //granary::printf("Exiting the code cache.\n");
    }

}}



