/*
 * granary.c
 *
 *  Created on: 2012-11-08
 *      Author: pag
 *     Version: $Id$
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "granary/kernel/module.h"
#include "deps/icxxabi/icxxabi.h"


/// It's a trap!
MODULE_LICENSE("GPL");


/// Assembly function to run initialisers for globally-defined C++ data
/// structures.
extern void granary_run_initialisers(void);


/// Function that is called before granary faults.
void granary_break_on_fault(void) {
    __asm__ __volatile__ ("");
}


/// Function that is called in order to force a fault.
int granary_fault(void) {
    __asm__ __volatile__ ("mov 0, %rax;");
    return 1;
}


struct kernel_module *modules = NULL;
extern int (**kernel_printf)(const char *, ...);
extern void *(**kernel_vmalloc_exec)(unsigned long);
extern void *(**kernel_vmalloc)(unsigned long);
extern void (**kernel_vfree)(const void *);


/// C++-implemented function that operates on modules. This is the
/// bridge from C to C++.
extern void notify_module_state_change(struct kernel_module *);


/// Find the Granary-representation for an internal module.
static struct kernel_module *find_interal_module(void *vmod) {

    struct kernel_module *module = modules;
    struct kernel_module **next_link = &modules;
    const int is_granary = NULL == modules;
    struct module *mod = NULL;

    for(; NULL != module; module = module->next) {
        if(module->address == vmod) {
            return module;
        }
        next_link = &(module->next);
    }

    module = kmalloc(sizeof(struct kernel_module), GFP_KERNEL);
    mod = (struct module *) vmod;

    // Initialise.
    module->is_granary = is_granary;
    module->init = &(mod->init);
    module->exit = &(mod->exit);
    module->address = vmod;
    module->text_begin = mod->module_core;
    module->text_end = mod->module_core + mod->core_text_size;
    module->next = NULL;

    // Chain it in and return.
    *next_link = module;

    return module;
}


/// Notify Granary's back-end of a state change to a particular module.
static int module_load_notifier(
    struct notifier_block *nb,
    unsigned long mod_state,
    void *vmod
) {
    struct kernel_module *internal_mod = NULL;
    struct module *mod = (struct module *) vmod;
    printk("    Notified of module 0x%p\n", vmod);
    printk("    Module's name is: %s.\n", mod->name);
    internal_mod = find_interal_module(vmod);
    printk("    Got internal representation for module.\n");
    internal_mod->state = mod_state;
    printk("    Notifying Granary of the module...\n");
    notify_module_state_change(internal_mod);
    printk("    Notified Granary of the module.\n");
    return 0;
}


/// Callback structure used by Linux for module state change events.
static struct notifier_block notifier_block = {
    .notifier_call = module_load_notifier,
    .next = NULL,
    .priority = -1,
};


/// Allocate some executable memory
static void *allocate_executable(unsigned long size) {
    void *ret = __vmalloc(size, GFP_ATOMIC, PAGE_KERNEL_EXEC);
    if(!ret) {
        ret = __vmalloc(size, GFP_KERNEL, PAGE_KERNEL_EXEC);
    }
    return ret;
}


/// Allocate some executable memory
static void *allocate(unsigned long size) {
    void *ret = __vmalloc(size, GFP_ATOMIC, PAGE_KERNEL);
    if(!ret) {
        ret = __vmalloc(size, GFP_KERNEL, PAGE_KERNEL);
    }
    return ret;
}



/// Initialise Granary.
static int init_granary(void) {

    *kernel_printf = printk;
    *kernel_vmalloc_exec = allocate_executable;
    *kernel_vmalloc = allocate;
    *kernel_vfree = vfree;

    printk("Loading Granary...\n");

    printk("    Running initialisers...\n");
    granary_run_initialisers();
    printk("    Done running initialisers.\n");



    printk("    Registering module notifier...\n");

    register_module_notifier(&notifier_block);
    return 0;
}


/// Remove Granary.
static void exit_granary(void) {
    struct kernel_module *mod = modules;
    struct kernel_module *next_mod = NULL;

    printk("Unloading Granary... Goodbye!\n");
    unregister_module_notifier(&notifier_block);

    // free the memory associated with internal modules
    for(; NULL != mod; mod = next_mod) {
        next_mod = mod->next;
        kfree(mod);
    }

    // Invoke C++ global destructors.
    __cxa_finalize(0);
}

module_init(init_granary);
module_exit(exit_granary);
