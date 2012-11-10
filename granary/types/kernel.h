/*
 * kernel_types.hpp
 *
 *  Created on: 2012-10-01
 *      Author: pag
 *     Version: $Id$
 */

#ifndef DRK_KERNEL_TYPES_HPP_
#define DRK_KERNEL_TYPES_HPP_

#ifdef __cplusplus
namespace kernel {
extern "C" {
#endif

#define private private_
#define public public_
#define protected protected_
#define bool char

#ifdef __unused
#   undef __unused
#   define HAVE_OLD_UNUSED 0
#endif

//enum { false = 0 , true = 1 } ;

typedef __signed__ char __s8 ;

typedef unsigned char __u8 ;

typedef __signed__ short __s16 ;

typedef unsigned short __u16 ;

typedef __signed__ int __s32 ;

typedef unsigned int __u32 ;

typedef __signed__ long long __s64 ;

typedef unsigned long long __u64 ;

typedef signed char s8 ;

typedef unsigned char u8 ;

typedef signed short s16 ;

typedef unsigned short u16 ;

typedef signed int s32 ;

typedef unsigned int u32 ;

typedef signed long long s64 ;

typedef unsigned long long u64 ;

typedef unsigned short umode_t ;

typedef u64 dma64_addr_t ;

typedef u64 dma_addr_t ;

typedef struct { unsigned long fds_bits [ ( 1024 / ( 8 * sizeof ( unsigned long ) ) ) ] ; } __kernel_fd_set ;

typedef void ( * __kernel_sighandler_t ) ( int ) ;

typedef int __kernel_key_t ;

typedef int __kernel_mqd_t ;

typedef unsigned long __kernel_ino_t ;

typedef unsigned int __kernel_mode_t ;

typedef unsigned long __kernel_nlink_t ;

typedef long __kernel_off_t ;

typedef int __kernel_pid_t ;

typedef int __kernel_ipc_pid_t ;

typedef unsigned int __kernel_uid_t ;

typedef unsigned int __kernel_gid_t ;

typedef unsigned long __kernel_size_t ;

typedef long __kernel_ssize_t ;

typedef long __kernel_ptrdiff_t ;

typedef long __kernel_time_t ;

typedef long __kernel_suseconds_t ;

typedef long __kernel_clock_t ;

typedef int __kernel_timer_t ;

typedef int __kernel_clockid_t ;

typedef int __kernel_daddr_t ;

typedef char * __kernel_caddr_t ;

typedef unsigned short __kernel_uid16_t ;

typedef unsigned short __kernel_gid16_t ;

typedef long long __kernel_loff_t ;

typedef struct { int val [ 2 ] ; } __kernel_fsid_t ;

typedef unsigned short __kernel_old_uid_t ;

typedef unsigned short __kernel_old_gid_t ;

typedef __kernel_uid_t __kernel_uid32_t ;

typedef __kernel_gid_t __kernel_gid32_t ;

typedef unsigned long __kernel_old_dev_t ;

typedef __u32 __kernel_dev_t ;

typedef __kernel_fd_set fd_set ;

typedef __kernel_dev_t dev_t ;

typedef __kernel_ino_t ino_t ;

typedef __kernel_mode_t mode_t ;

typedef __kernel_nlink_t nlink_t ;

typedef __kernel_off_t off_t ;

typedef __kernel_pid_t pid_t ;

typedef __kernel_daddr_t daddr_t ;

typedef __kernel_key_t key_t ;

typedef __kernel_suseconds_t suseconds_t ;

typedef __kernel_timer_t timer_t ;

typedef __kernel_clockid_t clockid_t ;

typedef __kernel_mqd_t mqd_t ;

typedef __kernel_uid32_t uid_t ;

typedef __kernel_gid32_t gid_t ;

typedef __kernel_uid16_t uid16_t ;

typedef __kernel_gid16_t gid16_t ;

typedef unsigned long uintptr_t ;

typedef __kernel_old_uid_t old_uid_t ;

typedef __kernel_old_gid_t old_gid_t ;

typedef __kernel_loff_t loff_t ;

typedef __kernel_size_t size_t ;

typedef __kernel_ssize_t ssize_t ;

typedef __kernel_ptrdiff_t ptrdiff_t ;

typedef __kernel_time_t time_t ;

typedef __kernel_clock_t clock_t ;

typedef __kernel_caddr_t caddr_t ;

typedef unsigned char u_char ;

typedef unsigned short u_short ;

typedef unsigned int u_int ;

typedef unsigned long u_long ;

typedef unsigned char unchar ;

typedef unsigned short ushort ;

typedef unsigned long ulong ;

typedef __u8 u_int8_t ;

typedef __s8 int8_t ;

typedef __u16 u_int16_t ;

typedef __s16 int16_t ;

typedef __u32 u_int32_t ;

typedef __s32 int32_t ;

typedef __u8 uint8_t ;

typedef __u16 uint16_t ;

typedef __u32 uint32_t ;

typedef __u64 uint64_t ;

typedef __u64 u_int64_t ;

typedef __s64 int64_t ;

typedef unsigned long sector_t ;

typedef unsigned long blkcnt_t ;

typedef __u16 __le16 ;

typedef __u16 __be16 ;

typedef __u32 __le32 ;

typedef __u32 __be32 ;

typedef __u64 __le64 ;

typedef __u64 __be64 ;

typedef __u16 __sum16 ;

typedef __u32 __wsum ;

typedef unsigned gfp_t ;

typedef unsigned fmode_t ;

typedef u64 phys_addr_t ;

typedef phys_addr_t resource_size_t ;

typedef struct { volatile int counter ; } atomic_t ;

typedef struct { volatile long counter ; } atomic64_t ;

struct ustat { __kernel_daddr_t f_tfree ; __kernel_ino_t f_tinode ; char f_fname [ 6 ] ; char f_fpack [ 6 ] ; } ;

struct task_struct ;

struct mm_struct ;

struct vm86_regs { long ebx ; long ecx ; long edx ; long esi ; long edi ; long ebp ; long eax ; long __null_ds ; long __null_es ; long __null_fs ; long __null_gs ; long orig_eax ; long eip ; unsigned short cs , __csh ; long eflags ; long esp ; unsigned short ss , __ssh ; unsigned short es , __esh ; unsigned short ds , __dsh ; unsigned short fs , __fsh ; unsigned short gs , __gsh ; } ;

struct revectored_struct { unsigned long __map [ 8 ] ; } ;

struct vm86_struct { struct vm86_regs regs ; unsigned long flags ; unsigned long screen_bitmap ; unsigned long cpu_type ; struct revectored_struct int_revectored ; struct revectored_struct int21_revectored ; } ;

struct vm86plus_info_struct { unsigned long force_return_for_pic : 1 ; unsigned long vm86dbg_active : 1 ; unsigned long vm86dbg_TFpendig : 1 ; unsigned long unused : 28 ; unsigned long is_vm86pus : 1 ; unsigned char vm86dbg_intxxtab [ 32 ] ; } ;

struct vm86plus_struct { struct vm86_regs regs ; unsigned long flags ; unsigned long screen_bitmap ; unsigned long cpu_type ; struct revectored_struct int_revectored ; struct revectored_struct int21_revectored ; struct vm86plus_info_struct vm86plus ; } ;

struct ptrace_bts_config { __u32 size ; __u32 flags ; __u32 signal ; __u32 bts_size ; } ;

struct pt_regs { unsigned long r15 ; unsigned long r14 ; unsigned long r13 ; unsigned long r12 ; unsigned long bp ; unsigned long bx ; unsigned long r11 ; unsigned long r10 ; unsigned long r9 ; unsigned long r8 ; unsigned long ax ; unsigned long cx ; unsigned long dx ; unsigned long si ; unsigned long di ; unsigned long orig_ax ; unsigned long ip ; unsigned long cs ; unsigned long flags ; unsigned long sp ; unsigned long ss ; } ;

typedef int ( * initcall_t ) ( void ) ;

typedef void ( * exitcall_t ) ( void ) ;

typedef void ( * ctor_fn_t ) ( void ) ;

struct cpuinfo_x86 ;

struct user_desc ;

struct kernel_vm86_regs { struct pt_regs pt ; unsigned short es , __esh ; unsigned short ds , __dsh ; unsigned short fs , __fsh ; unsigned short gs , __gsh ; } ;

struct kernel_vm86_struct { struct kernel_vm86_regs regs ; unsigned long flags ; unsigned long screen_bitmap ; unsigned long cpu_type ; struct revectored_struct int_revectored ; struct revectored_struct int21_revectored ; struct vm86plus_info_struct vm86plus ; struct pt_regs * regs32 ; } ;

struct math_emu_info { long ___orig_eip ; union { struct pt_regs * regs ; struct kernel_vm86_regs * vm86 ; } ; } ;

struct _fpx_sw_bytes { __u32 magic1 ; __u32 extended_size ; __u64 xstate_bv ; __u32 xstate_size ; __u32 padding [ 7 ] ; } ;

struct _fpstate { __u16 cwd ; __u16 swd ; __u16 twd ; __u16 fop ; __u64 rip ; __u64 rdp ; __u32 mxcsr ; __u32 mxcsr_mask ; __u32 st_space [ 32 ] ; __u32 xmm_space [ 64 ] ; __u32 reserved2 [ 12 ] ; union { __u32 reserved3 [ 12 ] ; struct _fpx_sw_bytes sw_reserved ; } ; } ;

struct sigcontext { unsigned long r8 ; unsigned long r9 ; unsigned long r10 ; unsigned long r11 ; unsigned long r12 ; unsigned long r13 ; unsigned long r14 ; unsigned long r15 ; unsigned long di ; unsigned long si ; unsigned long bp ; unsigned long bx ; unsigned long dx ; unsigned long ax ; unsigned long cx ; unsigned long sp ; unsigned long ip ; unsigned long flags ; unsigned short cs ; unsigned short gs ; unsigned short fs ; unsigned short __pad0 ; unsigned long err ; unsigned long trapno ; unsigned long oldmask ; unsigned long cr2 ; void * fpstate ; unsigned long reserved1 [ 8 ] ; } ;

struct _xsave_hdr { __u64 xstate_bv ; __u64 reserved1 [ 2 ] ; __u64 reserved2 [ 5 ] ; } ;

struct _ymmh_state { __u32 ymmh_space [ 64 ] ; } ;

struct _xstate { struct _fpstate fpstate ; struct _xsave_hdr xstate_hdr ; struct _ymmh_state ymmh ; } ;

struct alt_instr { u8 * instr ; u8 * replacement ; u8 cpuid ; u8 instrlen ; u8 replacementlen ; u8 pad1 ; u32 pad2 ; } ;

struct module ;

struct paravirt_patch_site ;

struct ratelimit_state { int interval ; int burst ; int printed ; int missed ; unsigned long begin ; } ;

struct _ddebug { const char * modname ; const char * function ; const char * filename ; const char * format ; char primary_hash ; char secondary_hash ; unsigned int lineno : 24 ; unsigned int flags : 8 ; } __attribute__ ( ( aligned ( 8 ) ) ) ;

struct bug_entry { signed int bug_addr_disp ; signed int file_disp ; unsigned short line ; unsigned short flags ; } ;

struct completion ;

struct pt_regs ;

struct user ;

struct pid ;

enum { DUMP_PREFIX_NONE , DUMP_PREFIX_ADDRESS , DUMP_PREFIX_OFFSET } ;

struct sysinfo ;

struct sysinfo { long uptime ; unsigned long loads [ 3 ] ; unsigned long totalram ; unsigned long freeram ; unsigned long sharedram ; unsigned long bufferram ; unsigned long totalswap ; unsigned long freeswap ; unsigned short procs ; unsigned short pad ; unsigned long totalhigh ; unsigned long freehigh ; unsigned int mem_unit ; char _f [ 20 - 2 * sizeof ( long ) - sizeof ( int ) ] ; } ;

typedef unsigned long pteval_t ;

typedef unsigned long pmdval_t ;

typedef unsigned long pudval_t ;

typedef unsigned long pgdval_t ;

typedef unsigned long pgprotval_t ;

typedef struct { pteval_t pte ; } pte_t ;

typedef struct pgprot { pgprotval_t pgprot ; } pgprot_t ;

typedef struct { pgdval_t pgd ; } pgd_t ;

typedef struct { pudval_t pud ; } pud_t ;

typedef struct { pmdval_t pmd ; } pmd_t ;

typedef struct page * pgtable_t ;

struct file ;

struct seq_file ;

enum { PG_LEVEL_NONE , PG_LEVEL_4K , PG_LEVEL_2M , PG_LEVEL_1G , PG_LEVEL_NUM } ;

struct desc_struct { union { struct { unsigned int a ; unsigned int b ; } ; struct { u16 limit0 ; u16 base0 ; unsigned base1 : 8 , type : 4 , s : 1 , dpl : 2 , p : 1 ; unsigned limit : 4 , avl : 1 , l : 1 , d : 1 , g : 1 , base2 : 8 ; } ; } ; } __attribute__ ( ( packed ) ) ;

enum { GATE_INTERRUPT = 0xE , GATE_TRAP = 0xF , GATE_CALL = 0xC , GATE_TASK = 0x5 } ;

struct gate_struct64 { u16 offset_low ; u16 segment ; unsigned ist : 3 , zero0 : 5 , type : 5 , dpl : 2 , p : 1 ; u16 offset_middle ; u32 offset_high ; u32 zero1 ; } __attribute__ ( ( packed ) ) ;

enum { DESC_TSS = 0x9 , DESC_LDT = 0x2 , DESCTYPE_S = 0x10 } ;

struct ldttss_desc64 { u16 limit0 ; u16 base0 ; unsigned base1 : 8 , type : 5 , dpl : 2 , p : 1 ; unsigned limit1 : 4 , zero0 : 3 , g : 1 , base2 : 8 ; u32 base3 ; u32 zero1 ; } __attribute__ ( ( packed ) ) ;

typedef struct gate_struct64 gate_desc ;

typedef struct ldttss_desc64 ldt_desc ;

typedef struct ldttss_desc64 tss_desc ;

struct desc_ptr { unsigned short size ; unsigned long address ; } __attribute__ ( ( packed ) ) ;

enum km_type { KM_BOUNCE_READ , KM_SKB_SUNRPC_DATA , KM_SKB_DATA_SOFTIRQ , KM_USER0 , KM_USER1 , KM_BIO_SRC_IRQ , KM_BIO_DST_IRQ , KM_PTE0 , KM_PTE1 , KM_IRQ0 , KM_IRQ1 , KM_SOFTIRQ0 , KM_SOFTIRQ1 , KM_SYNC_ICACHE , KM_SYNC_DCACHE , KM_UML_USERCOPY , KM_IRQ_PTE , KM_NMI , KM_NMI_PTE , KM_TYPE_NR } ;

struct page ;

struct thread_struct ;

struct desc_ptr ;

struct tss_struct ;

struct desc_struct ;

struct cpumask ;

struct paravirt_callee_save { void * func ; } ;

struct pv_info { unsigned int kernel_rpl ; int shared_kernel_pmd ; int paravirt_enabled ; const char * name ; } ;

struct pv_init_ops { unsigned ( * patch ) ( u8 type , u16 clobber , void * insnbuf , unsigned long addr , unsigned len ) ; } ;

struct pv_lazy_ops { void ( * enter ) ( void ) ; void ( * leave ) ( void ) ; } ;

struct pv_time_ops { unsigned long long ( * sched_clock ) ( void ) ; unsigned long ( * get_tsc_khz ) ( void ) ; } ;

struct pv_cpu_ops { unsigned long ( * get_debugreg ) ( int regno ) ; void ( * set_debugreg ) ( int regno , unsigned long value ) ; void ( * clts ) ( void ) ; unsigned long ( * read_cr0 ) ( void ) ; void ( * write_cr0 ) ( unsigned long ) ; unsigned long ( * read_cr4_safe ) ( void ) ; unsigned long ( * read_cr4 ) ( void ) ; void ( * write_cr4 ) ( unsigned long ) ; unsigned long ( * read_cr8 ) ( void ) ; void ( * write_cr8 ) ( unsigned long ) ; void ( * load_tr_desc ) ( void ) ; void ( * load_gdt ) ( const struct desc_ptr * ) ; void ( * load_idt ) ( const struct desc_ptr * ) ; void ( * store_gdt ) ( struct desc_ptr * ) ; void ( * store_idt ) ( struct desc_ptr * ) ; void ( * set_ldt ) ( const void * desc , unsigned entries ) ; unsigned long ( * store_tr ) ( void ) ; void ( * load_tls ) ( struct thread_struct * t , unsigned int cpu ) ; void ( * load_gs_index ) ( unsigned int idx ) ; void ( * write_ldt_entry ) ( struct desc_struct * ldt , int entrynum , const void * desc ) ; void ( * write_gdt_entry ) ( struct desc_struct * , int entrynum , const void * desc , int size ) ; void ( * write_idt_entry ) ( gate_desc * , int entrynum , const gate_desc * gate ) ; void ( * alloc_ldt ) ( struct desc_struct * ldt , unsigned entries ) ; void ( * free_ldt ) ( struct desc_struct * ldt , unsigned entries ) ; void ( * load_sp0 ) ( struct tss_struct * tss , struct thread_struct * t ) ; void ( * set_iopl_mask ) ( unsigned mask ) ; void ( * wbinvd ) ( void ) ; void ( * io_delay ) ( void ) ; void ( * cpuid ) ( unsigned int * eax , unsigned int * ebx , unsigned int * ecx , unsigned int * edx ) ; u64 ( * read_msr ) ( unsigned int msr , int * err ) ; int ( * rdmsr_regs ) ( u32 * regs ) ; int ( * write_msr ) ( unsigned int msr , unsigned low , unsigned high ) ; int ( * wrmsr_regs ) ( u32 * regs ) ; u64 ( * read_tsc ) ( void ) ; u64 ( * read_pmc ) ( int counter ) ; unsigned long long ( * read_tscp ) ( unsigned int * aux ) ; void ( * irq_enable_sysexit ) ( void ) ; void ( * usergs_sysret64 ) ( void ) ; void ( * usergs_sysret32 ) ( void ) ; void ( * iret ) ( void ) ; void ( * swapgs ) ( void ) ; void ( * start_context_switch ) ( struct task_struct * prev ) ; void ( * end_context_switch ) ( struct task_struct * next ) ; } ;

struct pv_irq_ops { struct paravirt_callee_save save_fl ; struct paravirt_callee_save restore_fl ; struct paravirt_callee_save irq_disable ; struct paravirt_callee_save irq_enable ; void ( * safe_halt ) ( void ) ; void ( * halt ) ( void ) ; void ( * adjust_exception_frame ) ( void ) ; } ;

struct pv_apic_ops { void ( * startup_ipi_hook ) ( int phys_apicid , unsigned long start_eip , unsigned long start_esp ) ; } ;

struct pv_mmu_ops { unsigned long ( * read_cr2 ) ( void ) ; void ( * write_cr2 ) ( unsigned long ) ; unsigned long ( * read_cr3 ) ( void ) ; void ( * write_cr3 ) ( unsigned long ) ; void ( * activate_mm ) ( struct mm_struct * prev , struct mm_struct * next ) ; void ( * dup_mmap ) ( struct mm_struct * oldmm , struct mm_struct * mm ) ; void ( * exit_mmap ) ( struct mm_struct * mm ) ; void ( * flush_tlb_user ) ( void ) ; void ( * flush_tlb_kernel ) ( void ) ; void ( * flush_tlb_single ) ( unsigned long addr ) ; void ( * flush_tlb_others ) ( const struct cpumask * cpus , struct mm_struct * mm , unsigned long va ) ; int ( * pgd_alloc ) ( struct mm_struct * mm ) ; void ( * pgd_free ) ( struct mm_struct * mm , pgd_t * pgd ) ; void ( * alloc_pte ) ( struct mm_struct * mm , unsigned long pfn ) ; void ( * alloc_pmd ) ( struct mm_struct * mm , unsigned long pfn ) ; void ( * alloc_pmd_clone ) ( unsigned long pfn , unsigned long clonepfn , unsigned long start , unsigned long count ) ; void ( * alloc_pud ) ( struct mm_struct * mm , unsigned long pfn ) ; void ( * release_pte ) ( unsigned long pfn ) ; void ( * release_pmd ) ( unsigned long pfn ) ; void ( * release_pud ) ( unsigned long pfn ) ; void ( * set_pte ) ( pte_t * ptep , pte_t pteval ) ; void ( * set_pte_at ) ( struct mm_struct * mm , unsigned long addr , pte_t * ptep , pte_t pteval ) ; void ( * set_pmd ) ( pmd_t * pmdp , pmd_t pmdval ) ; void ( * pte_update ) ( struct mm_struct * mm , unsigned long addr , pte_t * ptep ) ; void ( * pte_update_defer ) ( struct mm_struct * mm , unsigned long addr , pte_t * ptep ) ; pte_t ( * ptep_modify_prot_start ) ( struct mm_struct * mm , unsigned long addr , pte_t * ptep ) ; void ( * ptep_modify_prot_commit ) ( struct mm_struct * mm , unsigned long addr , pte_t * ptep , pte_t pte ) ; struct paravirt_callee_save pte_val ; struct paravirt_callee_save make_pte ; struct paravirt_callee_save pgd_val ; struct paravirt_callee_save make_pgd ; void ( * set_pud ) ( pud_t * pudp , pud_t pudval ) ; struct paravirt_callee_save pmd_val ; struct paravirt_callee_save make_pmd ; struct paravirt_callee_save pud_val ; struct paravirt_callee_save make_pud ; void ( * set_pgd ) ( pgd_t * pudp , pgd_t pgdval ) ; struct pv_lazy_ops lazy_mode ; void ( * set_fixmap ) ( unsigned idx , phys_addr_t phys , pgprot_t flags ) ; } ;

struct raw_spinlock ;

struct pv_lock_ops { int ( * spin_is_locked ) ( struct raw_spinlock * lock ) ; int ( * spin_is_contended ) ( struct raw_spinlock * lock ) ; void ( * spin_lock ) ( struct raw_spinlock * lock ) ; void ( * spin_lock_flags ) ( struct raw_spinlock * lock , unsigned long flags ) ; int ( * spin_trylock ) ( struct raw_spinlock * lock ) ; void ( * spin_unlock ) ( struct raw_spinlock * lock ) ; } ;

struct paravirt_patch_template { struct pv_init_ops pv_init_ops ; struct pv_time_ops pv_time_ops ; struct pv_cpu_ops pv_cpu_ops ; struct pv_irq_ops pv_irq_ops ; struct pv_apic_ops pv_apic_ops ; struct pv_mmu_ops pv_mmu_ops ; struct pv_lock_ops pv_lock_ops ; } ;

enum paravirt_lazy_mode { PARAVIRT_LAZY_NONE , PARAVIRT_LAZY_MMU , PARAVIRT_LAZY_CPU } ;

struct paravirt_patch_site { u8 * instr ; u8 instrtype ; u8 len ; u16 clobbers ; } ;

typedef struct cpumask { unsigned long bits [ ( ( ( 64 ) + ( 8 * sizeof ( long ) ) - 1 ) / ( 8 * sizeof ( long ) ) ) ] ; } cpumask_t ;

typedef struct cpumask cpumask_var_t [ 1 ] ;

struct msr { union { struct { u32 l ; u32 h ; } ; u64 q ; } ; } ;

struct msr_info { u32 msr_no ; struct msr reg ; struct msr * msrs ; int err ; } ;

struct msr_regs_info { u32 * regs ; int err ; } ;

struct exec_domain ;

enum { ADDR_NO_RANDOMIZE = 0x0040000 , FDPIC_FUNCPTRS = 0x0080000 , MMAP_PAGE_ZERO = 0x0100000 , ADDR_COMPAT_LAYOUT = 0x0200000 , READ_IMPLIES_EXEC = 0x0400000 , ADDR_LIMIT_32BIT = 0x0800000 , SHORT_INODE = 0x1000000 , WHOLE_SECONDS = 0x2000000 , STICKY_TIMEOUTS = 0x4000000 , ADDR_LIMIT_3GB = 0x8000000 } ;

enum { PER_LINUX = 0x0000 , PER_LINUX_32BIT = 0x0000 | ADDR_LIMIT_32BIT , PER_LINUX_FDPIC = 0x0000 | FDPIC_FUNCPTRS , PER_SVR4 = 0x0001 | STICKY_TIMEOUTS | MMAP_PAGE_ZERO , PER_SVR3 = 0x0002 | STICKY_TIMEOUTS | SHORT_INODE , PER_SCOSVR3 = 0x0003 | STICKY_TIMEOUTS | WHOLE_SECONDS | SHORT_INODE , PER_OSR5 = 0x0003 | STICKY_TIMEOUTS | WHOLE_SECONDS , PER_WYSEV386 = 0x0004 | STICKY_TIMEOUTS | SHORT_INODE , PER_ISCR4 = 0x0005 | STICKY_TIMEOUTS , PER_BSD = 0x0006 , PER_SUNOS = 0x0006 | STICKY_TIMEOUTS , PER_XENIX = 0x0007 | STICKY_TIMEOUTS | SHORT_INODE , PER_LINUX32 = 0x0008 , PER_LINUX32_3GB = 0x0008 | ADDR_LIMIT_3GB , PER_IRIX32 = 0x0009 | STICKY_TIMEOUTS , PER_IRIXN32 = 0x000a | STICKY_TIMEOUTS , PER_IRIX64 = 0x000b | STICKY_TIMEOUTS , PER_RISCOS = 0x000c , PER_SOLARIS = 0x000d | STICKY_TIMEOUTS , PER_UW7 = 0x000e | STICKY_TIMEOUTS | MMAP_PAGE_ZERO , PER_OSF4 = 0x000f , PER_HPUX = 0x0010 , PER_MASK = 0x00ff } ;

typedef void ( * handler_t ) ( int , struct pt_regs * ) ;

struct exec_domain { const char * name ; handler_t handler ; unsigned char pers_low ; unsigned char pers_high ; unsigned long * signal_map ; unsigned long * signal_invmap ; struct map_segment * err_map ; struct map_segment * socktype_map ; struct map_segment * sockopt_map ; struct map_segment * af_map ; struct module * module ; struct exec_domain * next ; } ;

struct cpuinfo_x86 { __u8 x86 ; __u8 x86_vendor ; __u8 x86_model ; __u8 x86_mask ; int x86_tlbsize ; __u8 x86_virt_bits ; __u8 x86_phys_bits ; __u8 x86_coreid_bits ; __u32 extended_cpuid_level ; int cpuid_level ; __u32 x86_capability [ 9 ] ; char x86_vendor_id [ 16 ] ; char x86_model_id [ 64 ] ; int x86_cache_size ; int x86_cache_alignment ; int x86_power ; unsigned long loops_per_jiffy ; cpumask_var_t llc_shared_map ; u16 x86_max_cores ; u16 apicid ; u16 initial_apicid ; u16 x86_clflush_size ; u16 booted_cores ; u16 phys_proc_id ; u16 cpu_core_id ; u16 cpu_index ; unsigned int x86_hyper_vendor ; } __attribute__ ( ( __aligned__ ( ( 1 << ( 6 ) ) ) ) ) ;

struct x86_hw_tss { u32 reserved1 ; u64 sp0 ; u64 sp1 ; u64 sp2 ; u64 reserved2 ; u64 ist [ 7 ] ; u32 reserved3 ; u32 reserved4 ; u16 reserved5 ; u16 io_bitmap_base ; } __attribute__ ( ( packed ) ) __attribute__ ( ( __aligned__ ( ( 1 << ( 6 ) ) ) ) ) ;

struct tss_struct { struct x86_hw_tss x86_tss ; unsigned long io_bitmap [ ( ( 65536 / 8 ) / sizeof ( long ) ) + 1 ] ; unsigned long stack [ 64 ] ; } __attribute__ ( ( __aligned__ ( ( 1 << ( 6 ) ) ) ) ) ;

struct orig_ist { unsigned long ist [ 7 ] ; } ;

struct i387_fsave_struct { u32 cwd ; u32 swd ; u32 twd ; u32 fip ; u32 fcs ; u32 foo ; u32 fos ; u32 st_space [ 20 ] ; u32 status ; } ;

struct i387_fxsave_struct { u16 cwd ; u16 swd ; u16 twd ; u16 fop ; union { struct { u64 rip ; u64 rdp ; } ; struct { u32 fip ; u32 fcs ; u32 foo ; u32 fos ; } ; } ; u32 mxcsr ; u32 mxcsr_mask ; u32 st_space [ 32 ] ; u32 xmm_space [ 64 ] ; u32 padding [ 12 ] ; union { u32 padding1 [ 12 ] ; u32 sw_reserved [ 12 ] ; } ; } __attribute__ ( ( aligned ( 16 ) ) ) ;

struct i387_soft_struct { u32 cwd ; u32 swd ; u32 twd ; u32 fip ; u32 fcs ; u32 foo ; u32 fos ; u32 st_space [ 20 ] ; u8 ftop ; u8 changed ; u8 lookahead ; u8 no_update ; u8 rm ; u8 alimit ; struct math_emu_info * info ; u32 entry_eip ; } ;

struct ymmh_struct { u32 ymmh_space [ 64 ] ; } ;

struct xsave_hdr_struct { u64 xstate_bv ; u64 reserved1 [ 2 ] ; u64 reserved2 [ 5 ] ; } __attribute__ ( ( packed ) ) ;

struct xsave_struct { struct i387_fxsave_struct i387 ; struct xsave_hdr_struct xsave_hdr ; struct ymmh_struct ymmh ; } __attribute__ ( ( packed , aligned ( 64 ) ) ) ;

/*union type*/

union thread_xstate { struct i387_fsave_struct fsave ; struct i387_fxsave_struct fxsave ; struct i387_soft_struct soft ; struct xsave_struct xsave ; } ;

union irq_stack_union { char irq_stack [ ( ( ( 1UL ) << 12 ) << 2 ) ] ; struct { char gs_base [ 40 ] ; unsigned long stack_canary ; } ; } ;

struct thread_struct { struct desc_struct tls_array [ 3 ] ; unsigned long sp0 ; unsigned long sp ; unsigned long usersp ; unsigned short es ; unsigned short ds ; unsigned short fsindex ; unsigned short gsindex ; unsigned long fs ; unsigned long gs ; unsigned long debugreg0 ; unsigned long debugreg1 ; unsigned long debugreg2 ; unsigned long debugreg3 ; unsigned long debugreg6 ; unsigned long debugreg7 ; unsigned long cr2 ; unsigned long trap_no ; unsigned long error_code ; union thread_xstate * xstate ; unsigned long * io_bitmap_ptr ; unsigned long iopl ; unsigned io_bitmap_max ; unsigned long debugctlmsr ; struct ds_context * ds_ctx ; } ;

typedef struct { unsigned long seg ; } mm_segment_t ;

struct aperfmperf { u64 aperf , mperf ; } ;

struct list_head { struct list_head * next , * prev ; } ;

struct hlist_head { struct hlist_node * first ; } ;

struct hlist_node { struct hlist_node * next , * * pprev ; } ;

struct stat { unsigned long st_dev ; unsigned long st_ino ; unsigned long st_nlink ; unsigned int st_mode ; unsigned int st_uid ; unsigned int st_gid ; unsigned int __pad0 ; unsigned long st_rdev ; long st_size ; long st_blksize ; long st_blocks ; unsigned long st_atime ; unsigned long st_atime_nsec ; unsigned long st_mtime ; unsigned long st_mtime_nsec ; unsigned long st_ctime ; unsigned long st_ctime_nsec ; long __unused [ 3 ] ; } ;

struct __old_kernel_stat { unsigned short st_dev ; unsigned short st_ino ; unsigned short st_mode ; unsigned short st_nlink ; unsigned short st_uid ; unsigned short st_gid ; unsigned short st_rdev ; unsigned int st_size ; unsigned int st_atime ; unsigned int st_mtime ; unsigned int st_ctime ; } ;

struct timespec ;

struct compat_timespec ;

struct restart_block { long ( * fn ) ( struct restart_block * ) ; union { struct { unsigned long arg0 , arg1 , arg2 , arg3 ; } ; struct { u32 * uaddr ; u32 val ; u32 flags ; u32 bitset ; u64 time ; u32 * uaddr2 ; } futex ; struct { clockid_t index ; struct timespec * rmtp ; struct compat_timespec * compat_rmtp ; u64 expires ; } nanosleep ; struct { struct pollfd * ufds ; int nfds ; int has_timeout ; unsigned long tv_sec ; unsigned long tv_nsec ; } poll ; } ; } ;

struct dyn_arch_ftrace { } ;

typedef atomic64_t atomic_long_t ;

typedef struct { unsigned long int error_code ; unsigned char * xip ; unsigned long int cs ; unsigned long int xflags ; unsigned long int xsp ; unsigned long int ss ; } interrupt_stack_frame ;

struct client_extension { void * return_address_stack [ 16 ] ; unsigned return_stack_size ; void * exit_address ; void ( * iret_handler ) ( void ) ; interrupt_stack_frame pending ; } ;

struct thread_info { struct task_struct * task ; struct exec_domain * exec_domain ; __u32 flags ; __u32 status ; __u32 cpu ; int preempt_count ; mm_segment_t addr_limit ; struct restart_block restart_block ; void * sysenter_return ; int uaccess_err ; struct client_extension client_data ; } ;

struct preempt_notifier ;

struct preempt_ops { void ( * sched_in ) ( struct preempt_notifier * notifier , int cpu ) ; void ( * sched_out ) ( struct preempt_notifier * notifier , struct task_struct * next ) ; } ;

struct preempt_notifier { struct hlist_node link ; struct preempt_ops * ops ; } ;

typedef struct raw_spinlock { unsigned int slock ; } raw_spinlock_t ;

typedef struct { unsigned int lock ; } raw_rwlock_t ;

struct lockdep_map ;

struct lock_class_key { } ;

typedef struct { raw_spinlock_t raw_lock ; } spinlock_t ;

typedef struct { raw_rwlock_t raw_lock ; } rwlock_t ;

typedef struct { unsigned sequence ; spinlock_t lock ; } seqlock_t ;

typedef struct seqcount { unsigned sequence ; } seqcount_t ;

struct timespec { __kernel_time_t tv_sec ; long tv_nsec ; } ;

struct timeval { __kernel_time_t tv_sec ; __kernel_suseconds_t tv_usec ; } ;

struct itimerval ;

struct tms ;

struct tm { int tm_sec ; int tm_min ; int tm_hour ; int tm_mday ; int tm_mon ; long tm_year ; int tm_wday ; int tm_yday ; } ;

struct itimerspec { struct timespec it_interval ; struct timespec it_value ; } ;

struct kstat { u64 ino ; dev_t dev ; umode_t mode ; unsigned int nlink ; uid_t uid ; gid_t gid ; dev_t rdev ; loff_t size ; struct timespec atime ; struct timespec mtime ; struct timespec ctime ; unsigned long blksize ; unsigned long long blocks ; } ;

typedef struct __wait_queue wait_queue_t ;

typedef int ( * wait_queue_func_t ) ( wait_queue_t * wait , unsigned mode , int flags , void * key ) ;

struct __wait_queue { unsigned int flags ; void * private ; wait_queue_func_t func ; struct list_head task_list ; } ;

struct wait_bit_key { void * flags ; int bit_nr ; } ;

struct wait_bit_queue { struct wait_bit_key key ; wait_queue_t wait ; } ;

struct __wait_queue_head { spinlock_t lock ; struct list_head task_list ; } ;

typedef struct __wait_queue_head wait_queue_head_t ;

typedef struct { unsigned long bits [ ( ( ( ( 1 << 6 ) ) + ( 8 * sizeof ( long ) ) - 1 ) / ( 8 * sizeof ( long ) ) ) ] ; } nodemask_t ;

enum node_states { N_POSSIBLE , N_ONLINE , N_NORMAL_MEMORY , N_HIGH_MEMORY = N_NORMAL_MEMORY , N_CPU , NR_NODE_STATES } ;

struct nodemask_scratch { nodemask_t mask1 ; nodemask_t mask2 ; } ;

enum pageblock_bits { PB_migrate , PB_migrate_end = PB_migrate + 3 - 1 , NR_PAGEBLOCK_BITS } ;

struct free_area { struct list_head free_list [ 5 ] ; unsigned long nr_free ; } ;

struct pglist_data ;

struct zone_padding { char x [ 0 ] ; } __attribute__ ( ( __aligned__ ( 1 << ( ( 6 ) ) ) ) ) ;

enum zone_stat_item { NR_FREE_PAGES , NR_LRU_BASE , NR_INACTIVE_ANON = NR_LRU_BASE , NR_ACTIVE_ANON , NR_INACTIVE_FILE , NR_ACTIVE_FILE , NR_UNEVICTABLE , NR_MLOCK , NR_ANON_PAGES , NR_FILE_MAPPED , NR_FILE_PAGES , NR_FILE_DIRTY , NR_WRITEBACK , NR_SLAB_RECLAIMABLE , NR_SLAB_UNRECLAIMABLE , NR_PAGETABLE , NR_KERNEL_STACK , NR_UNSTABLE_NFS , NR_BOUNCE , NR_VMSCAN_WRITE , NR_WRITEBACK_TEMP , NR_ISOLATED_ANON , NR_ISOLATED_FILE , NR_SHMEM , NUMA_HIT , NUMA_MISS , NUMA_FOREIGN , NUMA_INTERLEAVE_HIT , NUMA_LOCAL , NUMA_OTHER , NR_VM_ZONE_STAT_ITEMS } ;

enum lru_list { LRU_INACTIVE_ANON = 0 , LRU_ACTIVE_ANON = 0 + 1 , LRU_INACTIVE_FILE = 0 + 2 , LRU_ACTIVE_FILE = 0 + 2 + 1 , LRU_UNEVICTABLE , NR_LRU_LISTS } ;

enum zone_watermarks { WMARK_MIN , WMARK_LOW , WMARK_HIGH , NR_WMARK } ;

struct per_cpu_pages { int count ; int high ; int batch ; struct list_head lists [ 3 ] ; } ;

struct per_cpu_pageset { struct per_cpu_pages pcp ; s8 expire ; s8 stat_threshold ; s8 vm_stat_diff [ NR_VM_ZONE_STAT_ITEMS ] ; } __attribute__ ( ( __aligned__ ( ( 1 << ( 6 ) ) ) ) ) ;

enum zone_type { ZONE_DMA , ZONE_DMA32 , ZONE_NORMAL , ZONE_MOVABLE , __MAX_NR_ZONES } ;

struct zone_reclaim_stat { unsigned long recent_rotated [ 2 ] ; unsigned long recent_scanned [ 2 ] ; unsigned long nr_saved_scan [ NR_LRU_LISTS ] ; } ;

struct zone { unsigned long watermark [ NR_WMARK ] ; unsigned long lowmem_reserve [ 4 ] ; int node ; unsigned long min_unmapped_pages ; unsigned long min_slab_pages ; struct per_cpu_pageset * pageset [ 64 ] ; spinlock_t lock ; struct free_area free_area [ 11 ] ; struct zone_padding _pad1_ ; spinlock_t lru_lock ; struct zone_lru { struct list_head list ; } lru [ NR_LRU_LISTS ] ; struct zone_reclaim_stat reclaim_stat ; unsigned long pages_scanned ; unsigned long flags ; atomic_long_t vm_stat [ NR_VM_ZONE_STAT_ITEMS ] ; int prev_priority ; unsigned int inactive_ratio ; struct zone_padding _pad2_ ; wait_queue_head_t * wait_table ; unsigned long wait_table_hash_nr_entries ; unsigned long wait_table_bits ; struct pglist_data * zone_pgdat ; unsigned long zone_start_pfn ; unsigned long spanned_pages ; unsigned long present_pages ; const char * name ; } __attribute__ ( ( __aligned__ ( 1 << ( ( 6 ) ) ) ) ) ;

typedef enum { ZONE_ALL_UNRECLAIMABLE , ZONE_RECLAIM_LOCKED , ZONE_OOM_LOCKED } zone_flags_t ;

struct zonelist_cache { unsigned short z_to_n [ ( ( 1 << 6 ) * 4 ) ] ; unsigned long fullzones [ ( ( ( ( ( 1 << 6 ) * 4 ) ) + ( 8 * sizeof ( long ) ) - 1 ) / ( 8 * sizeof ( long ) ) ) ] ; unsigned long last_full_zap ; } ;

struct zoneref { struct zone * zone ; int zone_idx ; } ;

struct zonelist { struct zonelist_cache * zlcache_ptr ; struct zoneref _zonerefs [ ( ( 1 << 6 ) * 4 ) + 1 ] ; struct zonelist_cache zlcache ; } ;

struct node_active_region { unsigned long start_pfn ; unsigned long end_pfn ; int nid ; } ;

struct bootmem_data ;

typedef struct pglist_data { struct zone node_zones [ 4 ] ; struct zonelist node_zonelists [ 2 ] ; int nr_zones ; struct bootmem_data * bdata ; unsigned long node_start_pfn ; unsigned long node_present_pages ; unsigned long node_spanned_pages ; int node_id ; wait_queue_head_t kswapd_wait ; struct task_struct * kswapd ; int kswapd_max_order ; } pg_data_t ;

struct mutex { atomic_t count ; spinlock_t wait_lock ; struct list_head wait_list ; struct thread_info * owner ; } ;

struct mutex_waiter { struct list_head list ; struct task_struct * task ; } ;

struct rw_semaphore ;

struct rwsem_waiter ;

typedef signed long rwsem_count_t ;

struct rw_semaphore { rwsem_count_t count ; spinlock_t wait_lock ; struct list_head wait_list ; } ;

struct srcu_struct_array { int c [ 2 ] ; } ;

struct srcu_struct { int completed ; struct srcu_struct_array * per_cpu_ref ; struct mutex mutex ; } ;

struct notifier_block { int ( * notifier_call ) ( struct notifier_block * , unsigned long , void * ) ; struct notifier_block * next ; int priority ; } ;

struct atomic_notifier_head { spinlock_t lock ; struct notifier_block * head ; } ;

struct blocking_notifier_head { struct rw_semaphore rwsem ; struct notifier_block * head ; } ;

struct raw_notifier_head { struct notifier_block * head ; } ;

struct srcu_notifier_head { struct mutex mutex ; struct srcu_struct srcu ; struct notifier_block * head ; } ;

struct zone ;

struct mem_section ;

enum memmap_context { MEMMAP_EARLY , MEMMAP_HOTPLUG } ;

struct ctl_table ;

struct mpf_intel { char signature [ 4 ] ; unsigned int physptr ; unsigned char length ; unsigned char specification ; unsigned char checksum ; unsigned char feature1 ; unsigned char feature2 ; unsigned char feature3 ; unsigned char feature4 ; unsigned char feature5 ; } ;

struct mpc_table { char signature [ 4 ] ; unsigned short length ; char spec ; char checksum ; char oem [ 8 ] ; char productid [ 12 ] ; unsigned int oemptr ; unsigned short oemsize ; unsigned short oemcount ; unsigned int lapic ; unsigned int reserved ; } ;

struct mpc_cpu { unsigned char type ; unsigned char apicid ; unsigned char apicver ; unsigned char cpuflag ; unsigned int cpufeature ; unsigned int featureflag ; unsigned int reserved [ 2 ] ; } ;

struct mpc_bus { unsigned char type ; unsigned char busid ; unsigned char bustype [ 6 ] ; } ;

struct mpc_ioapic { unsigned char type ; unsigned char apicid ; unsigned char apicver ; unsigned char flags ; unsigned int apicaddr ; } ;

struct mpc_intsrc { unsigned char type ; unsigned char irqtype ; unsigned short irqflag ; unsigned char srcbus ; unsigned char srcbusirq ; unsigned char dstapic ; unsigned char dstirq ; } ;

enum mp_irq_source_types { mp_INT = 0 , mp_NMI = 1 , mp_SMI = 2 , mp_ExtINT = 3 } ;

struct mpc_lintsrc { unsigned char type ; unsigned char irqtype ; unsigned short irqflag ; unsigned char srcbusid ; unsigned char srcbusirq ; unsigned char destapic ; unsigned char destapiclint ; } ;

struct mpc_oemtable { char signature [ 4 ] ; unsigned short length ; char rev ; char checksum ; char mpc [ 8 ] ; } ;

enum mp_bustype { MP_BUS_ISA = 1 , MP_BUS_EISA , MP_BUS_PCI , MP_BUS_MCA } ;

struct screen_info { __u8 orig_x ; __u8 orig_y ; __u16 ext_mem_k ; __u16 orig_video_page ; __u8 orig_video_mode ; __u8 orig_video_cols ; __u16 unused2 ; __u16 orig_video_ega_bx ; __u16 unused3 ; __u8 orig_video_lines ; __u8 orig_video_isVGA ; __u16 orig_video_points ; __u16 lfb_width ; __u16 lfb_height ; __u16 lfb_depth ; __u32 lfb_base ; __u32 lfb_size ; __u16 cl_magic , cl_offset ; __u16 lfb_linelength ; __u8 red_size ; __u8 red_pos ; __u8 green_size ; __u8 green_pos ; __u8 blue_size ; __u8 blue_pos ; __u8 rsvd_size ; __u8 rsvd_pos ; __u16 vesapm_seg ; __u16 vesapm_off ; __u16 pages ; __u16 vesa_attributes ; __u32 capabilities ; __u8 _reserved [ 6 ] ; } __attribute__ ( ( packed ) ) ;

typedef unsigned short apm_event_t ;

typedef unsigned short apm_eventinfo_t ;

struct apm_bios_info { __u16 version ; __u16 cseg ; __u32 offset ; __u16 cseg_16 ; __u16 dseg ; __u16 flags ; __u16 cseg_len ; __u16 cseg_16_len ; __u16 dseg_len ; } ;

struct apm_info { struct apm_bios_info bios ; unsigned short connection_version ; int get_power_status_broken ; int get_power_status_swabinminutes ; int allow_ints ; int forbid_idle ; int realmode_power_off ; int disabled ; } ;

struct edd_device_params { __u16 length ; __u16 info_flags ; __u32 num_default_cylinders ; __u32 num_default_heads ; __u32 sectors_per_track ; __u64 number_of_sectors ; __u16 bytes_per_sector ; __u32 dpte_ptr ; __u16 key ; __u8 device_path_info_length ; __u8 reserved2 ; __u16 reserved3 ; __u8 host_bus_type [ 4 ] ; __u8 interface_type [ 8 ] ; union { struct { __u16 base_address ; __u16 reserved1 ; __u32 reserved2 ; } __attribute__ ( ( packed ) ) isa ; struct { __u8 bus ; __u8 slot ; __u8 function ; __u8 channel ; __u32 reserved ; } __attribute__ ( ( packed ) ) pci ; struct { __u64 reserved ; } __attribute__ ( ( packed ) ) ibnd ; struct { __u64 reserved ; } __attribute__ ( ( packed ) ) xprs ; struct { __u64 reserved ; } __attribute__ ( ( packed ) ) htpt ; struct { __u64 reserved ; } __attribute__ ( ( packed ) ) unknown ; } interface_path ; union { struct { __u8 device ; __u8 reserved1 ; __u16 reserved2 ; __u32 reserved3 ; __u64 reserved4 ; } __attribute__ ( ( packed ) ) ata ; struct { __u8 device ; __u8 lun ; __u8 reserved1 ; __u8 reserved2 ; __u32 reserved3 ; __u64 reserved4 ; } __attribute__ ( ( packed ) ) atapi ; struct { __u16 id ; __u64 lun ; __u16 reserved1 ; __u32 reserved2 ; } __attribute__ ( ( packed ) ) scsi ; struct { __u64 serial_number ; __u64 reserved ; } __attribute__ ( ( packed ) ) usb ; struct { __u64 eui ; __u64 reserved ; } __attribute__ ( ( packed ) ) i1394 ; struct { __u64 wwid ; __u64 lun ; } __attribute__ ( ( packed ) ) fibre ; struct { __u64 identity_tag ; __u64 reserved ; } __attribute__ ( ( packed ) ) i2o ; struct { __u32 array_number ; __u32 reserved1 ; __u64 reserved2 ; } __attribute__ ( ( packed ) ) raid ; struct { __u8 device ; __u8 reserved1 ; __u16 reserved2 ; __u32 reserved3 ; __u64 reserved4 ; } __attribute__ ( ( packed ) ) sata ; struct { __u64 reserved1 ; __u64 reserved2 ; } __attribute__ ( ( packed ) ) unknown ; } device_path ; __u8 reserved4 ; __u8 checksum ; } __attribute__ ( ( packed ) ) ;

struct edd_info { __u8 device ; __u8 version ; __u16 interface_support ; __u16 legacy_max_cylinder ; __u8 legacy_max_head ; __u8 legacy_sectors_per_track ; struct edd_device_params params ; } __attribute__ ( ( packed ) ) ;

struct edd { unsigned int mbr_signature [ 16 ] ; struct edd_info edd_info [ 6 ] ; unsigned char mbr_signature_nr ; unsigned char edd_info_nr ; } ;

struct e820entry { __u64 addr ; __u64 size ; __u32 type ; } __attribute__ ( ( packed ) ) ;

struct e820map { __u32 nr_map ; struct e820entry map [ ( 128 + 3 * ( 1 << 6 ) ) ] ; } ;

struct setup_data ;

struct resource { resource_size_t start ; resource_size_t end ; const char * name ; unsigned long flags ; struct resource * parent , * sibling , * child ; } ;

struct resource_list { struct resource_list * next ; struct resource * res ; struct pci_dev * dev ; } ;

struct device ;

struct ist_info { __u32 signature ; __u32 command ; __u32 event ; __u32 perf_level ; } ;

struct edid_info { unsigned char dummy [ 128 ] ; } ;

struct setup_data { __u64 next ; __u32 type ; __u32 len ; __u8 data [ 0 ] ; } ;

struct setup_header { __u8 setup_sects ; __u16 root_flags ; __u32 syssize ; __u16 ram_size ; __u16 vid_mode ; __u16 root_dev ; __u16 boot_flag ; __u16 jump ; __u32 header ; __u16 version ; __u32 realmode_swtch ; __u16 start_sys ; __u16 kernel_version ; __u8 type_of_loader ; __u8 loadflags ; __u16 setup_move_size ; __u32 code32_start ; __u32 ramdisk_image ; __u32 ramdisk_size ; __u32 bootsect_kludge ; __u16 heap_end_ptr ; __u8 ext_loader_ver ; __u8 ext_loader_type ; __u32 cmd_line_ptr ; __u32 initrd_addr_max ; __u32 kernel_alignment ; __u8 relocatable_kernel ; __u8 _pad2 [ 3 ] ; __u32 cmdline_size ; __u32 hardware_subarch ; __u64 hardware_subarch_data ; __u32 payload_offset ; __u32 payload_length ; __u64 setup_data ; } __attribute__ ( ( packed ) ) ;

struct sys_desc_table { __u16 length ; __u8 table [ 14 ] ; } ;

struct efi_info { __u32 efi_loader_signature ; __u32 efi_systab ; __u32 efi_memdesc_size ; __u32 efi_memdesc_version ; __u32 efi_memmap ; __u32 efi_memmap_size ; __u32 efi_systab_hi ; __u32 efi_memmap_hi ; } ;

struct boot_params { struct screen_info screen_info ; struct apm_bios_info apm_bios_info ; __u8 _pad2 [ 4 ] ; __u64 tboot_addr ; struct ist_info ist_info ; __u8 _pad3 [ 16 ] ; __u8 hd0_info [ 16 ] ; __u8 hd1_info [ 16 ] ; struct sys_desc_table sys_desc_table ; __u8 _pad4 [ 144 ] ; struct edid_info edid_info ; struct efi_info efi_info ; __u32 alt_mem_k ; __u32 scratch ; __u8 e820_entries ; __u8 eddbuf_entries ; __u8 edd_mbr_sig_buf_entries ; __u8 _pad6 [ 6 ] ; struct setup_header hdr ; __u8 _pad7 [ 0x290 - 0x1f1 - sizeof ( struct setup_header ) ] ; __u32 edd_mbr_sig_buffer [ 16 ] ; struct e820entry e820_map [ 128 ] ; __u8 _pad8 [ 48 ] ; struct edd_info eddbuf [ 6 ] ; __u8 _pad9 [ 276 ] ; } __attribute__ ( ( packed ) ) ;

enum { X86_SUBARCH_PC = 0 , X86_SUBARCH_LGUEST , X86_SUBARCH_XEN , X86_SUBARCH_MRST , X86_NR_SUBARCHS } ;

struct mpc_bus ;

struct mpc_cpu ;

struct mpc_table ;

struct x86_init_mpparse { void ( * mpc_record ) ( unsigned int mode ) ; void ( * setup_ioapic_ids ) ( void ) ; int ( * mpc_apic_id ) ( struct mpc_cpu * m ) ; void ( * smp_read_mpc_oem ) ( struct mpc_table * mpc ) ; void ( * mpc_oem_pci_bus ) ( struct mpc_bus * m ) ; void ( * mpc_oem_bus_info ) ( struct mpc_bus * m , char * name ) ; void ( * find_smp_config ) ( unsigned int reserve ) ; void ( * get_smp_config ) ( unsigned int early ) ; } ;

struct x86_init_resources { void ( * probe_roms ) ( void ) ; void ( * reserve_resources ) ( void ) ; char * ( * memory_setup ) ( void ) ; } ;

struct x86_init_irqs { void ( * pre_vector_init ) ( void ) ; void ( * intr_init ) ( void ) ; void ( * trap_init ) ( void ) ; } ;

struct x86_init_oem { void ( * arch_setup ) ( void ) ; void ( * banner ) ( void ) ; } ;

struct x86_init_paging { void ( * pagetable_setup_start ) ( pgd_t * base ) ; void ( * pagetable_setup_done ) ( pgd_t * base ) ; } ;

struct x86_init_timers { void ( * setup_percpu_clockev ) ( void ) ; void ( * tsc_pre_init ) ( void ) ; void ( * timer_init ) ( void ) ; } ;

struct x86_init_ops { struct x86_init_resources resources ; struct x86_init_mpparse mpparse ; struct x86_init_irqs irqs ; struct x86_init_oem oem ; struct x86_init_paging paging ; struct x86_init_timers timers ; } ;

struct x86_cpuinit_ops { void ( * setup_percpu_clockev ) ( void ) ; } ;

struct x86_platform_ops { unsigned long ( * calibrate_tsc ) ( void ) ; unsigned long ( * get_wallclock ) ( void ) ; int ( * set_wallclock ) ( unsigned long nowtime ) ; } ;

struct physid_mask { unsigned long mask [ ( ( ( 255 ) + ( 8 * sizeof ( long ) ) - 1 ) / ( 8 * sizeof ( long ) ) ) ] ; } ;

typedef struct physid_mask physid_mask_t ;

struct timex { unsigned int modes ; long offset ; long freq ; long maxerror ; long esterror ; int status ; long constant ; long precision ; long tolerance ; struct timeval time ; long tick ; long ppsfreq ; long jitter ; int shift ; long stabil ; long jitcnt ; long calcnt ; long errcnt ; long stbcnt ; int tai ; int : 32 ; int : 32 ; int : 32 ; int : 32 ; int : 32 ; int : 32 ; int : 32 ; int : 32 ; int : 32 ; int : 32 ; int : 32 ; } ;

typedef unsigned long long cycles_t ;

union ktime { s64 tv64 ; } ;

typedef union ktime ktime_t ;

enum debug_obj_state { ODEBUG_STATE_NONE , ODEBUG_STATE_INIT , ODEBUG_STATE_INACTIVE , ODEBUG_STATE_ACTIVE , ODEBUG_STATE_DESTROYED , ODEBUG_STATE_NOTAVAILABLE , ODEBUG_STATE_MAX } ;

struct debug_obj_descr ;

struct debug_obj { struct hlist_node node ; enum debug_obj_state state ; void * object ; struct debug_obj_descr * descr ; } ;

struct debug_obj_descr { const char * name ; int ( * fixup_init ) ( void * addr , enum debug_obj_state state ) ; int ( * fixup_activate ) ( void * addr , enum debug_obj_state state ) ; int ( * fixup_destroy ) ( void * addr , enum debug_obj_state state ) ; int ( * fixup_free ) ( void * addr , enum debug_obj_state state ) ; } ;

struct tvec_base ;

struct timer_list { struct list_head entry ; unsigned long expires ; void ( * function ) ( unsigned long ) ; unsigned long data ; struct tvec_base * base ; void * start_site ; char start_comm [ 16 ] ; int start_pid ; } ;

struct hrtimer ;

struct workqueue_struct ;

struct work_struct ;

typedef void ( * work_func_t ) ( struct work_struct * work ) ;

struct work_struct { atomic_long_t data ; struct list_head entry ; work_func_t func ; } ;

struct delayed_work { struct work_struct work ; struct timer_list timer ; } ;

struct execute_work { struct work_struct work ; } ;

typedef struct pm_message { int event ; } pm_message_t ;

struct dev_pm_ops { int ( * prepare ) ( struct device * dev ) ; void ( * complete ) ( struct device * dev ) ; int ( * suspend ) ( struct device * dev ) ; int ( * resume ) ( struct device * dev ) ; int ( * freeze ) ( struct device * dev ) ; int ( * thaw ) ( struct device * dev ) ; int ( * poweroff ) ( struct device * dev ) ; int ( * restore ) ( struct device * dev ) ; int ( * suspend_noirq ) ( struct device * dev ) ; int ( * resume_noirq ) ( struct device * dev ) ; int ( * freeze_noirq ) ( struct device * dev ) ; int ( * thaw_noirq ) ( struct device * dev ) ; int ( * poweroff_noirq ) ( struct device * dev ) ; int ( * restore_noirq ) ( struct device * dev ) ; int ( * runtime_suspend ) ( struct device * dev ) ; int ( * runtime_resume ) ( struct device * dev ) ; int ( * runtime_idle ) ( struct device * dev ) ; } ;

enum dpm_state { DPM_INVALID , DPM_ON , DPM_PREPARING , DPM_RESUMING , DPM_SUSPENDING , DPM_OFF , DPM_OFF_IRQ } ;

enum rpm_status { RPM_ACTIVE = 0 , RPM_RESUMING , RPM_SUSPENDED , RPM_SUSPENDING } ;

enum rpm_request { RPM_REQ_NONE = 0 , RPM_REQ_IDLE , RPM_REQ_SUSPEND , RPM_REQ_RESUME } ;

struct dev_pm_info { pm_message_t power_state ; unsigned int can_wakeup : 1 ; unsigned int should_wakeup : 1 ; enum dpm_state status ; struct list_head entry ; struct timer_list suspend_timer ; unsigned long timer_expires ; struct work_struct work ; wait_queue_head_t wait_queue ; spinlock_t lock ; atomic_t usage_count ; atomic_t child_count ; unsigned int disable_depth : 3 ; unsigned int ignore_children : 1 ; unsigned int idle_notification : 1 ; unsigned int request_pending : 1 ; unsigned int deferred_resume : 1 ; enum rpm_request request ; enum rpm_status runtime_status ; int runtime_error ; } ;

enum dpm_order { DPM_ORDER_NONE , DPM_ORDER_DEV_AFTER_PARENT , DPM_ORDER_PARENT_BEFORE_DEV , DPM_ORDER_DEV_LAST } ;

struct local_apic { struct { unsigned int __reserved [ 4 ] ; } __reserved_01 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_02 ; struct { unsigned int __reserved_1 : 24 , phys_apic_id : 4 , __reserved_2 : 4 ; unsigned int __reserved [ 3 ] ; } id ; const struct { unsigned int version : 8 , __reserved_1 : 8 , max_lvt : 8 , __reserved_2 : 8 ; unsigned int __reserved [ 3 ] ; } version ; struct { unsigned int __reserved [ 4 ] ; } __reserved_03 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_04 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_05 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_06 ; struct { unsigned int priority : 8 , __reserved_1 : 24 ; unsigned int __reserved_2 [ 3 ] ; } tpr ; const struct { unsigned int priority : 8 , __reserved_1 : 24 ; unsigned int __reserved_2 [ 3 ] ; } apr ; const struct { unsigned int priority : 8 , __reserved_1 : 24 ; unsigned int __reserved_2 [ 3 ] ; } ppr ; struct { unsigned int eoi ; unsigned int __reserved [ 3 ] ; } eoi ; struct { unsigned int __reserved [ 4 ] ; } __reserved_07 ; struct { unsigned int __reserved_1 : 24 , logical_dest : 8 ; unsigned int __reserved_2 [ 3 ] ; } ldr ; struct { unsigned int __reserved_1 : 28 , model : 4 ; unsigned int __reserved_2 [ 3 ] ; } dfr ; struct { unsigned int spurious_vector : 8 , apic_enabled : 1 , focus_cpu : 1 , __reserved_2 : 22 ; unsigned int __reserved_3 [ 3 ] ; } svr ; struct { unsigned int bitfield ; unsigned int __reserved [ 3 ] ; } isr [ 8 ] ; struct { unsigned int bitfield ; unsigned int __reserved [ 3 ] ; } tmr [ 8 ] ; struct { unsigned int bitfield ; unsigned int __reserved [ 3 ] ; } irr [ 8 ] ; union { struct { unsigned int send_cs_error : 1 , receive_cs_error : 1 , send_accept_error : 1 , receive_accept_error : 1 , __reserved_1 : 1 , send_illegal_vector : 1 , receive_illegal_vector : 1 , illegal_register_address : 1 , __reserved_2 : 24 ; unsigned int __reserved_3 [ 3 ] ; } error_bits ; struct { unsigned int errors ; unsigned int __reserved_3 [ 3 ] ; } all_errors ; } esr ; struct { unsigned int __reserved [ 4 ] ; } __reserved_08 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_09 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_10 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_11 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_12 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_13 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_14 ; struct { unsigned int vector : 8 , delivery_mode : 3 , destination_mode : 1 , delivery_status : 1 , __reserved_1 : 1 , level : 1 , trigger : 1 , __reserved_2 : 2 , shorthand : 2 , __reserved_3 : 12 ; unsigned int __reserved_4 [ 3 ] ; } icr1 ; struct { union { unsigned int __reserved_1 : 24 , phys_dest : 4 , __reserved_2 : 4 ; unsigned int __reserved_3 : 24 , logical_dest : 8 ; } dest ; unsigned int __reserved_4 [ 3 ] ; } icr2 ; struct { unsigned int vector : 8 , __reserved_1 : 4 , delivery_status : 1 , __reserved_2 : 3 , mask : 1 , timer_mode : 1 , __reserved_3 : 14 ; unsigned int __reserved_4 [ 3 ] ; } lvt_timer ; struct { unsigned int vector : 8 , delivery_mode : 3 , __reserved_1 : 1 , delivery_status : 1 , __reserved_2 : 3 , mask : 1 , __reserved_3 : 15 ; unsigned int __reserved_4 [ 3 ] ; } lvt_thermal ; struct { unsigned int vector : 8 , delivery_mode : 3 , __reserved_1 : 1 , delivery_status : 1 , __reserved_2 : 3 , mask : 1 , __reserved_3 : 15 ; unsigned int __reserved_4 [ 3 ] ; } lvt_pc ; struct { unsigned int vector : 8 , delivery_mode : 3 , __reserved_1 : 1 , delivery_status : 1 , polarity : 1 , remote_irr : 1 , trigger : 1 , mask : 1 , __reserved_2 : 15 ; unsigned int __reserved_3 [ 3 ] ; } lvt_lint0 ; struct { unsigned int vector : 8 , delivery_mode : 3 , __reserved_1 : 1 , delivery_status : 1 , polarity : 1 , remote_irr : 1 , trigger : 1 , mask : 1 , __reserved_2 : 15 ; unsigned int __reserved_3 [ 3 ] ; } lvt_lint1 ; struct { unsigned int vector : 8 , __reserved_1 : 4 , delivery_status : 1 , __reserved_2 : 3 , mask : 1 , __reserved_3 : 15 ; unsigned int __reserved_4 [ 3 ] ; } lvt_error ; struct { unsigned int initial_count ; unsigned int __reserved_2 [ 3 ] ; } timer_icr ; const struct { unsigned int curr_count ; unsigned int __reserved_2 [ 3 ] ; } timer_ccr ; struct { unsigned int __reserved [ 4 ] ; } __reserved_16 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_17 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_18 ; struct { unsigned int __reserved [ 4 ] ; } __reserved_19 ; struct { unsigned int divisor : 4 , __reserved_1 : 28 ; unsigned int __reserved_2 [ 3 ] ; } timer_dcr ; struct { unsigned int __reserved [ 4 ] ; } __reserved_20 ; } __attribute__ ( ( packed ) ) ;

struct bootnode { u64 start ; u64 end ; } ;

typedef struct { void * ldt ; int size ; struct mutex lock ; void * vdso ; } mm_context_t ;

struct bootnode ;

enum vsyscall_num { __NR_vgettimeofday , __NR_vtime , __NR_vgetcpu } ;

enum fixed_addresses { VSYSCALL_LAST_PAGE , VSYSCALL_FIRST_PAGE = VSYSCALL_LAST_PAGE + ( ( ( - 2UL << 20 ) - ( - 10UL << 20 ) ) >> 12 ) - 1 , VSYSCALL_HPET , FIX_DBGP_BASE , FIX_EARLYCON_MEM_BASE , FIX_APIC_BASE , FIX_IO_APIC_BASE_0 , FIX_IO_APIC_BASE_END = FIX_IO_APIC_BASE_0 + 128 - 1 , FIX_PARAVIRT_BOOTMAP , FIX_TEXT_POKE1 , FIX_TEXT_POKE0 , __end_of_permanent_fixed_addresses , FIX_BTMAP_END = __end_of_permanent_fixed_addresses + 256 - ( __end_of_permanent_fixed_addresses & 255 ) , FIX_BTMAP_BEGIN = FIX_BTMAP_END + 64 * 4 - 1 , __end_of_fixed_addresses } ;

struct apic { char * name ; int ( * probe ) ( void ) ; int ( * acpi_madt_oem_check ) ( char * oem_id , char * oem_table_id ) ; int ( * apic_id_registered ) ( void ) ; u32 irq_delivery_mode ; u32 irq_dest_mode ; const struct cpumask * ( * target_cpus ) ( void ) ; int disable_esr ; int dest_logical ; unsigned long ( * check_apicid_used ) ( physid_mask_t bitmap , int apicid ) ; unsigned long ( * check_apicid_present ) ( int apicid ) ; void ( * vector_allocation_domain ) ( int cpu , struct cpumask * retmask ) ; void ( * init_apic_ldr ) ( void ) ; physid_mask_t ( * ioapic_phys_id_map ) ( physid_mask_t map ) ; void ( * setup_apic_routing ) ( void ) ; int ( * multi_timer_check ) ( int apic , int irq ) ; int ( * apicid_to_node ) ( int logical_apicid ) ; int ( * cpu_to_logical_apicid ) ( int cpu ) ; int ( * cpu_present_to_apicid ) ( int mps_cpu ) ; physid_mask_t ( * apicid_to_cpu_present ) ( int phys_apicid ) ; void ( * setup_portio_remap ) ( void ) ; int ( * check_phys_apicid_present ) ( int phys_apicid ) ; void ( * enable_apic_mode ) ( void ) ; int ( * phys_pkg_id ) ( int cpuid_apic , int index_msb ) ; int ( * mps_oem_check ) ( struct mpc_table * mpc , char * oem , char * productid ) ; unsigned int ( * get_apic_id ) ( unsigned long x ) ; unsigned long ( * set_apic_id ) ( unsigned int id ) ; unsigned long apic_id_mask ; unsigned int ( * cpu_mask_to_apicid ) ( const struct cpumask * cpumask ) ; unsigned int ( * cpu_mask_to_apicid_and ) ( const struct cpumask * cpumask , const struct cpumask * andmask ) ; void ( * send_IPI_mask ) ( const struct cpumask * mask , int vector ) ; void ( * send_IPI_mask_allbutself ) ( const struct cpumask * mask , int vector ) ; void ( * send_IPI_allbutself ) ( int vector ) ; void ( * send_IPI_all ) ( int vector ) ; void ( * send_IPI_self ) ( int vector ) ; int ( * wakeup_secondary_cpu ) ( int apicid , unsigned long start_eip ) ; int trampoline_phys_low ; int trampoline_phys_high ; void ( * wait_for_init_deassert ) ( atomic_t * deassert ) ; void ( * smp_callin_clear_local_apic ) ( void ) ; void ( * inquire_remote_apic ) ( int apicid ) ; u32 ( * read ) ( u32 reg ) ; void ( * write ) ( u32 reg , u32 v ) ; u64 ( * icr_read ) ( void ) ; void ( * icr_write ) ( u32 low , u32 high ) ; void ( * wait_icr_idle ) ( void ) ; u32 ( * safe_wait_icr_idle ) ( void ) ; } ;

union IO_APIC_reg_00 { u32 raw ; struct { u32 __reserved_2 : 14 , LTS : 1 , delivery_type : 1 , __reserved_1 : 8 , ID : 8 ; } __attribute__ ( ( packed ) ) bits ; } ;

union IO_APIC_reg_01 { u32 raw ; struct { u32 version : 8 , __reserved_2 : 7 , PRQ : 1 , entries : 8 , __reserved_1 : 8 ; } __attribute__ ( ( packed ) ) bits ; } ;

union IO_APIC_reg_02 { u32 raw ; struct { u32 __reserved_2 : 24 , arbitration : 4 , __reserved_1 : 4 ; } __attribute__ ( ( packed ) ) bits ; } ;

union IO_APIC_reg_03 { u32 raw ; struct { u32 boot_DT : 1 , __reserved_1 : 31 ; } __attribute__ ( ( packed ) ) bits ; } ;

enum ioapic_irq_destination_types { dest_Fixed = 0 , dest_LowestPrio = 1 , dest_SMI = 2 , dest__reserved_1 = 3 , dest_NMI = 4 , dest_INIT = 5 , dest__reserved_2 = 6 , dest_ExtINT = 7 } ;

struct IO_APIC_route_entry { __u32 vector : 8 , delivery_mode : 3 , dest_mode : 1 , delivery_status : 1 , polarity : 1 , irr : 1 , trigger : 1 , mask : 1 , __reserved_2 : 15 ; __u32 __reserved_3 : 24 , dest : 8 ; } __attribute__ ( ( packed ) ) ;

struct IR_IO_APIC_route_entry { __u64 vector : 8 , zero : 3 , index2 : 1 , delivery_status : 1 , polarity : 1 , irr : 1 , trigger : 1 , mask : 1 , reserved : 31 , format : 1 , index : 15 ; } __attribute__ ( ( packed ) ) ;

struct io_apic_irq_attr ;

struct mp_ioapic_gsi { int gsi_base ; int gsi_end ; } ;

struct smp_ops { void ( * smp_prepare_boot_cpu ) ( void ) ; void ( * smp_prepare_cpus ) ( unsigned max_cpus ) ; void ( * smp_cpus_done ) ( unsigned max_cpus ) ; void ( * smp_send_stop ) ( void ) ; void ( * smp_send_reschedule ) ( int cpu ) ; int ( * cpu_up ) ( unsigned cpu ) ; int ( * cpu_disable ) ( void ) ; void ( * cpu_die ) ( unsigned int cpu ) ; void ( * play_dead ) ( void ) ; void ( * send_call_func_ipi ) ( const struct cpumask * mask ) ; void ( * send_call_func_single_ipi ) ( int cpu ) ; } ;

struct memnode { int shift ; unsigned int mapsize ; s16 * map ; s16 embedded_map [ 64 - 8 ] ; } __attribute__ ( ( __aligned__ ( ( 1 << ( 6 ) ) ) ) ) ;

struct page_cgroup ;

struct mem_section { unsigned long section_mem_map ; unsigned long * pageblock_flags ; struct page_cgroup * page_cgroup ; unsigned long pad ; } ;

struct call_single_data { struct list_head list ; void ( * func ) ( void * info ) ; void * info ; u16 flags ; u16 priv ; } ;

struct pci_bus ;

struct vm_area_struct ;

struct key ;

struct subprocess_info ;

enum umh_wait { UMH_NO_WAIT = - 1 , UMH_WAIT_EXEC = 0 , UMH_WAIT_PROC = 1 } ;

struct user_i387_struct { unsigned short cwd ; unsigned short swd ; unsigned short twd ; unsigned short fop ; __u64 rip ; __u64 rdp ; __u32 mxcsr ; __u32 mxcsr_mask ; __u32 st_space [ 32 ] ; __u32 xmm_space [ 64 ] ; __u32 padding [ 24 ] ; } ;

struct user_regs_struct { unsigned long r15 ; unsigned long r14 ; unsigned long r13 ; unsigned long r12 ; unsigned long bp ; unsigned long bx ; unsigned long r11 ; unsigned long r10 ; unsigned long r9 ; unsigned long r8 ; unsigned long ax ; unsigned long cx ; unsigned long dx ; unsigned long si ; unsigned long di ; unsigned long orig_ax ; unsigned long ip ; unsigned long cs ; unsigned long flags ; unsigned long sp ; unsigned long ss ; unsigned long fs_base ; unsigned long gs_base ; unsigned long ds ; unsigned long es ; unsigned long fs ; unsigned long gs ; } ;

struct user { struct user_regs_struct regs ; int u_fpvalid ; int pad0 ; struct user_i387_struct i387 ; unsigned long int u_tsize ; unsigned long int u_dsize ; unsigned long int u_ssize ; unsigned long start_code ; unsigned long start_stack ; long int signal ; int reserved ; int pad1 ; unsigned long u_ar0 ; struct user_i387_struct * u_fpstate ; unsigned long magic ; char u_comm [ 32 ] ; unsigned long u_debugreg [ 8 ] ; unsigned long error_code ; unsigned long fault_address ; } ;

typedef unsigned long elf_greg_t ;

typedef elf_greg_t elf_gregset_t [ ( sizeof ( struct user_regs_struct ) / sizeof ( elf_greg_t ) ) ] ;

typedef struct user_i387_struct elf_fpregset_t ;

struct linux_binprm ;

typedef __u32 Elf32_Addr ;

typedef __u16 Elf32_Half ;

typedef __u32 Elf32_Off ;

typedef __s32 Elf32_Sword ;

typedef __u32 Elf32_Word ;

typedef __u64 Elf64_Addr ;

typedef __u16 Elf64_Half ;

typedef __s16 Elf64_SHalf ;

typedef __u64 Elf64_Off ;

typedef __s32 Elf64_Sword ;

typedef __u32 Elf64_Word ;

typedef __u64 Elf64_Xword ;

typedef __s64 Elf64_Sxword ;

typedef struct dynamic { Elf32_Sword d_tag ; union { Elf32_Sword d_val ; Elf32_Addr d_ptr ; } d_un ; } Elf32_Dyn ;

typedef struct { Elf64_Sxword d_tag ; union { Elf64_Xword d_val ; Elf64_Addr d_ptr ; } d_un ; } Elf64_Dyn ;

typedef struct elf32_rel { Elf32_Addr r_offset ; Elf32_Word r_info ; } Elf32_Rel ;

typedef struct elf64_rel { Elf64_Addr r_offset ; Elf64_Xword r_info ; } Elf64_Rel ;

typedef struct elf32_rela { Elf32_Addr r_offset ; Elf32_Word r_info ; Elf32_Sword r_addend ; } Elf32_Rela ;

typedef struct elf64_rela { Elf64_Addr r_offset ; Elf64_Xword r_info ; Elf64_Sxword r_addend ; } Elf64_Rela ;

typedef struct elf32_sym { Elf32_Word st_name ; Elf32_Addr st_value ; Elf32_Word st_size ; unsigned char st_info ; unsigned char st_other ; Elf32_Half st_shndx ; } Elf32_Sym ;

typedef struct elf64_sym { Elf64_Word st_name ; unsigned char st_info ; unsigned char st_other ; Elf64_Half st_shndx ; Elf64_Addr st_value ; Elf64_Xword st_size ; } Elf64_Sym ;

typedef struct elf32_hdr { unsigned char e_ident [ 16 ] ; Elf32_Half e_type ; Elf32_Half e_machine ; Elf32_Word e_version ; Elf32_Addr e_entry ; Elf32_Off e_phoff ; Elf32_Off e_shoff ; Elf32_Word e_flags ; Elf32_Half e_ehsize ; Elf32_Half e_phentsize ; Elf32_Half e_phnum ; Elf32_Half e_shentsize ; Elf32_Half e_shnum ; Elf32_Half e_shstrndx ; } Elf32_Ehdr ;

typedef struct elf64_hdr { unsigned char e_ident [ 16 ] ; Elf64_Half e_type ; Elf64_Half e_machine ; Elf64_Word e_version ; Elf64_Addr e_entry ; Elf64_Off e_phoff ; Elf64_Off e_shoff ; Elf64_Word e_flags ; Elf64_Half e_ehsize ; Elf64_Half e_phentsize ; Elf64_Half e_phnum ; Elf64_Half e_shentsize ; Elf64_Half e_shnum ; Elf64_Half e_shstrndx ; } Elf64_Ehdr ;

typedef struct elf32_phdr { Elf32_Word p_type ; Elf32_Off p_offset ; Elf32_Addr p_vaddr ; Elf32_Addr p_paddr ; Elf32_Word p_filesz ; Elf32_Word p_memsz ; Elf32_Word p_flags ; Elf32_Word p_align ; } Elf32_Phdr ;

typedef struct elf64_phdr { Elf64_Word p_type ; Elf64_Word p_flags ; Elf64_Off p_offset ; Elf64_Addr p_vaddr ; Elf64_Addr p_paddr ; Elf64_Xword p_filesz ; Elf64_Xword p_memsz ; Elf64_Xword p_align ; } Elf64_Phdr ;

typedef struct { Elf32_Word sh_name ; Elf32_Word sh_type ; Elf32_Word sh_flags ; Elf32_Addr sh_addr ; Elf32_Off sh_offset ; Elf32_Word sh_size ; Elf32_Word sh_link ; Elf32_Word sh_info ; Elf32_Word sh_addralign ; Elf32_Word sh_entsize ; } Elf32_Shdr ;

typedef struct elf64_shdr { Elf64_Word sh_name ; Elf64_Word sh_type ; Elf64_Xword sh_flags ; Elf64_Addr sh_addr ; Elf64_Off sh_offset ; Elf64_Xword sh_size ; Elf64_Word sh_link ; Elf64_Word sh_info ; Elf64_Xword sh_addralign ; Elf64_Xword sh_entsize ; } Elf64_Shdr ;

typedef struct elf32_note { Elf32_Word n_namesz ; Elf32_Word n_descsz ; Elf32_Word n_type ; } Elf32_Nhdr ;

typedef struct elf64_note { Elf64_Word n_namesz ; Elf64_Word n_descsz ; Elf64_Word n_type ; } Elf64_Nhdr ;

struct kobject ;

struct attribute { const char * name ; struct module * owner ; mode_t mode ; } ;

struct attribute_group { const char * name ; mode_t ( * is_visible ) ( struct kobject * , struct attribute * , int ) ; struct attribute * * attrs ; } ;

struct bin_attribute { struct attribute attr ; size_t size ; void * private ; ssize_t ( * read ) ( struct kobject * , struct bin_attribute * , char * , loff_t , size_t ) ; ssize_t ( * write ) ( struct kobject * , struct bin_attribute * , char * , loff_t , size_t ) ; int ( * mmap ) ( struct kobject * , struct bin_attribute * attr , struct vm_area_struct * vma ) ; } ;

struct sysfs_ops { ssize_t ( * show ) ( struct kobject * , struct attribute * , char * ) ; ssize_t ( * store ) ( struct kobject * , struct attribute * , const char * , size_t ) ; } ;

struct sysfs_dirent ;

struct kref { atomic_t refcount ; } ;

enum kobject_action { KOBJ_ADD , KOBJ_REMOVE , KOBJ_CHANGE , KOBJ_MOVE , KOBJ_ONLINE , KOBJ_OFFLINE , KOBJ_MAX } ;

struct kobject { const char * name ; struct list_head entry ; struct kobject * parent ; struct kset * kset ; struct kobj_type * ktype ; struct sysfs_dirent * sd ; struct kref kref ; unsigned int state_initialized : 1 ; unsigned int state_in_sysfs : 1 ; unsigned int state_add_uevent_sent : 1 ; unsigned int state_remove_uevent_sent : 1 ; unsigned int uevent_suppress : 1 ; } ;

struct kobj_type { void ( * release ) ( struct kobject * kobj ) ; struct sysfs_ops * sysfs_ops ; struct attribute * * default_attrs ; } ;

struct kobj_uevent_env { char * envp [ 32 ] ; int envp_idx ; char buf [ 2048 ] ; int buflen ; } ;

struct kset_uevent_ops { int ( * filter ) ( struct kset * kset , struct kobject * kobj ) ; const char * ( * name ) ( struct kset * kset , struct kobject * kobj ) ; int ( * uevent ) ( struct kset * kset , struct kobject * kobj , struct kobj_uevent_env * env ) ; } ;

struct kobj_attribute { struct attribute attr ; ssize_t ( * show ) ( struct kobject * kobj , struct kobj_attribute * attr , char * buf ) ; ssize_t ( * store ) ( struct kobject * kobj , struct kobj_attribute * attr , const char * buf , size_t count ) ; } ;

struct kset { struct list_head list ; spinlock_t list_lock ; struct kobject kobj ; struct kset_uevent_ops * uevent_ops ; } ;

struct kernel_param ;

typedef int ( * param_set_fn ) ( const char * val , struct kernel_param * kp ) ;

typedef int ( * param_get_fn ) ( char * buffer , struct kernel_param * kp ) ;

struct kernel_param { const char * name ; u16 perm ; u16 flags ; param_set_fn set ; param_get_fn get ; union { void * arg ; const struct kparam_string * str ; const struct kparam_array * arr ; } ; } ;

struct kparam_string { unsigned int maxlen ; char * string ; } ;

struct kparam_array { unsigned int max ; unsigned int * num ; param_set_fn set ; param_get_fn get ; unsigned int elemsize ; void * elem ; } ;

struct completion { unsigned int done ; wait_queue_head_t wait ; } ;

struct rcu_head { struct rcu_head * next ; void ( * func ) ( struct rcu_head * head ) ; } ;

struct notifier_block ;

struct rcu_synchronize { struct rcu_head head ; struct completion completion ; } ;

struct tracepoint ;

struct tracepoint { const char * name ; int state ; void ( * regfunc ) ( void ) ; void ( * unregfunc ) ( void ) ; void * * funcs ; } __attribute__ ( ( aligned ( 32 ) ) ) ;

struct tracepoint_iter { struct module * module ; struct tracepoint * tracepoint ; } ;

enum stat_item { ALLOC_FASTPATH , ALLOC_SLOWPATH , FREE_FASTPATH , FREE_SLOWPATH , FREE_FROZEN , FREE_ADD_PARTIAL , FREE_REMOVE_PARTIAL , ALLOC_FROM_PARTIAL , ALLOC_SLAB , ALLOC_REFILL , FREE_SLAB , CPUSLAB_FLUSH , DEACTIVATE_FULL , DEACTIVATE_EMPTY , DEACTIVATE_TO_HEAD , DEACTIVATE_TO_TAIL , DEACTIVATE_REMOTE_FREES , ORDER_FALLBACK , NR_SLUB_STAT_ITEMS } ;

struct kmem_cache_cpu { void * * freelist ; struct page * page ; int node ; unsigned int offset ; unsigned int objsize ; } ;

struct kmem_cache_node { spinlock_t list_lock ; unsigned long nr_partial ; struct list_head partial ; atomic_long_t nr_slabs ; atomic_long_t total_objects ; struct list_head full ; } ;

struct kmem_cache_order_objects { unsigned long x ; } ;

struct kmem_cache { unsigned long flags ; int size ; int objsize ; int offset ; struct kmem_cache_order_objects oo ; struct kmem_cache_node local_node ; struct kmem_cache_order_objects max ; struct kmem_cache_order_objects min ; gfp_t allocflags ; int refcount ; void ( * ctor ) ( void * ) ; int inuse ; int align ; unsigned long min_partial ; const char * name ; struct list_head list ; struct kobject kobj ; int remote_node_defrag_ratio ; struct kmem_cache_node * node [ ( 1 << 6 ) ] ; struct kmem_cache_cpu * cpu_slab [ 64 ] ; } ;

struct pcpu_group_info { int nr_units ; unsigned long base_offset ; unsigned int * cpu_map ; } ;

struct pcpu_alloc_info { size_t static_size ; size_t reserved_size ; size_t dyn_size ; size_t unit_size ; size_t atom_size ; size_t alloc_size ; size_t __ai_size ; int nr_groups ; struct pcpu_group_info groups [ ] ; } ;

enum pcpu_fc { PCPU_FC_AUTO , PCPU_FC_EMBED , PCPU_FC_PAGE , PCPU_FC_NR } ;

typedef void * ( * pcpu_fc_alloc_fn_t ) ( unsigned int cpu , size_t size , size_t align ) ;

typedef void ( * pcpu_fc_free_fn_t ) ( void * ptr , size_t size ) ;

typedef void ( * pcpu_fc_populate_pte_fn_t ) ( unsigned long addr ) ;

typedef int ( pcpu_fc_cpu_distance_fn_t ) ( unsigned int from , unsigned int to ) ;

typedef struct { atomic_long_t a ; } local_t ;

struct mod_arch_specific { } ;

struct kernel_symbol { unsigned long value ; const char * name ; } ;

struct modversion_info { unsigned long crc ; char name [ ( 64 - sizeof ( unsigned long ) ) ] ; } ;

struct module_attribute { struct attribute attr ; ssize_t ( * show ) ( struct module_attribute * , struct module * , char * ) ; ssize_t ( * store ) ( struct module_attribute * , struct module * , const char * , size_t count ) ; void ( * setup ) ( struct module * , const char * ) ; int ( * test ) ( struct module * ) ; void ( * free ) ( struct module * ) ; } ;

struct module_kobject { struct kobject kobj ; struct module * mod ; struct kobject * drivers_dir ; struct module_param_attrs * mp ; } ;

struct exception_table_entry ;

enum module_state { MODULE_STATE_LIVE , MODULE_STATE_COMING , MODULE_STATE_GOING } ;

struct module { enum module_state state ; struct list_head list ; char name [ ( 64 - sizeof ( unsigned long ) ) ] ; struct module_kobject mkobj ; struct module_attribute * modinfo_attrs ; const char * version ; const char * srcversion ; struct kobject * holders_dir ; const struct kernel_symbol * syms ; const unsigned long * crcs ; unsigned int num_syms ; struct kernel_param * kp ; unsigned int num_kp ; unsigned int num_gpl_syms ; const struct kernel_symbol * gpl_syms ; const unsigned long * gpl_crcs ; const struct kernel_symbol * unused_syms ; const unsigned long * unused_crcs ; unsigned int num_unused_syms ; unsigned int num_unused_gpl_syms ; const struct kernel_symbol * unused_gpl_syms ; const unsigned long * unused_gpl_crcs ; const struct kernel_symbol * gpl_future_syms ; const unsigned long * gpl_future_crcs ; unsigned int num_gpl_future_syms ; unsigned int num_exentries ; struct exception_table_entry * extable ; int ( * init ) ( void ) ; void * module_init ; void * module_core ; unsigned int init_size , core_size ; unsigned int init_text_size , core_text_size ; struct mod_arch_specific arch ; unsigned int taints ; unsigned num_bugs ; struct list_head bug_list ; struct bug_entry * bug_table ; Elf64_Sym * symtab , * core_symtab ; unsigned int num_symtab , core_num_syms ; char * strtab , * core_strtab ; struct module_sect_attrs * sect_attrs ; struct module_notes_attrs * notes_attrs ; void * percpu ; char * args ; struct tracepoint * tracepoints ; unsigned int num_tracepoints ; const char * * trace_bprintk_fmt_start ; unsigned int num_trace_bprintk_fmt ; struct ftrace_event_call * trace_events ; unsigned int num_trace_events ; unsigned long * ftrace_callsites ; unsigned int num_ftrace_callsites ; struct list_head modules_which_use_me ; struct task_struct * waiter ; void ( * exit ) ( void ) ; char * refptr ; ctor_fn_t * ctors ; unsigned int num_ctors ; } ;

struct symsearch { const struct kernel_symbol * start , * stop ; const unsigned long * crcs ; enum { NOT_GPL_ONLY , GPL_ONLY , WILL_BE_GPL_ONLY } licence ; bool unused ; } ;

struct device_driver ;

struct kernel_module { int is_granary ; int ( * * init ) ( void ) ; void ( * * exit ) ( void ) ; void * address ; void * text_begin ; void * text_end ; enum { KERNEL_MODULE_STATE_LIVE , KERNEL_MODULE_STATE_COMING , KERNEL_MODULE_STATE_GOING } state ; struct kernel_module * next ; } ;

#undef KERN_BOOL

#ifdef HAVE_OLD_UNUSED
#   define __unused __attribute__((unused))
#endif

#undef private
#undef public
#undef protected
#undef bool

#ifdef __cplusplus
} /* extern */
} /* kernel namespace */
#endif

#endif /* DRK_KERNEL_TYPES_HPP_ */
