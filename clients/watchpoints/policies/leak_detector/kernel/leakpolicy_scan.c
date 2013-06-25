/*
 * leakpolicy_scan.c
 *
 *  Created on: 2013-06-24
 *      Author: akshayk
 */


#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/stop_machine.h>

#define D(x)

typedef unsigned char* app_pc;

typedef void (scanner_callback)(void);

scanner_callback *scan_thread_callback = NULL;
scanner_callback *update_rootset_callback = NULL;

int
stop_machine_callback(void *arg){
    D(printk("%s\n", __FUNCTION__))
    if(update_rootset_callback){
        update_rootset_callback();
    }

    return 0;
}


int
leakpolicy_scan_thread(void *arg){

    while(!kthread_should_stop()){

        stop_machine(stop_machine_callback, NULL, NULL);

        preempt_disable();
      //  raw_local_irq_save(flags);
        D(printk("inside scanning thread\n");)

        if(scan_thread_callback)
            scan_thread_callback();

       // raw_local_irq_restore(flags);
        preempt_enable();
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(10*HZ);
    }

    set_current_state(TASK_RUNNING);
    return 0;
}


void
leak_policy_scanner_init(const app_pc thread_callback, const app_pc rootset_callback){

    scan_thread_callback = (scanner_callback*)thread_callback;
    update_rootset_callback = (scanner_callback*)rootset_callback;

    D(printk("inside function leak_policy_scanner_init\n");)

    {
        /* creates a new thread for scanning the rootsets
         * and registers the callback functions for that */
        struct task_struct *scan_task = kthread_create(leakpolicy_scan_thread,
                NULL, "leakpolicy_scanner");

        /* if the scan thread is created ; starts running the thread
         * by calling wake_up_process */
        if (!IS_ERR(scan_task))
            wake_up_process(scan_task);
        else
            WARN_ON(1);
    }
}

