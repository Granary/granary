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
#include <linux/stop_machine.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/relay.h>

#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/thread_info.h>
#include <asm/desc.h>

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


/// Configuration for RelayFS.
#define SUBBUF_SIZE 262144
#define N_SUBBUFS 4
struct rchan *GRANARY_RELAY_CHANNEL = NULL;


/// Get the base of the kernel's interrupt descriptor table.
void *kernel_get_idt_table(void) {
    return (void *) DETACH_ADDR_idt_table;
}


/// Get access to per-CPU Granary state.
__attribute__((hot, optimize("O3")))
void **kernel_get_cpu_state(void *ptr[]) {
    return &(ptr[raw_smp_processor_id()]);
}


/// Get access to the per-task Granary state. The Granary state field might
/// be as small as a pointer, or might be a larger structure, depending on how
/// the kernel's task struct has been changed.
__attribute__((hot, optimize("O3")))
void *kernel_get_thread_state(void) {
    return (void *) &((current)->granary);
}


/// Run a function on each CPU.
void kernel_run_on_each_cpu(void (*func)(void)) {
    on_each_cpu(((void (*)(void *)) func), NULL, 1);
}


/// Search for an exception table entry.
const void *kernel_search_exception_tables(void *pc) {
#ifdef DETACH_ADDR_search_exception_tables
    const void *(*search_exception_tables)(void *) = \
        (const void *(*)(void *)) DETACH_ADDR_search_exception_tables;
    return search_exception_tables(pc);
#else
#   return nullptr;
#endif
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


/// Function that is called on a weird event in Granary.
void granary_break_on_curiosity(void) {
    __asm__ __volatile__ ("");
}


/// Function that is called in order to force a fault.
int granary_fault(void) {
    __asm__ __volatile__ ("int3; int3; mov 0, %rax;");
    return 1;
}


/// Allocate memory for an interrupt descriptor table (IDT).
void *granary_allocate_idt(void) {
    return (void *) __get_free_page(GFP_ATOMIC);
}


static int do_init_sync(void *func) {
    ((void (*)(void)) func)();
    return 0;
}


/// Call a function where all CPUs are synchronised.
void kernel_run_synchronised(void (*func)(void)) {
    stop_machine(do_init_sync, (void *) func, NULL);
}


/// Log some data to user space through RelayFS.
void kernel_log(const char *data, size_t size) {
    if(GRANARY_RELAY_CHANNEL) {
        relay_write(GRANARY_RELAY_CHANNEL, data, size);
    }
}


/// Linked list of Granary-recognized modules.
static struct kernel_module *LOADED_MODULES = NULL;


enum {
    /// Bounds on where kernel module code is placed
    MODULE_TEXT_START = 0xffffffffa0000000ULL,
    MODULE_TEXT_END = 0xfffffffffff00000ULL,

    /// Bounds on where non-module kernel code is placed.
    KERNEL_TEXT_START = 0xffffffff80000000ULL,
    KERNEL_TEXT_END = MODULE_TEXT_START
};


static struct kernel_module KERNEL_MODULE = {
    .is_granary = 0,
    .is_instrumented = 0,
    .address = NULL,
    .text_begin = (void *) KERNEL_TEXT_START,
    .text_end = (void *) KERNEL_TEXT_END,
    .name = "linux",
    .next = NULL
};


/// Exposed for the C++ side of things to see for the debug memset hot-patcher.
unsigned long EXEC_START = 0;
static unsigned long EXEC_END = 0;


/// Layout of the executable region:
///
///  EXEC_START                GEN_CODE_START   WRAPPER_START       EXEC_END
///      |--------------->              <--------------|-------->      |
///                CODE_CACHE_END                          WRAPPER_END
///
unsigned long CODE_CACHE_END = 0;
static unsigned long GEN_CODE_START = 0;
static unsigned long WRAPPER_START = 0;
static unsigned long WRAPPER_END = 0;

static int DEVICE_IS_OPEN = 0;
static int DEVICE_IS_INITIALISED = 0;


/// Returns true if an address is a kernel address, or native kernel module.
int is_host_address(uintptr_t addr) {
    if(KERNEL_TEXT_START <= addr && addr < KERNEL_TEXT_END) {
        return 1;
    } else if(EXEC_START <= addr && addr < EXEC_END) {
        return 0;
    } else if(MODULE_TEXT_START <= addr && addr < MODULE_TEXT_END) {
        struct kernel_module *mod = LOADED_MODULES;
        for(; mod; mod = mod->next) {
            if(((uintptr_t) mod->text_begin) <= addr
            && addr < ((uintptr_t) mod->max_text_end)) {
                return mod->is_granary;
            }
        }
    }
    return 0;
}


/// Returns the kernel module information for a given app address.
const struct kernel_module *kernel_get_module(uintptr_t addr) {
    struct kernel_module *found_mod = NULL;
    if(MODULE_TEXT_START <= addr && addr < MODULE_TEXT_END) {
        struct kernel_module *mod;
        for(mod = LOADED_MODULES; mod; mod = mod->next) {
            if(((uintptr_t) mod->text_begin) <= addr
            && addr < ((uintptr_t) mod->max_text_end)) {
                found_mod = mod;
            }
        }
    } else if(KERNEL_TEXT_START <= addr && addr < KERNEL_TEXT_END) {
        return &KERNEL_MODULE;
    }
    return found_mod;
}


/// Returns true iff an address belongs to a Granary-instrumented kernel
/// module.
int is_app_address(uintptr_t addr) {
    if(MODULE_TEXT_START <= addr && addr < MODULE_TEXT_END) {
        struct kernel_module *mod = LOADED_MODULES;
        for(; mod; mod = mod->next) {
            if(((uintptr_t) mod->text_begin) <= addr
            && addr < ((uintptr_t) mod->max_text_end)) {
                return !(mod->is_granary);
            }
        }
    }
    return 0;
}


/// Helper function for setting the page permissions of some pages.
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
    (void) module;
    /*
    set_page_perms(
        set_memory_nx,
        module->text_begin,
        module->text_end
    ); */
}


/// Set a module's text to be read-only.
void granary_before_module_bootstrap(struct kernel_module *module) {
    set_page_perms(
        set_memory_ro,
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


/// Mark a page as being readable and writeable.
void kernel_make_memory_writeable(void *addr) {
    unsigned level = 0;
    pte_t *pte = lookup_address((unsigned long) addr, &level);
    pte->pte |= _PAGE_RW;
}


/// Mark a page as being only readable.
void kernel_make_page_read_only(void *addr) {
    set_page_perms(set_memory_ro, addr, (void *) (((uintptr_t) addr) + 1));
}


/// Mark a page as being executable. Add PAGE_SIZE here so it will likely cover
/// two adjacent pages.
void kernel_make_page_executable(void *addr) {
    set_page_perms(
        set_memory_x,
        addr,
        (void *) (((uintptr_t) addr) + PAGE_SIZE)
    );
}


/// C++-implemented function that operates on modules. This is the
/// bridge from C to C++.
extern void notify_module_state_change(struct kernel_module *);


/// Find the Granary-representation for an internal module.
static struct kernel_module *find_internal_module(void *vmod) {

    struct kernel_module *module = LOADED_MODULES;
    struct kernel_module **next_link = &LOADED_MODULES;
    const int is_granary = NULL == LOADED_MODULES;
    struct module *mod = (struct module *) vmod;

    for(; NULL != module; module = module->next) {
        if(module->text_begin == mod->module_core) {
            return module;
        }
        next_link = &(module->next);
    }

    // We don't care about modules that are being unloaded and that we
    // previously didn't know about.
    if(MODULE_STATE_GOING == mod->state) {
        return NULL;
    }

    module = kmalloc(sizeof(struct kernel_module), GFP_KERNEL);

    // Make sure to convert module pointer into an unwatched address, just
    // in case we're using watchpoints and patch-wrapping kmalloc return
    // addresses.
    module = (struct kernel_module *) ((0xFFFFULL << 48) | ((uintptr_t) module));

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

    module->max_text_end = module->text_end;
    if(module->ro_text_end > module->text_end) {
        module->max_text_end = module->ro_text_end;
    }

    module->next = NULL;
    module->name = mod->name;
    module->is_instrumented = DEVICE_IS_INITIALISED;

    if(!is_granary) {
        module_set_exec_perms(module);
    }

    // Chain it in and return.
    *next_link = module;

    return module;
}


/// Notify Granary's back-end of a state change to a particular module.
///
/// Note: Not static so that we can see it in granary/detach.cc.
int module_load_notifier(
    struct notifier_block *nb,
    unsigned long mod_state,
    void *vmod
) {
    struct kernel_module *internal_mod = NULL;
    struct module *mod = (struct module *) vmod;
    printk("[granary] Notified of module 0x%p [.text = %p]\n",
        vmod, mod->module_core);
    printk("[granary] Module's name is: %s.\n", mod->name);

    internal_mod = find_internal_module(vmod);

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
static struct notifier_block NOTIFIER_BLOCK = {
    .notifier_call = &module_load_notifier,
    .next = NULL,
    .priority = -1,
};


enum {
    _1_MB = 1048576,
    CODE_CACHE_SIZE = 40 * _1_MB,
    _1_P = 4096,

    // Maximum size of the part of the code cache containing basic blocks /
    // fragments.
    FRAGMENT_CACHE_MAX_SIZE = CODE_CACHE_SIZE - _1_MB,

    // Keep this consistent with `granary/state.h`,
    // `fragment_allocator_config::SLAB_SIZE`
    FRAGMENT_SLAB_SIZE = _1_P * 8,

    // Maximum number of fragment slabs.
    MAX_NUM_FRAGMENT_SLABS = FRAGMENT_CACHE_MAX_SIZE / FRAGMENT_SLAB_SIZE
};


/// Allocate a "fake" module that will serve as a lasting memory zone for
/// executable allocations.
static void preallocate_executable(void) {

    typedef void *(module_alloc_t)(unsigned long);

    /// What is used internally by the kernel to allocate modules :D
    module_alloc_t *module_alloc_update_bounds  =
        (module_alloc_t *) DETACH_ADDR_module_alloc_update_bounds;

    void *mem = module_alloc_update_bounds(CODE_CACHE_SIZE);
    if(!mem) {
        granary_fault();
    }

    // Memset all module code to be int3 instructions.
    memset(mem, 0xCC, CODE_CACHE_SIZE);

    EXEC_START = (unsigned long) mem;
    EXEC_END = EXEC_START + CODE_CACHE_SIZE;

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


static void *FRAGMENT_SLABS[MAX_NUM_FRAGMENT_SLABS] = {NULL};


/// Given an arbitrary address into a basic block, we want to be able to find
/// the associated basic block info / meta-data. The approach is to first find
/// the slab to which the basic block belongs, and then from there binary search
/// over all basic blocks allocated in that slab.
void **granary_find_fragment_slab(unsigned long fragment_addr) {
    const unsigned index = (fragment_addr - EXEC_START) / FRAGMENT_SLAB_SIZE;
    return &(FRAGMENT_SLABS[index]);
}


/// Allocate some executable memory
void *kernel_alloc_executable(unsigned long size, int where) {
    unsigned long mem = 0;
    switch(where) {

    // code cache pages are allocated from the beginning
    case EXEC_CODE_CACHE:
        mem = __sync_fetch_and_add(&CODE_CACHE_END, size);
        if((mem + size) > GEN_CODE_START) {
            //printk("[granary] Unable to allocate fragment code!\n\n");
            granary_fault();
        }
        break;

    // gencode pages are allocated from near the end
    case EXEC_GEN_CODE:
        mem = __sync_sub_and_fetch(&GEN_CODE_START, size);
        if(mem < CODE_CACHE_END) {
            //printk("[granary] Unable to allocate gencode!\n\n");
            granary_fault();
        }
        break;

    // wrapper entry points are allocated from the end in a fixed-size buffer.
    case EXEC_WRAPPER:
        mem = __sync_fetch_and_add(&WRAPPER_END, size);
        if((mem + size) > EXEC_END) {
            //printk("[granary] Unable to allocate wrapper code!\n\n");
            granary_fault();
        }
        break;

    default:
        //printk("[granary] Unknown executable allocation type!\n\n");
        granary_fault();
        break;
    }

    return (void *) mem; // Should all already be memset to 0xCC.
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


/// Open Granary as a device.
static int device_open(struct inode *inode, struct file *file) {
    if(DEVICE_IS_OPEN) {
        return -EBUSY;
    }

    DEVICE_IS_OPEN++;

    if(!DEVICE_IS_INITIALISED) {
        DEVICE_IS_INITIALISED = 1;
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
    if(DEVICE_IS_OPEN) {
        DEVICE_IS_OPEN--;
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


static struct dentry *create_relay_file_handler(
    const char *filename,
    struct dentry *parent,
    umode_t mode,
    struct rchan_buf *buf,
    int *is_global
) {
    struct dentry *buf_file;
    buf_file = debugfs_create_file(
        filename, mode, parent, buf, &relay_file_operations);
    *is_global = 1;
    return buf_file;
}


static int remove_relay_file_handler(struct dentry *dentry) {
    debugfs_remove(dentry);
    return 0;
}


static struct file_operations operations = {
    .owner      = THIS_MODULE,
    .open       = device_open,
    .release    = device_close,
    .write      = device_write,
    .read       = device_read
};


static struct miscdevice device = {
    .minor      = 0,
    .name       = "granary",
    .fops       = &operations
};

static struct rchan_callbacks relay_operations = {
    .create_buf_file = create_relay_file_handler,
    .remove_buf_file = remove_relay_file_handler
};


/// C++ operator new and delete variants.
extern void *_Znwm(void) { granary_fault(); return NULL; }
extern void *_Znam(void) { granary_fault(); return NULL; }
extern void _ZdlPv(void) { granary_fault(); }
extern void _ZdaPv(void) { granary_fault(); }


/// Initialise Granary.
static int init_granary(void) {

    printk("[granary] Loading Granary...\n");
    printk("[granary] Stack size is %lu\n", THREAD_SIZE);
    printk("[granary] Running initialisers...\n");

    granary_run_initialisers();

    printk("[granary] Done running initialisers.\n");
    printk("[granary] Registering module notifier...\n");

    register_module_notifier(&NOTIFIER_BLOCK);

    printk("[granary] Registering 'granary' device...\n");

    if(0 != misc_register(&device)) {
        printk("[granary] Unable to register 'granary' device.\n");
    } else {
        printk("[granary] Registered 'granary' device.\n");
    }

    // Initialise a channel with RelayFS.
    GRANARY_RELAY_CHANNEL = relay_open(
        "granary", NULL, SUBBUF_SIZE, N_SUBBUFS, &relay_operations, NULL);
    if (!GRANARY_RELAY_CHANNEL) {
        printk("[granary] Unable to initialise the `granary` relay channel.\n");
    } else {
        printk("[granary] Relay channel initialised.\n");
    }

    printk("[granary] Done; waiting for command to initialise Granary.\n");

    return 0;
}


/// Remove Granary.
static void exit_granary(void) {
    struct kernel_module *mod = LOADED_MODULES;
    struct kernel_module *next_mod = NULL;

    printk("Unloading Granary... Goodbye!\n");
    unregister_module_notifier(&NOTIFIER_BLOCK);
    misc_register(&device);

    // free the memory associated with internal modules
    for(; NULL != mod; mod = next_mod) {
        next_mod = mod->next;
        kfree(mod);
    }
}


module_init(init_granary);
module_exit(exit_granary);

