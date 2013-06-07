/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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
#include <linux/compiler.h>
#include <linux/version.h>
#include <linux/gfp.h>

#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/thread_info.h>

#define LINUX_MAJOR_VERSION ((LINUX_VERSION_CODE >> 16) & 0xFF)
#define LINUX_MINOR_VERSION ((LINUX_VERSION_CODE >> 8)  & 0xFF)
#define LINUX_PATCH_VERSION ((LINUX_VERSION_CODE >> 0)  & 0xFF)

#include "granary/kernel/linux/module.h"

#define WRAP_FOR_DETACH(func)
#define DETACH(func)
#define TYPED_DETACH(func)

#include "granary/gen/kernel_detach.inc"

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
__attribute__((hot))
void **kernel_get_cpu_state(void *ptr[]) {
    return &(ptr[smp_processor_id()]);
}


/// Run a function on each CPU.
void kernel_run_on_each_cpu(void (*func)(void *), void *thunk) {
    on_each_cpu(func, thunk, 1);
}


/// Assembly function to run constructors for globally-defined C++ data
/// structures.
extern void granary_run_initialisers(void);


/// C function defined in granary/kernel/module.cc for initialising Granary.
/// This function is invoked when an ioctl is used to tell Granary to
/// initialise.
extern void granary_initialise(void);


/// C funciton defined in granary/kernel/module.cc for communicating performance
/// counters, etc.
extern void granary_report(void);


/// Function that is called before granary faults.
void granary_break_on_fault(void) {
    __asm__ __volatile__ ("");
}


/// Function that is called in order to force a fault.
int granary_fault(void) {
    __asm__ __volatile__ ("mov 0, %rax;");
    return 1;
}


/// Allocate memory for an interrupt descriptor table (IDT).
void *granary_allocate_idt(void) {
    return (void *) __get_free_page(GFP_ATOMIC);
}


/// Notify the kernel that pre-emption is disabled. Granary does this even
/// when interrupts are disabled.
__attribute__((hot))
void kernel_preempt_disable(void) {
    inc_preempt_count();
    barrier();
}


/// Notify the kernel that pre-emption is re-enabled. Granary does this even
/// when interrupts remain disabled, with the understanding that Granary won't
/// do anything "interesting" (i.e. call kernel functions that might inspect
/// the pre-emption state) until it re-enabled interrupts.
__attribute__((hot))
void kernel_preempt_enable(void) {
    barrier();
    dec_preempt_count();
}


struct kernel_module *modules = NULL;
extern void *(**kernel_malloc_exec)(unsigned long, int);
extern void *(**kernel_malloc)(unsigned long);
extern void (**kernel_free)(const void *);

static unsigned long EXEC_START = 0;
static unsigned long EXEC_END = 0;


/// Layout of the executable region:
///
///  EXEC_START                GEN_CODE_START   WRAPPER_START       EXEC_END
///      |--------------->              <--------------|----------------|
///                CODE_CACHE_END                                WRAPPER_END
///
static unsigned long CODE_CACHE_END = 0;
static unsigned long GEN_CODE_START = 0;
static unsigned long WRAPPER_START = 0;
static unsigned long WRAPPER_END = 0;

static int granary_device_open = 0;
static int granary_device_initialised = 0;


static void set_page_perms(
    int (*set_memory_)(unsigned long, int),
    void *begin,
    void *end
) {
    const uint64_t begin_pfn = PFN_DOWN(((uint64_t) begin));
    const uint64_t end_pfn = PFN_DOWN(((uint64_t) end));

    if(begin == end) {
        return;
    }

    if(end_pfn > begin_pfn) {
        set_memory_(begin_pfn << PAGE_SHIFT, end_pfn - begin_pfn);

    } else if(end_pfn == begin_pfn) {
        set_memory_(begin_pfn << PAGE_SHIFT, 1);
    }
}


/// Set a module's text to be non-executable
static void module_set_exec_perms(struct kernel_module *module) {
    set_page_perms(
        set_memory_nx,
        module->text_begin,
        module->text_end
    );
}


/// A callback that is invoked *immediately* before the module's init function
/// is invoked.
void granary_before_module_init(struct kernel_module *module) {
    set_page_perms(
        set_memory_rw,
        module->ro_text_begin,
        module->ro_text_end
    );

    set_page_perms(
        set_memory_rw,
        module->ro_init_begin,
        module->ro_init_end
    );
}


/// C++-implemented function that operates on modules. This is the
/// bridge from C to C++.
extern void notify_module_state_change(struct kernel_module *);


/// Find the Granary-representation for an internal module.
static struct kernel_module *find_interal_module(void *vmod) {

    struct kernel_module *module = modules;
    struct kernel_module **next_link = &modules;
    const int is_granary = NULL == modules;
    struct module *mod = (struct module *) vmod;

    for(; NULL != module; module = module->next) {
        if(module->text_begin == mod->module_core) {
            return module;
        }
        next_link = &(module->next);
    }

    // we don't care about modules that are being unloaded and that we
    // previously didn't know about.
    if(MODULE_STATE_GOING == mod->state) {
        return NULL;
    }

    module = kmalloc(sizeof(struct kernel_module), GFP_KERNEL);

    // Initialise.
    module->is_granary = is_granary;
    module->init = &(mod->init);
#ifdef CONFIG_MODULE_UNLOAD
    module->exit = &(mod->exit);
#else
    module->exit = NULL;
#endif
    module->address = vmod;
    module->text_begin = mod->module_core;
    module->text_end = mod->module_core + mod->core_text_size;

    // read-only data sections
    module->ro_text_begin = module->text_end;
    module->ro_text_end =
        module->ro_text_begin + (mod->core_ro_size - mod->core_text_size);

    module->ro_init_begin = mod->module_init + mod->init_text_size;
    module->ro_init_end =
        module->ro_init_begin + (mod->init_ro_size - mod->init_text_size);

    module->next = NULL;
    module->name = mod->name;
    module->is_instrumented = granary_device_initialised;

    if(!is_granary) {
        module_set_exec_perms(module);
    }

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
    printk("[granary] Notified of module 0x%p [.text = %p]\n",
        vmod, mod->module_core);
    printk("[granary] Module's name is: %s.\n", mod->name);

    internal_mod = find_interal_module(vmod);

    if(!internal_mod || !(internal_mod->is_instrumented)) {
        printk("[granary] Ignoring module state change.\n");
        return 0;
    }

    printk("[granary] Got internal representation for module.\n");
    internal_mod->state = mod_state;

    if(mod_state) {
        module_set_exec_perms(internal_mod);
    }

    printk("[granary] Notifying Granary of the module...\n");
    notify_module_state_change(internal_mod);
    printk("[granary] Notified Granary of the module.\n");

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
static void preallocate_executable(void) {

    typedef void *(module_alloc_t)(unsigned long);

    enum {
        _1_MB = 1048576,
        _100_MB = 100 * _1_MB,
        _1_P = 4096
    };

    /// What is used internally by the kernel to allocate modules :D
    module_alloc_t *module_alloc_update_bounds  =
        (module_alloc_t *) DETACH_ADDR_module_alloc_update_bounds;

    void *mem = module_alloc_update_bounds(_100_MB);
    if(!mem) {
        granary_fault();
    }

    EXEC_START = (unsigned long) mem;
    EXEC_END = EXEC_START + _100_MB;

    set_page_perms(
        set_memory_x,
        (void *) EXEC_START,
        (void *) EXEC_END
    );

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
int is_code_cache_address(unsigned long addr) {
    return EXEC_START <= addr && addr < CODE_CACHE_END;
}


/// granary::is_wrapper_address
int is_wrapper_address(unsigned long addr) {
    return WRAPPER_START <= addr && addr < WRAPPER_END;
}


/// granary::is_gencode_address
int is_gencode_address(unsigned long addr) {
    return GEN_CODE_START <= addr && addr < WRAPPER_START;
}


/// Allocate some non-executable memory
static void *allocate(unsigned long size) {
    void *ret = kmalloc(size, preempt_count() ? GFP_ATOMIC : GFP_KERNEL);
    if(!ret) {
        granary_fault();
    }
    return ret;
}


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
    } else {
        granary_report();
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

    printk("[granary] Loading Granary...\n");
    printk("[granary] Stack size is %lu\n", THREAD_SIZE);
    printk("[granary] Running initialisers...\n");

    granary_run_initialisers();

    printk("[granary] Done running initialisers.\n");
    printk("[granary] Registering module notifier...\n");

    register_module_notifier(&notifier_block);

    printk("[granary] Registering 'granary' device...\n");

    if(0 != misc_register(&device)) {
        printk("[granary] Unable to register 'granary' device.\n");
    } else {
        printk("[granary] Registered 'granary' device.\n");
    }

    printk("[granary] Done; waiting for command to initialise Granary.\n");

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
