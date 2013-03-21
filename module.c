/*
 * granary.c
 *
 *  Created on: 2012-11-08
 *      Author: pag
 *     Version: $Id$
 */


#ifndef CONFIG_MODULES
#   define CONFIG_MODULES 1
#endif


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/pfn.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/percpu.h>
#include <linux/percpu-defs.h>
#include <linux/preempt.h>

#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/thread_info.h>

#include "granary/kernel/module.h"

#   define WRAP_FOR_DETACH(func)
#   define DETACH(func)
#   define TYPED_DETACH(func)
#include "granary/gen/detach.inc"

#ifndef DETACH_ADDR_module_alloc_update_bounds
#   error "Unable to compile; need to be able to allocate module memory."
#endif

#ifndef SUCCESS
#   define SUCCESS 0
#endif


#define MAJOR_NUM
#define IOCTL_RUN_COMMAND _IOR(MAJOR_NUM, 0, char *)


/// It's a trap!
MODULE_LICENSE("GPL");


/// Get access to per-CPU Granary state.
void *get_percpu_state(void *ptr) {
    return this_cpu_ptr(ptr);
}


/// Assembly function to run constructors for globally-defined C++ data
/// structures.
extern void granary_run_initialisers(void);


/// C function defined in granary/kernel/module.cc for initialising Granary.
/// This function is invoked when an ioctl is used to tell Granary to
/// initialise.
extern void granary_initialise(void);


/// Function that is called before granary faults.
void granary_break_on_fault(void) {
    __asm__ __volatile__ ("");
}


/// Function that is called in order to force a fault.
int granary_fault(void) {
    __asm__ __volatile__ ("mov 0, %rax;");
    return 1;
}


/// Notify the kernel that pre-emption is disabled. Granary does this even
/// when interrupts are disabled.
void kernel_preempt_disable(void) {
    preempt_disable();
}


/// Notify the kernel that pre-emption is re-enabled. Granary does this even
/// when interrupts remain disabled, with the understanding that Granary won't
/// do anything "interesting" (i.e. call kernel functions that might inspect
/// the pre-emption state) until it re-enabled interrupts.
void kernel_preempt_enable(void) {
    preempt_enable();
}


struct kernel_module *modules = NULL;
extern void *(**kernel_malloc_exec)(unsigned long, int);
extern void *(**kernel_malloc)(unsigned long);
extern void (**kernel_free)(const void *);


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


/// Allocate a "fake" module that will serve as a lasting memory zone for
/// executable allocations.
static unsigned long EXEC_START = 0;
static unsigned long EXEC_END = 0;

static unsigned long CODE_CACHE_END = 0;
static unsigned long GEN_CODE_START = 0;
static unsigned long WRAPPER_START = 0;
static unsigned long WRAPPER_END = 0;

static void preallocate_executable(void) {

    typedef void *(module_alloc_t)(unsigned long);

    enum {
        _250_MB = 262144000,
        _1_P = 4096,
        _1_MB = 1048576
    };

    /// What is used internally by the kernel to allocate modules :D
    module_alloc_t *module_alloc_update_bounds  =
        (module_alloc_t *) DETACH_ADDR_module_alloc_update_bounds;

    void *mem = module_alloc_update_bounds(_250_MB);
    uint64_t begin_pfn = 0;
    uint64_t end_pfn = 0;

    if(!mem) {
        granary_fault();
    }

    EXEC_START = (unsigned long) mem;
    EXEC_END = EXEC_START + _250_MB;


    begin_pfn = PFN_DOWN(EXEC_START);
    end_pfn = PFN_DOWN(EXEC_END);

    if(end_pfn > begin_pfn) {
        set_memory_x(begin_pfn << PAGE_SHIFT, end_pfn - begin_pfn);

    } else if(end_pfn == begin_pfn) {
        set_memory_x(begin_pfn << PAGE_SHIFT, 1);
    }

    CODE_CACHE_END = EXEC_START;
    WRAPPER_START = EXEC_END - _1_MB;
    WRAPPER_END = WRAPPER_START;
    GEN_CODE_START = WRAPPER_START;
}

enum executable_memory_kind {
    EXEC_CODE_CACHE = 0,
    EXEC_GEN_CODE = 1,
    EXEC_WRAPPER = 2
};

/// Allocate some executable memory
static void *allocate_executable(unsigned long size, int where) {
    unsigned long mem = 0;
    switch(where) {

    // code cache pages are allocated from the beginning
    case EXEC_CODE_CACHE:
        mem = __sync_fetch_and_add(&CODE_CACHE_END, size);
        if((mem + size) > GEN_CODE_START) {
            granary_fault();
        }
        break;

    // gencode pages are allocated from near the end
    case EXEC_GEN_CODE:
        mem = __sync_sub_and_fetch(&GEN_CODE_START, size);
        if(mem < CODE_CACHE_END) {
            granary_fault();
        }
        break;

    // wrapper entry points are allocated from the end in a fixed-size buffer.
    case EXEC_WRAPPER:
        mem = __sync_fetch_and_add(&WRAPPER_END, size);
        if((mem + size) > EXEC_END) {
            granary_fault();
        }
        break;

    default:
        granary_fault();
        break;
    }

    return (void *) mem;
}


/// granary::is_code_cache_address
int _ZN7granary21is_code_cache_addressEPh(unsigned long addr) {
    return EXEC_START <= addr && addr < CODE_CACHE_END;
}


/// granary::is_wrapper_address
int _ZN7granary18is_wrapper_addressEPh(unsigned long addr) {
    return WRAPPER_START <= addr && addr < WRAPPER_END;
}


/// Allocate some executable memory
static void *allocate(unsigned long size) {
    void *ret = kmalloc(size, GFP_ATOMIC);
    if(!ret) {
        granary_fault();
    }
    return ret;
}


static int granary_device_open = 0;
static int granary_device_initialised = 0;


/// Open Granary as a device.
static int device_open(struct inode *inode, struct file *file) {
    if(granary_device_open) {
        return -EBUSY;
    }

    granary_device_open++;

    if(!granary_device_initialised) {
        granary_device_initialised = 1;
        preallocate_executable();
        granary_initialise();
    }

    (void) inode;
    (void) file;

    return SUCCESS;
}


/// Close Granary as a device.
static int device_close(struct inode *inode, struct file *file) {
    if(granary_device_open) {
        granary_device_open--;
    }

    (void) inode;
    (void) file;

    return SUCCESS;
}


/// Tell a Granary device to run a command.
static ssize_t device_write(
    struct file *file, const char *str, size_t size, loff_t *offset
) {
    (void) file;
    (void) str;
    (void) size;
    (void) offset;
    return 0;
}


/// Tell a Granary device to run a command.
static ssize_t device_read(
    struct file *file, char *str, size_t size, loff_t *offset
) {
    (void) file;
    (void) str;
    (void) size;
    (void) offset;
    return 0;
}


struct file_operations operations = {
    .owner      = THIS_MODULE,
    .open       = device_open,
    .release    = device_close,
    .write      = device_write,
    .read       = device_read
};


struct miscdevice device = {
    .minor      = 0,
    .name       = "granary",
    .fops       = &operations
};


/// Initialise Granary.
static int init_granary(void) {

    *kernel_malloc_exec = allocate_executable;
    *kernel_malloc = allocate;
    *kernel_free = kfree;

    printk("Stack size is %lu\n", THREAD_SIZE);

    printk("Loading Granary...\n");

    printk("    Running initialisers...\n");
    granary_run_initialisers();
    printk("    Done running initialisers.\n");



    printk("    Registering module notifier...\n");

    register_module_notifier(&notifier_block);

    printk("    Registering 'granary' device...\n");

    if(0 != misc_register(&device)) {
        printk("        Unable to register 'granary' device.\n");
    } else {
        printk("        Registered 'granary' device.\n");
    }

    printk("    Done; waiting for command to initialise Granary.\n");

    return 0;
}


/// Remove Granary.
static void exit_granary(void) {
    struct kernel_module *mod = modules;
    struct kernel_module *next_mod = NULL;

    printk("Unloading Granary... Goodbye!\n");
    unregister_module_notifier(&notifier_block);
    misc_register(&device);

    // free the memory associated with internal modules
    for(; NULL != mod; mod = next_mod) {
        next_mod = mod->next;
        kfree(mod);
    }
}

module_init(init_granary);
module_exit(exit_granary);
