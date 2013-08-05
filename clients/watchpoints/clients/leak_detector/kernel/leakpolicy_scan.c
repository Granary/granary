/*
 * leakpolicy_scan.c
 *
 *  Created on: 2013-06-24
 *      Author: akshayk
 */


#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/stop_machine.h>
#include <linux/mm.h>
#include <linux/hugetlb_inline.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <asm/pgtable_64.h>

#include <asm/pgtable.h>
#include <asm/highmem.h>
#include <asm/context_tracking.h>
#include <asm/paravirt.h>
#include <linux/swap.h>
#include <linux/swapops.h>

#define D(x)

#define USER_OR_MASK (0x1ULL<<48)

#define HEAP_BASE 0x880000000000

typedef unsigned char* app_pc;

typedef void (scanner_callback)(void);


/* indices for address_markers; keep sync'd w/ address_markers below */
enum address_markers_idx {
    USER_SPACE_NR = 0,
    KERNEL_SPACE_NR,
    LOW_KERNEL_NR,
    VMALLOC_START_NR,
    VMEMMAP_START_NR,
    HIGH_KERNEL_NR,
    MODULES_VADDR_NR,
    MODULES_END_NR,
};

struct pg_state {
    int level;
    pgprot_t current_prot;
    unsigned long start_address;
    unsigned long current_address;
    const struct addr_marker *marker;
};

struct addr_marker {
    unsigned long start_address;
    const char *name;
};


/* Address space markers hints */
static struct addr_marker address_markers[] = {
    { 0, "User Space" },
    { 0x8000000000000000UL, "Kernel Space" },
    { PAGE_OFFSET,      "Low Kernel Mapping" },
    { VMALLOC_START,        "vmalloc() Area" },
    { VMEMMAP_START,        "Vmemmap" },
    { __START_KERNEL_map,   "High Kernel Mapping" },
    { MODULES_VADDR,        "Modules" },
    { MODULES_END,          "End Modules" },
    { -1, NULL }        /* End of list */
};

/* Multipliers for offsets within the PTEs */
#define PTE_LEVEL_MULT (PAGE_SIZE)
#define PMD_LEVEL_MULT (PTRS_PER_PTE * PTE_LEVEL_MULT)
#define PUD_LEVEL_MULT (PTRS_PER_PMD * PMD_LEVEL_MULT)
#define PGD_LEVEL_MULT (PTRS_PER_PUD * PUD_LEVEL_MULT)


typedef struct {
    unsigned long address;
    struct module *module;
    bool has_size;
    size_t size;
    const char *name;
} kernel_symbol_t;


scanner_callback *scan_thread_callback = NULL;
scanner_callback *update_rootset_callback = NULL;

static int (*module_walk_page_range)(unsigned long, unsigned long, struct mm_walk*) = NULL;

static void (*watchpoint_callback)(unsigned long) = NULL;

static void *module_init_level4_pgt = NULL;

void get_task_pages(struct task_struct *tsk);
void walk_pgd_level(void);

/*
 * On 64 bits, sign-extend the 48 bit address to 64 bit
 */
inline unsigned long normalize_addr(unsigned long u)
{
    return (signed long)(u << 16) >> 16;
}


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

    unsigned long flags;

    while(!kthread_should_stop()){

        stop_machine(stop_machine_callback, NULL, NULL);

        preempt_disable();
        raw_local_irq_save(flags);
        D(printk("inside scanning thread\n");)

        if(scan_thread_callback)
            scan_thread_callback();

        raw_local_irq_restore(flags);
        preempt_enable();
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(100*HZ);
    }

    set_current_state(TASK_RUNNING);
    return 0;
}

static int
find_kernel_sybmol_callback(void *data, const char *name, struct module *module,
                            unsigned long address)
{
    kernel_symbol_t *symbol = (kernel_symbol_t*) data;
    if (strcmp(name, symbol->name) == 0) {
        symbol->module = module;
        symbol->address = address;
        return 1;
    }
    return 0;
}

static bool
find_kernel_symbol(kernel_symbol_t *symbol)
{
    if (kallsyms_on_each_symbol(find_kernel_sybmol_callback, symbol)) {
        return true;
    }
    printk("find_kernel_symbol failed for %s\n", symbol->name);
    return false;
}

static void*
find_kernel_symbol_address(const char *name)
{
    kernel_symbol_t symbol;
    symbol.name = name;
    if (find_kernel_symbol(&symbol)) {
        return (void*) symbol.address;
    } else {
        return NULL;
    }
}

void
leak_policy_scanner_init(const app_pc thread_callback, const app_pc rootset_callback){

    scan_thread_callback = (scanner_callback*)thread_callback;
    update_rootset_callback = (scanner_callback*)rootset_callback;

    D(printk("inside function leak_policy_scanner_init\n");)

    {
        module_walk_page_range = find_kernel_symbol_address("walk_page_range");

        /* kernel space page table*/
        module_init_level4_pgt = find_kernel_symbol_address("init_level4_pgt");

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
}


struct thread_info*
kernel_current_thread_info(void){
    return current_thread_info();
}

struct task_struct*
kernel_get_current(void){
    struct task_struct *tsk = get_current();
    return tsk;
}

/* code changes for page table walk and scanning for the watched objects*/


/*
 * Check 47th and 48th bit of address to decide if this is watchpoint
 */
inline bool is_watched_address(uint64_t ptr) {
    enum {
        MASK_47_48 = (USER_OR_MASK >> 1) | USER_OR_MASK
    };
    const uint64_t masked_ptr = (ptr & 0xFFFFFFFFFFFFULL);
    if(masked_ptr > HEAP_BASE)
        return true;

    return false;
    //return masked_ptr && MASK_47_48 != masked_ptr;
}

/* Function for scanning the kernel pages and look for the watched object*/

void scan_kernel_page(unsigned long addr){
    unsigned long offset = 0;

    for(offset=0; offset < PAGE_SIZE; offset=offset+8){
        uint64_t *ptr = (uint64_t*)(addr+offset);

        if(is_watched_address(*ptr)){
            watchpoint_callback(*ptr);
        }
    }
}


void scan_kernel_memory(uint64_t *start, uint64_t *end){
    for(; start <= end; start++){
        if(virt_addr_valid(start)){
            uint64_t *ptr = (uint64_t*)(start);

            if(is_watched_address(*ptr)){
                watchpoint_callback(*ptr);
            }
        }
    }
}



/* pte_entry callback which gets called for all the page table entries in a virtual address*/

static int
page_table_callback(pte_t *pte, unsigned long addr,
    unsigned long next, struct mm_walk *walk)
{
    unsigned int i;
    uint64_t ptr;

    for( i=0; i<PAGE_SIZE; i= i+8 ){
        if (pte_present(*pte)){
            ptr = (uint64_t)pfn_to_kaddr(pte_pfn(*pte));
            D(printk("present( %llx )", ptr);)
            if(virt_addr_valid(ptr))
                scan_kernel_page(ptr);
        }else if (is_swap_pte(*pte)) {
            swp_entry_t swpent = pte_to_swp_entry(*pte);
            ptr = (uint64_t)pfn_to_kaddr(swp_offset(swpent));
            D(printk("swap( %llx )", p);)
        } else if (pte_file(*pte)) {
            D(printk("pte in file\n");)
        }

        pte++;
    }

    return 0;
}

/*pmd_entry callback for walking the entire range of pte*/
static int
pte_range_callback(pmd_t *pmd, unsigned long addr,
        unsigned long end, struct mm_walk *walk)
{
    pte_t *pte;
    uint64_t ptr;

    for(; addr != end; addr += PAGE_SIZE){
        pte = pte_offset_map(pmd, addr);
        ptr = (uint64_t)pfn_to_kaddr(pte_pfn(*pte));
    }
    return 0;
}


void get_task_pages(struct task_struct *tsk){
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    struct mm_walk walk = {0,};

    if(tsk->mm){
        mm = tsk->mm;
    }else {
        mm = tsk->active_mm;
    }

    if(!mm)
        return;

    walk.pmd_entry = pte_range_callback;
    walk.pte_entry = page_table_callback;
    walk.mm = mm;

    down_read(&mm->mmap_sem);

    for (vma = mm->mmap; vma; vma = vma->vm_next) {
        if (is_vm_hugetlb_page(vma)){
            continue;
        }

        if(module_walk_page_range){
            module_walk_page_range(vma->vm_start, vma->vm_end, &walk);
            D(printk("range : start : %llx, end : %llx\n", vma->vm_start, vma->vm_end);)
        }

    }

    up_read(&mm->mmap_sem);
}

/*
 * Print a readable form of a pgprot_t to the seq_file
 */
static void printk_prot(pgprot_t prot, int level)
{
    pgprotval_t pr = pgprot_val(prot);
    static const char * const level_name[] =
        { "cr3", "pgd", "pud", "pmd", "pte" };

    if (!pgprot_val(prot)) {
        /* Not present */
        printk( "                          ");
    } else {
        if (pr & _PAGE_USER)
            printk( "USR ");
        else
            printk( "    ");
        if (pr & _PAGE_RW)
            printk( "RW ");
        else
            printk( "ro ");
        if (pr & _PAGE_PWT)
            printk( "PWT ");
        else
            printk( "    ");
        if (pr & _PAGE_PCD)
            printk( "PCD ");
        else
            printk( "    ");

        /* Bit 9 has a different meaning on level 3 vs 4 */
        if (level <= 3) {
            if (pr & _PAGE_PSE)
                printk( "PSE ");
            else
                printk( "    ");
        } else {
            if (pr & _PAGE_PAT)
                printk( "pat ");
            else
                printk( "    ");
        }
        if (pr & _PAGE_GLOBAL)
            printk( "GLB ");
        else
            printk( "    ");
        if (pr & _PAGE_NX)
            printk( "NX ");
        else
            printk( "x  ");
    }
    printk( "%s\n", level_name[level]);
}



static void note_page(struct pg_state *st,
              pgprot_t new_prot, int level)
{
    pgprotval_t prot, cur;
    static const char units[] = "KMGTPE";

    /*
     * If we have a "break" in the series, we need to flush the state that
     * we have now. "break" is either changing perms, levels or
     * address space marker.
     */
    prot = pgprot_val(new_prot) & PTE_FLAGS_MASK;
    cur = pgprot_val(st->current_prot) & PTE_FLAGS_MASK;

    if (!st->level) {
        /* First entry */
        st->current_prot = new_prot;
        st->level = level;
        st->marker = address_markers;
        printk( "---[ %s ]---\n", st->marker->name);
    } else if (prot != cur || level != st->level ||
           st->current_address >= st->marker[1].start_address) {
        const char *unit = units;
        unsigned long delta;
        int width = sizeof(unsigned long) * 2;

        /*
         * Now print the actual finished series
         */
        if((cur & _PAGE_RW) && (cur & _PAGE_NX)) {
            printk("0x%0*lx-0x%0*lx   ",
                    width, st->start_address,
                    width, st->current_address);

            delta = (st->current_address - st->start_address) >> 10;
            while (!(delta & 1023) && unit[1]) {
            delta >>= 10;
            unit++;
            }
            printk( "%9lu%c ", delta, *unit);
            printk_prot(st->current_prot, st->level);

            scan_kernel_memory((uint64_t*)st->start_address, (uint64_t*)st->current_address);
        }

        /*
         * We print markers for special areas of address space,
         * such as the start of vmalloc space etc.
         * This helps in the interpretation.
         */
        if (st->current_address >= st->marker[1].start_address) {
            st->marker++;
            printk( "---[ %s ]---\n", st->marker->name);
        }

        st->start_address = st->current_address;
        st->current_prot = new_prot;
        st->level = level;
    }
}

static void walk_pte_level(struct pg_state *st, pmd_t addr,
                            unsigned long P)
{
    int i;
    pte_t *start;

    start = (pte_t *) pmd_page_vaddr(addr);
    for (i = 0; i < PTRS_PER_PTE; i++) {
        pgprot_t prot = pte_pgprot(*start);
        if(!pte_none(*start) && pte_present(*start)){
            st->current_address = normalize_addr(P + i * PTE_LEVEL_MULT);
            note_page(st, prot, 4);
        }else
            note_page(st, __pgprot(0), 4);

        start++;
    }
}

static void walk_pmd_level(struct pg_state *st, pud_t addr,
                            unsigned long P)
{
    int i;
    pmd_t *start;

    start = (pmd_t *) pud_page_vaddr(addr);
    for (i = 0; i < PTRS_PER_PMD; i++) {
        if (!pmd_none(*start) && pmd_present(*start)) {
            pgprotval_t prot = pmd_val(*start) & PTE_FLAGS_MASK;
            st->current_address = normalize_addr(P + i * PMD_LEVEL_MULT);

            if (pmd_large(*start) /*|| !pmd_present(*start)*/){
                note_page(st, __pgprot(prot), 3);
            } else
                walk_pte_level(st, *start,
                           P + i * PMD_LEVEL_MULT);
        }else
            note_page(st, __pgprot(0), 3);

        start++;
    }
}


static void walk_pud_level( struct pg_state *st, pgd_t addr,
                            unsigned long P)
{
    int i;
    pud_t *start;

    start = (pud_t *) pgd_page_vaddr(addr);

    for (i = 0; i < PTRS_PER_PUD; i++) {
        if (!pud_none(*start) && pud_present(*start)) {
            pgprotval_t prot = pud_val(*start) & PTE_FLAGS_MASK;
            st->current_address = normalize_addr(P + i * PUD_LEVEL_MULT);

            if (pud_large(*start) /*|| !pud_present(*start)*/){
                note_page( st, __pgprot(prot), 2);
            } else
                walk_pmd_level(st, *start,
                           P + i * PUD_LEVEL_MULT);
        } else
            note_page(st, __pgprot(0), 2);

        start++;
    }
}

void walk_pgd_level(void){

    pgd_t *start = (pgd_t *)module_init_level4_pgt;
    int i;
    struct pg_state st;

    memset(&st, 0, sizeof(st));

    for (i = 0; i < PTRS_PER_PGD; i++) {
        if (!pgd_none(*start) && pgd_present(*start)) {
            pgprotval_t prot = pgd_val(*start) & PTE_FLAGS_MASK;
            st.current_address = normalize_addr(i * PGD_LEVEL_MULT);
            if (pgd_large(*start) /*|| !pgd_present(*start)*/) {
                note_page(&st, __pgprot(prot), 1);
            } else
                walk_pud_level(&st, *start,
                           i * PGD_LEVEL_MULT);
        }else
            note_page(&st, __pgprot(0), 1);

        start++;
    }

    /* Flush out the last page */
    st.current_address = normalize_addr(PTRS_PER_PGD*PGD_LEVEL_MULT);
    note_page(&st, __pgprot(0), 0);
}

void
kernel_pagetable_walk(void *addr){
#ifdef ENABLE_PER_PROCESS_SCANNING
    struct task_struct *tsk;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    struct mm_walk walk = {0,};

    watchpoint_callback = addr;

   for (tsk = &init_task ; (tsk = next_task(tsk)) != &init_task ; ) {
        printk("task pointer : %llx\n", (unsigned long)tsk);

        if(!pid_alive(tsk))
            continue;

        if(tsk->mm){
            mm = tsk->mm;
        }else {
            mm = tsk->active_mm;
        }

        if(!mm)
            continue;

        //printk("mm struct : %llx\n", mm);

        walk.pmd_entry = pte_range_callback;
        walk.pte_entry = page_table_callback;
        walk.pte_hole = pte_hole_callback;
        walk.mm = mm;

        down_read(&mm->mmap_sem);

        for (vma = mm->mmap; vma; vma = vma->vm_next) {
            if (is_vm_hugetlb_page(vma)){
                continue;
            }

            if(module_walk_page_range){
                module_walk_page_range(vma->vm_start, vma->vm_end, &walk);
               // printk("range : start : %llx, end : %llx\n", vma->vm_start, vma->vm_end);
            }

        }

        up_read(&mm->mmap_sem);
    }
    printk("walk the kernel page table\n");
#else
    watchpoint_callback = addr;

    /* walk the kernel data structure using page table data structure*/
    walk_pgd_level();
#endif
}

#ifdef ENABLE_DEBUG
noinline void break_on_fault_address(unsigned long address){
    unsigned long level;
    pte_t *pte = lookup_address(address, &level);

    if(!pte_present(*pte)){
        printk("pte is not present\n");
    }
    printk("faulr_address : %llx\n", address);
}

void kernel_handle_fault(struct pt_regs *regs, unsigned long error_code){
    unsigned long address;
    exception_enter(regs);

    address = read_cr2();

    if(address == 0xffff88007fffd000){
        break_on_fault_address(address);
    }
    exception_exit(regs);
}
#endif
