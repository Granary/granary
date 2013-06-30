/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * thread.cc
 *
 *  Created on: 2013-06-24
 *      Author: Peter Goodman, akshayk
 */

#include "granary/list.h"
#include "granary/types.h"
#include "clients/watchpoints/clients/leak_detector/descriptor.h"
#include "clients/watchpoints/clients/leak_detector/instrument.h"

using namespace granary;
using namespace granary::types;

extern "C" {
    struct task_struct *kernel_get_current(void);
    struct thread_info *kernel_current_thread_info(void);
}

#define GFP_ATOMIC 0x20U
#define PAGE_SIZE 4096
#define THREAD_SIZE 4*PAGE_SIZE

namespace client { namespace wp {

    struct leak_detector_thread_info {
        struct task_struct *tsk;
        void *stack;
        uint64_t stack_current_ptr;
        uint64_t stack_start;
        uint64_t thread_state;
    };

    static static_data<locked_hash_table<app_pc, app_pc>> object_scan_list;
    static static_data<locked_hash_table<struct thread_info*, leak_detector_thread_info*>> thread_scan_list;

    static static_data<list<uintptr_t>> watchpoint_list;


    STATIC_INITIALISE_ID(leak_detector, {
        object_scan_list.construct();
        thread_scan_list.construct();
        watchpoint_list.construct();
    })

#if 0
    template <typename T>
    void add_to_scanlist(T ptr_) throw() {
        scan_object_hash->store(
            reinterpret_cast<uintptr_t>(ptr_),
            unsafe_cast<app_pc>(&scan_function<T>::scan_impl::scan_head));
    }
#endif


    bool init_thread_private_slot(void){
        struct thread_info *info = kernel_current_thread_info();
        leak_detector_thread_info *my_thread_info = NULL;
        if(!thread_scan_list->find(info)){
            my_thread_info = (unsafe_cast<leak_detector_thread_info*>)
                    (granary::types::__kmalloc(sizeof(leak_detector_thread_info), GFP_ATOMIC));

            my_thread_info->stack = granary::types::__kmalloc(THREAD_SIZE, GFP_ATOMIC);
            my_thread_info->stack_current_ptr = 0x0ULL;
            my_thread_info->stack_start = 0x0ULL;
            thread_scan_list->store(info, my_thread_info);
        }
        return true;
    }

    void set_thread_private_info(void){
        struct thread_info *info = kernel_current_thread_info();
        leak_detector_thread_info *my_thread_info =
                unsafe_cast<leak_detector_thread_info*>((unsafe_cast<unsigned long>(info) + 2));
        my_thread_info->thread_state = 0x0ULL;
    }

    leak_detector_thread_info *get_thread_private_info(){
        struct thread_info *info = kernel_current_thread_info();
        leak_detector_thread_info *private_slot =
                unsafe_cast<leak_detector_thread_info*>((unsafe_cast<unsigned long>(info) + 2));

        return private_slot;
    }

    bool add_to_scanlist(granary::app_pc obj1, granary::app_pc obj2){
        object_scan_list->store(obj1, obj2);
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

    void handle_watched_address(uintptr_t addr){
        watchpoint_list->append(addr);
    }





    /// Notify the leak detector that this thread's execution has entered the
    /// code cache.
    void leak_notify_thread_enter_module(void) throw() {
        struct task_struct *task = kernel_get_current();
        init_thread_private_slot();
        UNUSED(task);

        //granary::printf("Entering the code cache.\n");
    }


    /// Notify the leak detector that this thread's execution is leaving the
    /// code cache.
    ///
    /// Note: Entry/exits can be nested in the case of the kernel calling the
    ///       module calling the kernel calling the module.
    void leak_notify_thread_exit_module(void) throw() {
       // granary::printf("Exiting the code cache.\n");
    }

    void htable_scan_object(
        granary::app_pc key,
        granary::app_pc value,
        hash_set<granary::app_pc>&
    ) throw() {
        typedef void (*scanner)(void *addr);
        if(is_active_watchpoint(value)){
            handle_watched_address(unsafe_cast<uintptr_t>(value));
         }
        scanner func = (scanner)value;
        if(nullptr != func){
            func(unsafe_cast<void*>(key));
        }
    }

    void htable_stack_scanner(
        struct thread_info*,
        struct leak_detector_thread_info *my_thread_info,
        hash_set<granary::app_pc>&
    ) throw() {
        bool ret;
        uint64_t start = (my_thread_info->stack_current_ptr);
        uint64_t stack_base = my_thread_info->stack_start + THREAD_SIZE;
        while( start < stack_base){
            uint64_t *stack_ptr = (unsafe_cast<uint64_t*>)(start);
            ret = is_active_watchpoint((unsafe_cast<void*>)(*stack_ptr));
            start += 8;
        }
        UNUSED(ret);
    }

    void leak_policy_scan_callback(void) throw(){
        granary::printf("%s\n", __FUNCTION__);

        // scan the kernel objects passed to the module
        hash_set<granary::app_pc> collect_list;
        object_scan_list->for_each_entry(htable_scan_object, collect_list);

        // scan the kernel stack and look for the watched address;
       // thread_scan_list->for_each_entry(htable_stack_scanner, collect_list);

    }

    void htable_copy_task(
        struct thread_info *info,
        struct leak_detector_thread_info *my_thread_info,
        hash_set<granary::app_pc>&
    ) throw() {
        my_thread_info->stack_current_ptr = info->task->thread.sp;
        my_thread_info->stack_start = (unsafe_cast<uint64_t>)(info->task->stack);
        my_thread_info->tsk = info->task;
        memcpy(my_thread_info->stack, info->task->stack, THREAD_SIZE);
    }

    void leak_policy_update_rootset(void) throw(){
        granary::printf("%s\n", __FUNCTION__);
        hash_set<granary::app_pc> collect_list;
        thread_scan_list->for_each_entry(htable_copy_task, collect_list);
    }

}}

