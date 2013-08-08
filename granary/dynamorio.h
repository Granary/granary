/* **********************************************************
 * Copyright (c) 2011-2012 Google, Inc.  All rights reserved.
 * Copyright (c) 2002-2010 VMware, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* Copyright (c) 2003-2007 Determina Corp. */
/* Copyright (c) 2002-2003 Massachusetts Institute of Technology */

#ifndef Granary_DYNAMORIO_H_
#define Granary_DYNAMORIO_H_

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   include <string.h>
#endif

#ifndef X64
#   define X64 1
#endif

#define PREFIX_LOCK           0x1 /**< Makes the instruction's memory accesses atomic. */
#define PREFIX_JCC_NOT_TAKEN  0x2 /**< Branch hint: conditional branch is taken. */
#define PREFIX_JCC_TAKEN      0x4 /**< Branch hint: conditional branch is not taken. */
/* These are used only in the decoding tables.  We decode the
 * information into the operands.
 * For encoding these properties are specified in the operands,
 * with our encoder auto-adding the appropriate prefixes.
 */
#define PREFIX_DATA           0x0008
#define PREFIX_ADDR           0x0010
#define PREFIX_REX_W          0x0020
#define PREFIX_REX_R          0x0040
#define PREFIX_REX_X          0x0080
#define PREFIX_REX_B          0x0100
#define PREFIX_REX_GENERAL    0x0200 /* 0x40: only matters for SPL...SDL vs AH..BH */
#define PREFIX_REX_ALL        (PREFIX_REX_W|PREFIX_REX_R|PREFIX_REX_X|PREFIX_REX_B|\
                               PREFIX_REX_GENERAL)
#define PREFIX_SIZE_SPECIFIERS (PREFIX_DATA|PREFIX_ADDR|PREFIX_REX_ALL)

/* Unused except in decode tables (we encode the prefix into the opcodes) */
#define PREFIX_REP            0x0400
#define PREFIX_REPNE          0x0800

/* PREFIX_SEG_* is set by decode or decode_cti and is only a hint
 * to the caller.  Is ignored by encode in favor of the segment
 * reg specified in the applicable opnds.  We rely on it being set during
 * bb building.
 */
#define PREFIX_SEG_FS         0x1000
#define PREFIX_SEG_GS         0x2000

/* First two are only used during initial decode so if running out of
 * space could replace w/ byte value compare.
 */
#define PREFIX_VEX_2B    0x000004000
#define PREFIX_VEX_3B    0x000008000
#define PREFIX_VEX_L     0x000010000

/****************************************************************************
 * EFLAGS
 */
/* we only care about these 11 flags, and mostly only about the first 6
 * we consider an undefined effect on a flag to be a write
 */
#define EFLAGS_READ_CF   0x00000001 /**< Reads CF (Carry Flag). */
#define EFLAGS_READ_PF   0x00000002 /**< Reads PF (Parity Flag). */
#define EFLAGS_READ_AF   0x00000004 /**< Reads AF (Auxiliary Carry Flag). */
#define EFLAGS_READ_ZF   0x00000008 /**< Reads ZF (Zero Flag). */
#define EFLAGS_READ_SF   0x00000010 /**< Reads SF (Sign Flag). */
#define EFLAGS_READ_TF   0x00000020 /**< Reads TF (Trap Flag). */
#define EFLAGS_READ_IF   0x00000040 /**< Reads IF (Interrupt Enable Flag). */
#define EFLAGS_READ_DF   0x00000080 /**< Reads DF (Direction Flag). */
#define EFLAGS_READ_OF   0x00000100 /**< Reads OF (Overflow Flag). */
#define EFLAGS_READ_NT   0x00000200 /**< Reads NT (Nested Task). */
#define EFLAGS_READ_RF   0x00000400 /**< Reads RF (Resume Flag). */
#define EFLAGS_WRITE_CF  0x00000800 /**< Writes CF (Carry Flag). */
#define EFLAGS_WRITE_PF  0x00001000 /**< Writes PF (Parity Flag). */
#define EFLAGS_WRITE_AF  0x00002000 /**< Writes AF (Auxiliary Carry Flag). */
#define EFLAGS_WRITE_ZF  0x00004000 /**< Writes ZF (Zero Flag). */
#define EFLAGS_WRITE_SF  0x00008000 /**< Writes SF (Sign Flag). */
#define EFLAGS_WRITE_TF  0x00010000 /**< Writes TF (Trap Flag). */
#define EFLAGS_WRITE_IF  0x00020000 /**< Writes IF (Interrupt Enable Flag). */
#define EFLAGS_WRITE_DF  0x00040000 /**< Writes DF (Direction Flag). */
#define EFLAGS_WRITE_OF  0x00080000 /**< Writes OF (Overflow Flag). */
#define EFLAGS_WRITE_NT  0x00100000 /**< Writes NT (Nested Task). */
#define EFLAGS_WRITE_RF  0x00200000 /**< Writes RF (Resume Flag). */

#define EFLAGS_READ_ALL  0x000007ff /**< Reads all flags. */
#define EFLAGS_WRITE_ALL 0x003ff800 /**< Writes all flags. */

#ifdef __cplusplus
namespace dynamorio {
extern "C" {
#endif

typedef unsigned char __u_char ;
typedef unsigned short int __u_short ;
typedef unsigned int __u_int ;
typedef unsigned long int __u_long ;
typedef signed char __int8_t ;
typedef unsigned char __uint8_t ;
typedef signed short int __int16_t ;
typedef unsigned short int __uint16_t ;
typedef signed int __int32_t ;
typedef unsigned int __uint32_t ;
typedef signed long int __int64_t ;
typedef unsigned long int __uint64_t ;
typedef long int __quad_t ;
typedef unsigned long int __u_quad_t ;
typedef unsigned long int __dev_t ;
typedef unsigned int __uid_t ;
typedef unsigned int __gid_t ;
typedef unsigned long int __ino_t ;
typedef unsigned long int __ino64_t ;
typedef unsigned int __mode_t ;
typedef unsigned long int __nlink_t ;
typedef long int __off_t ;
typedef long int __off64_t ;
typedef int __pid_t ;
typedef struct { int __val [ 2 ] ; } __fsid_t ;
typedef long int __clock_t ;
typedef unsigned long int __rlim_t ;
typedef unsigned long int __rlim64_t ;
typedef unsigned int __id_t ;
typedef long int __time_t ;
typedef unsigned int __useconds_t ;
typedef long int __suseconds_t ;
typedef int __daddr_t ;
typedef long int __swblk_t ;
typedef int __key_t ;
typedef int __clockid_t ;
typedef void * __timer_t ;
typedef long int __blksize_t ;
typedef long int __blkcnt_t ;
typedef long int __blkcnt64_t ;
typedef unsigned long int __fsblkcnt_t ;
typedef unsigned long int __fsblkcnt64_t ;
typedef unsigned long int __fsfilcnt_t ;
typedef unsigned long int __fsfilcnt64_t ;
typedef long int __ssize_t ;
typedef __off64_t __loff_t ;
typedef __quad_t * __qaddr_t ;
typedef char * __caddr_t ;
typedef long int __intptr_t ;
typedef unsigned int __socklen_t ;
typedef __u_char u_char ;
typedef __u_short u_short ;
typedef __u_int u_int ;
typedef __u_long u_long ;
typedef __quad_t quad_t ;
typedef __u_quad_t u_quad_t ;
typedef __fsid_t fsid_t ;
typedef __loff_t loff_t ;
typedef __ino_t ino_t ;
typedef __dev_t dev_t ;
typedef __gid_t gid_t ;
typedef __mode_t mode_t ;
typedef __nlink_t nlink_t ;
typedef __uid_t uid_t ;
typedef __off_t off_t ;
typedef __pid_t pid_t ;
typedef __id_t id_t ;
typedef __ssize_t ssize_t ;
typedef __daddr_t daddr_t ;
typedef __caddr_t caddr_t ;
typedef __key_t key_t ;
typedef __time_t time_t ;
typedef __clockid_t clockid_t ;
typedef __timer_t timer_t ;
typedef long unsigned int size_t ;
typedef unsigned long int ulong ;
typedef unsigned short int ushort ;
typedef unsigned int uint ;
typedef int int8_t __attribute__ ( ( __mode__ ( __QI__ ) ) ) ;
typedef int int16_t __attribute__ ( ( __mode__ ( __HI__ ) ) ) ;
typedef int int32_t __attribute__ ( ( __mode__ ( __SI__ ) ) ) ;
typedef int int64_t __attribute__ ( ( __mode__ ( __DI__ ) ) ) ;
typedef unsigned int u_int8_t __attribute__ ( ( __mode__ ( __QI__ ) ) ) ;
typedef unsigned int u_int16_t __attribute__ ( ( __mode__ ( __HI__ ) ) ) ;
typedef unsigned int u_int32_t __attribute__ ( ( __mode__ ( __SI__ ) ) ) ;
typedef unsigned int u_int64_t __attribute__ ( ( __mode__ ( __DI__ ) ) ) ;
typedef int register_t __attribute__ ( ( __mode__ ( __word__ ) ) ) ;
typedef int __sig_atomic_t ;
typedef struct { unsigned long int __val [ ( 1024 / ( 8 * sizeof ( unsigned long int ) ) ) ] ; } __sigset_t ;
typedef __sigset_t sigset_t ;
/*record type*/
struct timespec { __time_t tv_sec ; long int tv_nsec ; } ;
struct timeval { __time_t tv_sec ; __suseconds_t tv_usec ; } ;
typedef __suseconds_t suseconds_t ;
typedef long int __fd_mask ;
typedef struct { __fd_mask __fds_bits [ 1024 / ( 8 * ( int ) sizeof ( __fd_mask ) ) ] ; } fd_set ;
typedef __fd_mask fd_mask ;
typedef __blkcnt_t blkcnt_t ;
typedef __fsblkcnt_t fsblkcnt_t ;
typedef __fsfilcnt_t fsfilcnt_t ;
typedef unsigned long int pthread_t ;
typedef union { char __size [ 56 ] ; long int __align ; } pthread_attr_t ;
typedef struct __pthread_internal_list { struct __pthread_internal_list * __prev ; struct __pthread_internal_list * __next ; } __pthread_list_t ;
typedef union { struct __pthread_mutex_s { int __lock ; unsigned int __count ; int __owner ; unsigned int __nusers ; int __kind ; int __spins ; __pthread_list_t __list ; } __data ; char __size [ 40 ] ; long int __align ; } pthread_mutex_t ;
typedef union { char __size [ 4 ] ; int __align ; } pthread_mutexattr_t ;
typedef union { struct { int __lock ; unsigned int __futex ; __extension__ unsigned long long int __total_seq ; __extension__ unsigned long long int __wakeup_seq ; __extension__ unsigned long long int __woken_seq ; void * __mutex ; unsigned int __nwaiters ; unsigned int __broadcast_seq ; } __data ; char __size [ 48 ] ; __extension__ long long int __align ; } pthread_cond_t ;
typedef union { char __size [ 4 ] ; int __align ; } pthread_condattr_t ;
typedef unsigned int pthread_key_t ;
typedef int pthread_once_t ;
typedef union { struct { int __lock ; unsigned int __nr_readers ; unsigned int __readers_wakeup ; unsigned int __writer_wakeup ; unsigned int __nr_readers_queued ; unsigned int __nr_writers_queued ; int __writer ; int __shared ; unsigned long int __pad1 ; unsigned long int __pad2 ; unsigned int __flags ; } __data ; char __size [ 56 ] ; long int __align ; } pthread_rwlock_t ;
typedef union { char __size [ 8 ] ; long int __align ; } pthread_rwlockattr_t ;
typedef volatile int pthread_spinlock_t ;
typedef union { char __size [ 32 ] ; long int __align ; } pthread_barrier_t ;
typedef union { char __size [ 4 ] ; int __align ; } pthread_barrierattr_t ;
typedef __sig_atomic_t sig_atomic_t ;
typedef union sigval { int sival_int ; void * sival_ptr ; } sigval_t ;
typedef struct siginfo { int si_signo ; int si_errno ; int si_code ; union { int _pad [ ( ( 128 / sizeof ( int ) ) - 4 ) ] ; struct { __pid_t si_pid ; __uid_t si_uid ; } _kill ; struct { int si_tid ; int si_overrun ; sigval_t si_sigval ; } _timer ; struct { __pid_t si_pid ; __uid_t si_uid ; sigval_t si_sigval ; } _rt ; struct { __pid_t si_pid ; __uid_t si_uid ; int si_status ; __clock_t si_utime ; __clock_t si_stime ; } _sigchld ; struct { void * si_addr ; } _sigfault ; struct { long int si_band ; int si_fd ; } _sigpoll ; } _sifields ; } siginfo_t ;
/*enum type*/
enum { SI_ASYNCNL = - 60 , SI_TKILL = - 6 , SI_SIGIO , SI_ASYNCIO , SI_MESGQ , SI_TIMER , SI_QUEUE , SI_USER , SI_KERNEL = 0x80 } ;
enum { ILL_ILLOPC = 1 , ILL_ILLOPN , ILL_ILLADR , ILL_ILLTRP , ILL_PRVOPC , ILL_PRVREG , ILL_COPROC , ILL_BADSTK } ;
enum { FPE_INTDIV = 1 , FPE_INTOVF , FPE_FLTDIV , FPE_FLTOVF , FPE_FLTUND , FPE_FLTRES , FPE_FLTINV , FPE_FLTSUB } ;
enum { SEGV_MAPERR = 1 , SEGV_ACCERR } ;
enum { BUS_ADRALN = 1 , BUS_ADRERR , BUS_OBJERR } ;
enum { TRAP_BRKPT = 1 , TRAP_TRACE } ;
enum { CLD_EXITED = 1 , CLD_KILLED , CLD_DUMPED , CLD_TRAPPED , CLD_STOPPED , CLD_CONTINUED } ;
enum { POLL_IN = 1 , POLL_OUT , POLL_MSG , POLL_ERR , POLL_PRI , POLL_HUP } ;
typedef struct sigevent { sigval_t sigev_value ; int sigev_signo ; int sigev_notify ; union { int _pad [ ( ( 64 / sizeof ( int ) ) - 4 ) ] ; __pid_t _tid ; struct { void ( * _function ) ( sigval_t ) ; void * _attribute ; } _sigev_thread ; } _sigev_un ; } sigevent_t ;
enum { SIGEV_SIGNAL = 0 , SIGEV_NONE , SIGEV_THREAD , SIGEV_THREAD_ID = 4 } ;
typedef void ( * __sighandler_t ) ( int ) ;
typedef __sighandler_t sig_t ;
struct sigaction { union { __sighandler_t sa_handler ; void ( * sa_sigaction ) ( int , siginfo_t * , void * ) ; } __sigaction_handler ; __sigset_t sa_mask ; int sa_flags ; void ( * sa_restorer ) ( void ) ; } ;
struct sigvec { __sighandler_t sv_handler ; int sv_mask ; int sv_flags ; } ;
struct _fpreg { unsigned short significand [ 4 ] ; unsigned short exponent ; } ;
struct _fpxreg { unsigned short significand [ 4 ] ; unsigned short exponent ; unsigned short padding [ 3 ] ; } ;
struct _xmmreg { __uint32_t element [ 4 ] ; } ;
struct _fpstate { __uint16_t cwd ; __uint16_t swd ; __uint16_t ftw ; __uint16_t fop ; __uint64_t rip ; __uint64_t rdp ; __uint32_t mxcsr ; __uint32_t mxcr_mask ; struct _fpxreg _st [ 8 ] ; struct _xmmreg _xmm [ 16 ] ; __uint32_t padding [ 24 ] ; } ;
struct sigcontext { unsigned long r8 ; unsigned long r9 ; unsigned long r10 ; unsigned long r11 ; unsigned long r12 ; unsigned long r13 ; unsigned long r14 ; unsigned long r15 ; unsigned long rdi ; unsigned long rsi ; unsigned long rbp ; unsigned long rbx ; unsigned long rdx ; unsigned long rax ; unsigned long rcx ; unsigned long rsp ; unsigned long rip ; unsigned long eflags ; unsigned short cs ; unsigned short gs ; unsigned short fs ; unsigned short __pad0 ; unsigned long err ; unsigned long trapno ; unsigned long oldmask ; unsigned long cr2 ; struct _fpstate * fpstate ; unsigned long __reserved1 [ 8 ] ; } ;
struct sigstack { void * ss_sp ; int ss_onstack ; } ;
enum { SS_ONSTACK = 1 , SS_DISABLE } ;
typedef struct sigaltstack { void * ss_sp ; int ss_flags ; size_t ss_size ; } stack_t ;
typedef unsigned char byte ;
typedef signed char sbyte ;
typedef byte * app_pc ;
typedef void ( * generic_func_t ) ( ) ;
typedef unsigned long int uint64 ;
typedef long int int64 ;
typedef uint64 reg_t ;
typedef reg_t ptr_uint_t ;
typedef int64 ptr_int_t ;
typedef size_t app_rva_t ;
typedef pid_t thread_id_t ;
typedef pid_t process_id_t ;
typedef int file_t ;
typedef uint client_id_t ;
typedef enum { SYSLOG_INFORMATION = 0x1 , SYSLOG_WARNING = 0x2 , SYSLOG_ERROR = 0x4 , SYSLOG_CRITICAL = 0x8 , SYSLOG_NONE = 0x0 , SYSLOG_ALL = SYSLOG_INFORMATION | SYSLOG_WARNING | SYSLOG_ERROR | SYSLOG_CRITICAL } syslog_event_type_t ;
typedef unsigned int uint32 ;
typedef uint64 timestamp_t ;
typedef int64 stats_int_t ;
typedef char pathstring_t [ 260 ] ;
typedef char liststring_t [ 2048 ] ;
enum { RUNUNDER_OFF = 0x00 , RUNUNDER_ON = 0x01 , RUNUNDER_ALL = 0x02 , RUNUNDER_FORMERLY_EXPLICIT = 0x04 , RUNUNDER_COMMANDLINE_MATCH = 0x08 , RUNUNDER_COMMANDLINE_DISPATCH = 0x10 , RUNUNDER_COMMANDLINE_NO_STRIP = 0x20 , RUNUNDER_ONCE = 0x40 , RUNUNDER_EXPLICIT = 0x80 , } ;
typedef enum { NUDGE_DR_opt , NUDGE_DR_reset , NUDGE_DR_detach , NUDGE_DR_mode , NUDGE_DR_policy , NUDGE_DR_lstats , NUDGE_DR_process_control , NUDGE_DR_upgrade , NUDGE_DR_kstats , NUDGE_DR_stats , NUDGE_DR_invalidate , NUDGE_DR_recreate_pc , NUDGE_DR_recreate_state , NUDGE_DR_reattach , NUDGE_DR_diagnose , NUDGE_DR_ldmp , NUDGE_DR_freeze , NUDGE_DR_persist , NUDGE_DR_client , NUDGE_DR_violation , NUDGE_DR_PARAMETRIZED_END } nudge_generic_type_t ;
enum { NUDGE_IS_INTERNAL = 0x01 , } ;
typedef struct { int ignored1 ; uint nudge_action_mask : 28 ; uint version : 2 ; uint flags : 2 ; int ignored2 ; client_id_t client_id ; uint64 client_arg ; } nudge_arg_t ;
enum { GET_PARAMETER_BUF_TOO_SMALL = - 1 , GET_PARAMETER_FAILURE = 0 , GET_PARAMETER_SUCCESS = 1 , GET_PARAMETER_NOAPPSPECIFIC = 2 , SET_PARAMETER_FAILURE = GET_PARAMETER_FAILURE , SET_PARAMETER_SUCCESS = GET_PARAMETER_SUCCESS } ;
typedef union _dr_xmm_t { uint64 u64 [ 2 ] ; uint u32 [ 4 ] ; byte u8 [ 16 ] ; reg_t reg [ 2 ] ; } dr_xmm_t ;
typedef union _dr_ymm_t { uint u32 [ 8 ] ; byte u8 [ 32 ] ; reg_t reg [ 4 ] ; } dr_ymm_t ;
typedef enum { DR_MC_INTEGER = 0x01 , DR_MC_CONTROL = 0x02 , DR_MC_MULTIMEDIA = 0x04 , DR_MC_ALL = ( DR_MC_INTEGER | DR_MC_CONTROL | DR_MC_MULTIMEDIA ) , } dr_mcontext_flags_t ;
typedef struct _dr_mcontext_t { size_t size ; dr_mcontext_flags_t flags ; union { reg_t xdi ; reg_t rdi ; } ; union { reg_t xsi ; reg_t rsi ; } ; union { reg_t xbp ; reg_t rbp ; } ; union { reg_t xsp ; reg_t rsp ; } ; union { reg_t xbx ; reg_t rbx ; } ; union { reg_t xdx ; reg_t rdx ; } ; union { reg_t xcx ; reg_t rcx ; } ; union { reg_t xax ; reg_t rax ; } ; reg_t r8 ; reg_t r9 ; reg_t r10 ; reg_t r11 ; reg_t r12 ; reg_t r13 ; reg_t r14 ; reg_t r15 ; union { reg_t xflags ; reg_t rflags ; } ; union { byte * xip ; byte * pc ; byte * rip ; } ; byte padding [ 16 ] ; dr_ymm_t ymm [ 16 ] ; } dr_mcontext_t ;
typedef struct _priv_mcontext_t { union { reg_t xdi ; reg_t rdi ; } ; union { reg_t xsi ; reg_t rsi ; } ; union { reg_t xbp ; reg_t rbp ; } ; union { reg_t xsp ; reg_t rsp ; } ; union { reg_t xbx ; reg_t rbx ; } ; union { reg_t xdx ; reg_t rdx ; } ; union { reg_t xcx ; reg_t rcx ; } ; union { reg_t xax ; reg_t rax ; } ; reg_t r8 ; reg_t r9 ; reg_t r10 ; reg_t r11 ; reg_t r12 ; reg_t r13 ; reg_t r14 ; reg_t r15 ; union { reg_t xflags ; reg_t rflags ; } ; union { byte * xip ; byte * pc ; byte * rip ; } ; byte padding [ 16 ] ; dr_ymm_t ymm [ 16 ] ; } priv_mcontext_t ;
typedef struct { int quot ; int rem ; } div_t ;
typedef struct { long int quot ; long int rem ; } ldiv_t ;
typedef struct { long long int quot ; long long int rem ; } lldiv_t ;
struct random_data { int32_t * fptr ; int32_t * rptr ; int32_t * state ; int rand_type ; int rand_deg ; int rand_sep ; int32_t * end_ptr ; } ;
struct drand48_data { unsigned short int __x [ 3 ] ; unsigned short int __old_x [ 3 ] ; unsigned short int __c ; unsigned short int __init ; unsigned long long int __a ; } ;
typedef int ( * __compar_fn_t ) ( __const void * , __const void * ) ;
struct _IO_FILE ;
typedef struct _IO_FILE FILE ;
typedef struct _IO_FILE __FILE ;
typedef struct { int __count ; union { unsigned int __wch ; char __wchb [ 4 ] ; } __value ; } __mbstate_t ;
typedef struct { __off_t __pos ; __mbstate_t __state ; } _G_fpos_t ;
typedef struct { __off64_t __pos ; __mbstate_t __state ; } _G_fpos64_t ;
typedef int _G_int16_t __attribute__ ( ( __mode__ ( __HI__ ) ) ) ;
typedef int _G_int32_t __attribute__ ( ( __mode__ ( __SI__ ) ) ) ;
typedef unsigned int _G_uint16_t __attribute__ ( ( __mode__ ( __HI__ ) ) ) ;
typedef unsigned int _G_uint32_t __attribute__ ( ( __mode__ ( __SI__ ) ) ) ;
typedef __builtin_va_list __gnuc_va_list ;
struct _IO_jump_t ;
typedef void _IO_lock_t ;
struct _IO_marker { struct _IO_marker * _next ; struct _IO_FILE * _sbuf ; int _pos ; } ;
enum __codecvt_result { __codecvt_ok , __codecvt_partial , __codecvt_error , __codecvt_noconv } ;
struct _IO_FILE { int _flags ; char * _IO_read_ptr ; char * _IO_read_end ; char * _IO_read_base ; char * _IO_write_base ; char * _IO_write_ptr ; char * _IO_write_end ; char * _IO_buf_base ; char * _IO_buf_end ; char * _IO_save_base ; char * _IO_backup_base ; char * _IO_save_end ; struct _IO_marker * _markers ; struct _IO_FILE * _chain ; int _fileno ; int _flags2 ; __off_t _old_offset ; unsigned short _cur_column ; signed char _vtable_offset ; char _shortbuf [ 1 ] ; _IO_lock_t * _lock ; __off64_t _offset ; void * __pad1 ; void * __pad2 ; void * __pad3 ; void * __pad4 ; size_t __pad5 ; int _mode ; char _unused2 [ 15 * sizeof ( int ) - 4 * sizeof ( void * ) - sizeof ( size_t ) ] ; } ;
typedef struct _IO_FILE _IO_FILE ;
struct _IO_FILE_plus ;
typedef __ssize_t __io_read_fn ( void * __cookie , char * __buf , size_t __nbytes ) ;
typedef __ssize_t __io_write_fn ( void * __cookie , __const char * __buf , size_t __n ) ;
typedef int __io_seek_fn ( void * __cookie , __off64_t * __pos , int __w ) ;
typedef int __io_close_fn ( void * __cookie ) ;
typedef _G_fpos_t fpos_t ;
typedef unsigned char uchar ;
typedef byte * cache_pc ;
struct _opnd_t ;
typedef struct _opnd_t opnd_t ;
struct _instr_t ;
typedef struct _instr_t instr_t ;
struct _instr_list_t ;
struct _fragment_t ;
typedef struct _fragment_t fragment_t ;
struct _future_fragment_t ;
typedef struct _future_fragment_t future_fragment_t ;
struct _trace_t ;
typedef struct _trace_t trace_t ;
struct _linkstub_t ;
typedef struct _linkstub_t linkstub_t ;
struct _dcontext_t ;
typedef struct _dcontext_t dcontext_t ;
struct vm_area_vector_t ;
typedef struct vm_area_vector_t vm_area_vector_t ;
struct _coarse_info_t ;
typedef struct _coarse_info_t coarse_info_t ;
struct _coarse_freeze_info_t ;
typedef struct _coarse_freeze_info_t coarse_freeze_info_t ;
struct _module_data_t ;
typedef struct _instr_list_t instrlist_t ;
typedef struct _module_data_t module_data_t ;
typedef struct { uint year ; uint month ; uint day_of_week ; uint day ; uint hour ; uint minute ; uint second ; uint milliseconds ; } dr_time_t ;
typedef struct _thread_record_t { thread_id_t id ; process_id_t pid ; bool execve ; uint num ; bool under_dynamo_control ; dcontext_t * dcontext ; struct _thread_record_t * next ; } thread_record_t ;
typedef enum { MAP_FILE_COPY_ON_WRITE = 0x0001 , MAP_FILE_IMAGE = 0x0002 , MAP_FILE_FIXED = 0x0004 , MAP_FILE_REACHABLE = 0x0008 , } map_flags_t ;
typedef byte * heap_pc ;
typedef struct _special_heap_iterator { void * heap ; void * next_unit ; } special_heap_iterator_t ;
typedef void * contention_event_t ;
typedef struct _mutex_t { volatile int lock_requests ; contention_event_t contended_event ; } mutex_t ;
typedef struct _spin_mutex_t { mutex_t lock ; } spin_mutex_t ;
typedef struct _recursive_lock_t { mutex_t lock ; thread_id_t owner ; uint count ; } recursive_lock_t ;
typedef struct _read_write_lock_t { mutex_t lock ; volatile int num_readers ; thread_id_t writer ; volatile int num_pending_readers ; contention_event_t writer_waiting_readers ; contention_event_t readers_waiting_writer ; } read_write_lock_t ;
struct _broadcast_event_t ;
typedef struct _broadcast_event_t broadcast_event_t ;
typedef enum { DR_MODE_NONE = 0 , DR_MODE_CODE_MANIPULATION = 1 , DR_MODE_DO_NOT_RUN = 4 , } dr_operation_mode_t ;
typedef enum { DR_SUCCESS , DR_PROC_REG_EXISTS , DR_PROC_REG_INVALID , DR_PRIORITY_INVALID , DR_ID_CONFLICTING , DR_ID_INVALID , DR_FAILURE , DR_NUDGE_PID_NOT_INJECTED , DR_NUDGE_TIMEOUT , DR_CONFIG_STRING_TOO_LONG , DR_CONFIG_FILE_WRITE_FAILED , DR_NUDGE_PID_NOT_FOUND , } dr_config_status_t ;
typedef enum { DR_PLATFORM_DEFAULT , DR_PLATFORM_32BIT , DR_PLATFORM_64BIT , } dr_platform_t ;
typedef struct _dr_client_iterator_t dr_client_iterator_t ;
typedef enum { HASH_FUNCTION_NONE = 0 , HASH_FUNCTION_MULTIPLY_PHI = 1 , HASH_FUNCTION_STRING = 7 , HASH_FUNCTION_STRING_NOCASE = 8 , HASH_FUNCTION_ENUM_MAX , } hash_function_t ;
typedef uint bitmap_element_t ;
typedef bitmap_element_t bitmap_t [ ] ;
typedef enum { BASE_DIR , PROCESS_DIR } log_dir_t ;
enum { LONGJMP_EXCEPTION = 1 } ;
typedef struct { uint num_self ; timestamp_t total_self ; timestamp_t total_sub ; timestamp_t min_cum ; timestamp_t max_cum ; timestamp_t total_outliers ; } kstat_variable_t ;
typedef struct { kstat_variable_t thread_measured ; kstat_variable_t bb_building ; kstat_variable_t bb_decoding ; kstat_variable_t bb_emit ; kstat_variable_t mangling ; kstat_variable_t emit ; kstat_variable_t hotp_lookup ; kstat_variable_t trace_building ; kstat_variable_t temp_private_bb ; kstat_variable_t monitor_enter ; kstat_variable_t monitor_enter_thci ; kstat_variable_t cache_flush_unit_walk ; kstat_variable_t flush_region ; kstat_variable_t synchall_flush ; kstat_variable_t coarse_pclookup ; kstat_variable_t coarse_freeze_all ; kstat_variable_t persisted_generation ; kstat_variable_t persisted_load ; kstat_variable_t dispatch_num_exits ; kstat_variable_t num_exits_ind_good_miss ; kstat_variable_t num_exits_dir_miss ; kstat_variable_t num_exits_not_in_cache ; kstat_variable_t num_exits_ind_bad_miss_bb2bb ; kstat_variable_t num_exits_ind_bad_miss_bb2trace ; kstat_variable_t num_exits_ind_bad_miss_bb ; kstat_variable_t num_exits_ind_bad_miss_trace2trace ; kstat_variable_t num_exits_ind_bad_miss_trace2bb_nth ; kstat_variable_t num_exits_ind_bad_miss_trace2bb_th ; kstat_variable_t num_exits_ind_bad_miss_trace2bb ; kstat_variable_t num_exits_ind_bad_miss_trace ; kstat_variable_t num_exits_ind_bad_miss ; kstat_variable_t num_exits_dir_syscall ; kstat_variable_t num_exits_dir_cbret ; kstat_variable_t logging ; kstat_variable_t overhead_empty ; kstat_variable_t overhead_nested ; kstat_variable_t syscall_fcache ; kstat_variable_t pre_syscall ; kstat_variable_t post_syscall ; kstat_variable_t pre_syscall_free ; kstat_variable_t pre_syscall_protect ; kstat_variable_t pre_syscall_unmap ; kstat_variable_t post_syscall_alloc ; kstat_variable_t post_syscall_map ; kstat_variable_t native_exec_fcache ; kstat_variable_t fcache_default ; kstat_variable_t fcache_bb_bb ; kstat_variable_t fcache_trace_trace ; kstat_variable_t fcache_bb_trace ; kstat_variable_t wait_event ; kstat_variable_t rct_no_reloc ; kstat_variable_t rct_reloc ; kstat_variable_t rct_reloc_per_page ; kstat_variable_t aslr_validate_relocate ; kstat_variable_t aslr_compare ; } kstat_variables_t ;
typedef struct { kstat_variable_t * var ; timestamp_t self_time ; timestamp_t subpath_time ; timestamp_t outlier_time ; } kstat_node_t ;
enum { KSTAT_MAX_DEPTH = 16 } ;
typedef struct { volatile uint depth ; timestamp_t last_start_time ; timestamp_t last_end_time ; kstat_node_t node [ KSTAT_MAX_DEPTH ] ; } kstat_stack_t ;
typedef struct { thread_id_t thread_id ; kstat_variables_t vars_kstats ; kstat_stack_t stack_kstats ; file_t outfile_kstats ; } thread_kstats_t ;
enum { DUMP_NO_QUOTING = 0x01000 , DUMP_OCTAL = 0x02000 , DUMP_NO_CHARS = 0x04000 , DUMP_RAW = 0x08000 , DUMP_DWORD = 0x10000 , DUMP_ADDRESS = 0x20000 , DUMP_APPEND_ASCII = 0x40000 , DUMP_PER_LINE = 0x000ff } ;
struct MD5Context { uint32 state [ 4 ] ; uint64 count ; unsigned char buffer [ 64 ] ; } ;
typedef enum { OPTION_ENABLED = 0x1 , OPTION_DISABLED = 0x0 , OPTION_BLOCK = 0x2 , OPTION_NO_BLOCK = 0x0 , OPTION_HANDLING = 0x4 , OPTION_NO_HANDLING = 0x0 , OPTION_REPORT = 0x8 , OPTION_NO_REPORT = 0x0 , OPTION_BLOCK_IGNORE_DETECT = 0x20 , OPTION_CUSTOM = 0x100 , OPTION_NO_CUSTOM = 0x0 , } security_option_t ;
enum { HOOKED_TRAMPOLINE_DIE = 0 , HOOKED_TRAMPOLINE_SQUASH = 1 , HOOKED_TRAMPOLINE_CHAIN = 2 , HOOKED_TRAMPOLINE_HOOK_DEEPER = 3 , HOOKED_TRAMPOLINE_MAX = 3 , } ;
enum { APPFAULT_FAULT = 0x0001 , APPFAULT_CRASH = 0x0002 , } ;
typedef enum { OP_PCACHE_NOP = 0 , OP_PCACHE_LOCAL = 1 , OP_PCACHE_GLOBAL = 2 , } op_pcache_t ;
enum option_is_internal { OPTION_DEFAULT_VALUE_dynamic_options = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_dynamic_options = ( 0 ) , OPTION_IS_STRING_dynamic_options = 0 , OPTION_AFFECTS_PCACHE_dynamic_options = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_dummy_version = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_dummy_version = ( 1 ) , OPTION_IS_STRING_dummy_version = 0 , OPTION_AFFECTS_PCACHE_dummy_version = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_nolink = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_nolink = ( 1 ) , OPTION_IS_STRING_nolink = 0 , OPTION_AFFECTS_PCACHE_nolink = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_link_ibl = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_link_ibl = ( 1 ) , OPTION_IS_STRING_link_ibl = 0 , OPTION_AFFECTS_PCACHE_link_ibl = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_tracedump_binary = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_tracedump_binary = ( 1 ) , OPTION_IS_STRING_tracedump_binary = 0 , OPTION_AFFECTS_PCACHE_tracedump_binary = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_tracedump_text = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_tracedump_text = ( 1 ) , OPTION_IS_STRING_tracedump_text = 0 , OPTION_AFFECTS_PCACHE_tracedump_text = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_tracedump_origins = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_tracedump_origins = ( 1 ) , OPTION_IS_STRING_tracedump_origins = 0 , OPTION_AFFECTS_PCACHE_tracedump_origins = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_syntax_intel = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_syntax_intel = ( 0 ) , OPTION_IS_STRING_syntax_intel = 0 , OPTION_AFFECTS_PCACHE_syntax_intel = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_syntax_att = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_syntax_att = ( 0 ) , OPTION_IS_STRING_syntax_att = 0 , OPTION_AFFECTS_PCACHE_syntax_att = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_decode_strict = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_decode_strict = ( 0 ) , OPTION_IS_STRING_decode_strict = 0 , OPTION_AFFECTS_PCACHE_decode_strict = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_bbdump_tags = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_bbdump_tags = ( 1 ) , OPTION_IS_STRING_bbdump_tags = 0 , OPTION_AFFECTS_PCACHE_bbdump_tags = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_gendump = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_gendump = ( 1 ) , OPTION_IS_STRING_gendump = 0 , OPTION_AFFECTS_PCACHE_gendump = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_global_rstats = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_global_rstats = ( 0 ) , OPTION_IS_STRING_global_rstats = 0 , OPTION_AFFECTS_PCACHE_global_rstats = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_logdir = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_logdir = ( 0 ) , OPTION_IS_STRING_logdir = 1 , OPTION_AFFECTS_PCACHE_logdir = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_kstats = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_kstats = ( 0 ) , OPTION_IS_STRING_kstats = 0 , OPTION_AFFECTS_PCACHE_kstats = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_profile_pcs = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_profile_pcs = ( 1 ) , OPTION_IS_STRING_profile_pcs = 0 , OPTION_AFFECTS_PCACHE_profile_pcs = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_client_lib = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_client_lib = ( 1 ) , OPTION_IS_STRING_client_lib = 1 , OPTION_AFFECTS_PCACHE_client_lib = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_loader = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_private_loader = ( 1 ) , OPTION_IS_STRING_private_loader = 0 , OPTION_AFFECTS_PCACHE_private_loader = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_client_lib_tls_size = ( ptr_uint_t ) 1 , OPTION_IS_INTERNAL_client_lib_tls_size = ( 1 ) , OPTION_IS_STRING_client_lib_tls_size = 0 , OPTION_AFFECTS_PCACHE_client_lib_tls_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_privload_register_gdb = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_privload_register_gdb = ( 1 ) , OPTION_IS_STRING_privload_register_gdb = 0 , OPTION_AFFECTS_PCACHE_privload_register_gdb = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_code_api = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_code_api = ( 1 ) , OPTION_IS_STRING_code_api = 0 , OPTION_AFFECTS_PCACHE_code_api = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_probe_api = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_probe_api = ( 1 ) , OPTION_IS_STRING_probe_api = 0 , OPTION_AFFECTS_PCACHE_probe_api = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_opt_speed = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_opt_speed = ( 0 ) , OPTION_IS_STRING_opt_speed = 0 , OPTION_AFFECTS_PCACHE_opt_speed = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_opt_memory = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_opt_memory = ( 0 ) , OPTION_IS_STRING_opt_memory = 0 , OPTION_AFFECTS_PCACHE_opt_memory = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_bb_prefixes = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_bb_prefixes = ( 1 ) , OPTION_IS_STRING_bb_prefixes = 0 , OPTION_AFFECTS_PCACHE_bb_prefixes = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_full_decode = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_full_decode = ( 1 ) , OPTION_IS_STRING_full_decode = 0 , OPTION_AFFECTS_PCACHE_full_decode = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_fast_client_decode = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_fast_client_decode = ( 1 ) , OPTION_IS_STRING_fast_client_decode = 0 , OPTION_AFFECTS_PCACHE_fast_client_decode = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_separate_private_bss = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_separate_private_bss = ( 1 ) , OPTION_IS_STRING_separate_private_bss = 0 , OPTION_AFFECTS_PCACHE_separate_private_bss = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_opt_cleancall = ( ptr_uint_t ) 2 , OPTION_IS_INTERNAL_opt_cleancall = ( 1 ) , OPTION_IS_STRING_opt_cleancall = 0 , OPTION_AFFECTS_PCACHE_opt_cleancall = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cleancall_ignore_eflags = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_cleancall_ignore_eflags = ( 0 ) , OPTION_IS_STRING_cleancall_ignore_eflags = 0 , OPTION_AFFECTS_PCACHE_cleancall_ignore_eflags = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_mangle_app_seg = ( ptr_uint_t ) ( ( 1 ) ) , OPTION_IS_INTERNAL_mangle_app_seg = ( 1 ) , OPTION_IS_STRING_mangle_app_seg = 0 , OPTION_AFFECTS_PCACHE_mangle_app_seg = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_x86_to_x64 = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_x86_to_x64 = ( 0 ) , OPTION_IS_STRING_x86_to_x64 = 0 , OPTION_AFFECTS_PCACHE_x86_to_x64 = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_msgbox_mask = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_msgbox_mask = ( 0 ) , OPTION_IS_STRING_msgbox_mask = 0 , OPTION_AFFECTS_PCACHE_msgbox_mask = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_syslog_mask = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_syslog_mask = ( 0 ) , OPTION_IS_STRING_syslog_mask = 0 , OPTION_AFFECTS_PCACHE_syslog_mask = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_syslog_internal_mask = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_syslog_internal_mask = ( 1 ) , OPTION_IS_STRING_syslog_internal_mask = 0 , OPTION_AFFECTS_PCACHE_syslog_internal_mask = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_syslog_init = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_syslog_init = ( 0 ) , OPTION_IS_STRING_syslog_init = 0 , OPTION_AFFECTS_PCACHE_syslog_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_dumpcore_mask = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_dumpcore_mask = ( 0 ) , OPTION_IS_STRING_dumpcore_mask = 0 , OPTION_AFFECTS_PCACHE_dumpcore_mask = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_pause_on_error_aka_dumpcore_mask = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_pause_on_error_aka_dumpcore_mask = ( 0 ) , OPTION_IS_STRING_pause_on_error_aka_dumpcore_mask = 0 , OPTION_AFFECTS_PCACHE_pause_on_error_aka_dumpcore_mask = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_dumpcore_violation_threshold = ( ptr_uint_t ) 3 , OPTION_IS_INTERNAL_dumpcore_violation_threshold = ( 0 ) , OPTION_IS_STRING_dumpcore_violation_threshold = 0 , OPTION_AFFECTS_PCACHE_dumpcore_violation_threshold = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_live_dump = ( ptr_uint_t ) ( ( 0 ) ) , OPTION_IS_INTERNAL_live_dump = ( 0 ) , OPTION_IS_STRING_live_dump = 0 , OPTION_AFFECTS_PCACHE_live_dump = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_stderr_mask = ( ptr_uint_t ) SYSLOG_CRITICAL | SYSLOG_ERROR | SYSLOG_WARNING , OPTION_IS_INTERNAL_stderr_mask = ( 0 ) , OPTION_IS_STRING_stderr_mask = 0 , OPTION_AFFECTS_PCACHE_stderr_mask = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_appfault_mask = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_appfault_mask = ( 0 ) , OPTION_IS_STRING_appfault_mask = 0 , OPTION_AFFECTS_PCACHE_appfault_mask = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_dup_stdout_on_close = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_dup_stdout_on_close = ( 0 ) , OPTION_IS_STRING_dup_stdout_on_close = 0 , OPTION_AFFECTS_PCACHE_dup_stdout_on_close = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_dup_stderr_on_close = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_dup_stderr_on_close = ( 0 ) , OPTION_IS_STRING_dup_stderr_on_close = 0 , OPTION_AFFECTS_PCACHE_dup_stderr_on_close = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_dup_stdin_on_close = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_dup_stdin_on_close = ( 0 ) , OPTION_IS_STRING_dup_stdin_on_close = 0 , OPTION_AFFECTS_PCACHE_dup_stdin_on_close = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_steal_fds = ( ptr_uint_t ) 96 , OPTION_IS_INTERNAL_steal_fds = ( 0 ) , OPTION_IS_STRING_steal_fds = 0 , OPTION_AFFECTS_PCACHE_steal_fds = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_avoid_dlclose = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_avoid_dlclose = ( 0 ) , OPTION_IS_STRING_avoid_dlclose = 0 , OPTION_AFFECTS_PCACHE_avoid_dlclose = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_intercept_all_signals = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_intercept_all_signals = ( 0 ) , OPTION_IS_STRING_intercept_all_signals = 0 , OPTION_AFFECTS_PCACHE_intercept_all_signals = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_use_all_memory_areas = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_use_all_memory_areas = ( 0 ) , OPTION_IS_STRING_use_all_memory_areas = 0 , OPTION_AFFECTS_PCACHE_use_all_memory_areas = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_diagnostics = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_diagnostics = ( 0 ) , OPTION_IS_STRING_diagnostics = 0 , OPTION_AFFECTS_PCACHE_diagnostics = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_max_supported_os_version = ( ptr_uint_t ) 62 , OPTION_IS_INTERNAL_max_supported_os_version = ( 0 ) , OPTION_IS_STRING_max_supported_os_version = 0 , OPTION_AFFECTS_PCACHE_max_supported_os_version = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_os_aslr = ( ptr_uint_t ) 0x1 , OPTION_IS_INTERNAL_os_aslr = ( 0 ) , OPTION_IS_STRING_os_aslr = 0 , OPTION_AFFECTS_PCACHE_os_aslr = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_os_aslr_version = ( ptr_uint_t ) 60 , OPTION_IS_INTERNAL_os_aslr_version = ( 1 ) , OPTION_IS_STRING_os_aslr_version = 0 , OPTION_AFFECTS_PCACHE_os_aslr_version = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_svchost_timeout = ( ptr_uint_t ) 1000 , OPTION_IS_INTERNAL_svchost_timeout = ( 0 ) , OPTION_IS_STRING_svchost_timeout = 0 , OPTION_AFFECTS_PCACHE_svchost_timeout = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_deadlock_timeout = ( ptr_uint_t ) 0 * 3 * 1000 , OPTION_IS_INTERNAL_deadlock_timeout = ( 0 ) , OPTION_IS_STRING_deadlock_timeout = 0 , OPTION_AFFECTS_PCACHE_deadlock_timeout = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_stack_size = ( ptr_uint_t ) 24 * 1024 , OPTION_IS_INTERNAL_stack_size = ( 0 ) , OPTION_IS_STRING_stack_size = 0 , OPTION_AFFECTS_PCACHE_stack_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_stack_shares_gencode = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_stack_shares_gencode = ( 0 ) , OPTION_IS_STRING_stack_shares_gencode = 0 , OPTION_AFFECTS_PCACHE_stack_shares_gencode = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_spinlock_count_on_SMP = ( ptr_uint_t ) 1000U , OPTION_IS_INTERNAL_spinlock_count_on_SMP = ( 0 ) , OPTION_IS_STRING_spinlock_count_on_SMP = 0 , OPTION_AFFECTS_PCACHE_spinlock_count_on_SMP = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_nop_initial_bblock = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_nop_initial_bblock = ( 1 ) , OPTION_IS_STRING_nop_initial_bblock = 0 , OPTION_AFFECTS_PCACHE_nop_initial_bblock = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_nullcalls = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_nullcalls = ( 1 ) , OPTION_IS_STRING_nullcalls = 0 , OPTION_AFFECTS_PCACHE_nullcalls = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_trace_threshold = ( ptr_uint_t ) 50U , OPTION_IS_INTERNAL_trace_threshold = ( 1 ) , OPTION_IS_STRING_trace_threshold = 0 , OPTION_AFFECTS_PCACHE_trace_threshold = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_disable_traces = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_disable_traces = ( 0 ) , OPTION_IS_STRING_disable_traces = 0 , OPTION_AFFECTS_PCACHE_disable_traces = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_enable_traces = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_enable_traces = ( 0 ) , OPTION_IS_STRING_enable_traces = 0 , OPTION_AFFECTS_PCACHE_enable_traces = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_trace_counter_on_delete = ( ptr_uint_t ) 0U , OPTION_IS_INTERNAL_trace_counter_on_delete = ( 1 ) , OPTION_IS_STRING_trace_counter_on_delete = 0 , OPTION_AFFECTS_PCACHE_trace_counter_on_delete = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_max_elide_jmp = ( ptr_uint_t ) 16 , OPTION_IS_INTERNAL_max_elide_jmp = ( 0 ) , OPTION_IS_STRING_max_elide_jmp = 0 , OPTION_AFFECTS_PCACHE_max_elide_jmp = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_max_elide_call = ( ptr_uint_t ) 16 , OPTION_IS_INTERNAL_max_elide_call = ( 0 ) , OPTION_IS_STRING_max_elide_call = 0 , OPTION_AFFECTS_PCACHE_max_elide_call = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_elide_back_jmps = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_elide_back_jmps = ( 0 ) , OPTION_IS_STRING_elide_back_jmps = 0 , OPTION_AFFECTS_PCACHE_elide_back_jmps = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_elide_back_calls = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_elide_back_calls = ( 0 ) , OPTION_IS_STRING_elide_back_calls = 0 , OPTION_AFFECTS_PCACHE_elide_back_calls = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_selfmod_max_writes = ( ptr_uint_t ) 5 , OPTION_IS_INTERNAL_selfmod_max_writes = ( 0 ) , OPTION_IS_STRING_selfmod_max_writes = 0 , OPTION_AFFECTS_PCACHE_selfmod_max_writes = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_max_bb_instrs = ( ptr_uint_t ) 1024 , OPTION_IS_INTERNAL_max_bb_instrs = ( 0 ) , OPTION_IS_STRING_max_bb_instrs = 0 , OPTION_AFFECTS_PCACHE_max_bb_instrs = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_process_SEH_push = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_process_SEH_push = ( 0 ) , OPTION_IS_STRING_process_SEH_push = 0 , OPTION_AFFECTS_PCACHE_process_SEH_push = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_check_for_SEH_push = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_check_for_SEH_push = ( 1 ) , OPTION_IS_STRING_check_for_SEH_push = 0 , OPTION_AFFECTS_PCACHE_check_for_SEH_push = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_bbs = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_shared_bbs = ( 0 ) , OPTION_IS_STRING_shared_bbs = 0 , OPTION_AFFECTS_PCACHE_shared_bbs = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_shared_traces = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_shared_traces = ( 0 ) , OPTION_IS_STRING_shared_traces = 0 , OPTION_AFFECTS_PCACHE_shared_traces = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_thread_private = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_thread_private = ( 0 ) , OPTION_IS_STRING_thread_private = 0 , OPTION_AFFECTS_PCACHE_thread_private = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_remove_shared_trace_heads = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_remove_shared_trace_heads = ( 1 ) , OPTION_IS_STRING_remove_shared_trace_heads = 0 , OPTION_AFFECTS_PCACHE_remove_shared_trace_heads = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_remove_trace_components = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_remove_trace_components = ( 0 ) , OPTION_IS_STRING_remove_trace_components = 0 , OPTION_AFFECTS_PCACHE_remove_trace_components = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_deletion = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_shared_deletion = ( 0 ) , OPTION_IS_STRING_shared_deletion = 0 , OPTION_AFFECTS_PCACHE_shared_deletion = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_syscalls_synch_flush = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_syscalls_synch_flush = ( 0 ) , OPTION_IS_STRING_syscalls_synch_flush = 0 , OPTION_AFFECTS_PCACHE_syscalls_synch_flush = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_lazy_deletion_max_pending = ( ptr_uint_t ) 128 , OPTION_IS_INTERNAL_lazy_deletion_max_pending = ( 0 ) , OPTION_IS_STRING_lazy_deletion_max_pending = 0 , OPTION_AFFECTS_PCACHE_lazy_deletion_max_pending = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_free_unmapped_futures = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_free_unmapped_futures = ( 0 ) , OPTION_IS_STRING_free_unmapped_futures = 0 , OPTION_AFFECTS_PCACHE_free_unmapped_futures = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_ib_in_tls = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_private_ib_in_tls = ( 0 ) , OPTION_IS_STRING_private_ib_in_tls = 0 , OPTION_AFFECTS_PCACHE_private_ib_in_tls = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_single_thread_in_DR = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_single_thread_in_DR = ( 1 ) , OPTION_IS_STRING_single_thread_in_DR = 0 , OPTION_AFFECTS_PCACHE_single_thread_in_DR = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_separate_private_stubs = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_separate_private_stubs = ( 0 ) , OPTION_IS_STRING_separate_private_stubs = 0 , OPTION_AFFECTS_PCACHE_separate_private_stubs = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_separate_shared_stubs = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_separate_shared_stubs = ( 0 ) , OPTION_IS_STRING_separate_shared_stubs = 0 , OPTION_AFFECTS_PCACHE_separate_shared_stubs = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_free_private_stubs = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_free_private_stubs = ( 0 ) , OPTION_IS_STRING_free_private_stubs = 0 , OPTION_AFFECTS_PCACHE_free_private_stubs = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unsafe_free_shared_stubs = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_unsafe_free_shared_stubs = ( 0 ) , OPTION_IS_STRING_unsafe_free_shared_stubs = 0 , OPTION_AFFECTS_PCACHE_unsafe_free_shared_stubs = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cbr_single_stub = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_cbr_single_stub = ( 1 ) , OPTION_IS_STRING_cbr_single_stub = 0 , OPTION_AFFECTS_PCACHE_cbr_single_stub = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_indirect_stubs = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_indirect_stubs = ( 0 ) , OPTION_IS_STRING_indirect_stubs = 0 , OPTION_AFFECTS_PCACHE_indirect_stubs = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_inline_bb_ibl = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_inline_bb_ibl = ( 0 ) , OPTION_IS_STRING_inline_bb_ibl = 0 , OPTION_AFFECTS_PCACHE_inline_bb_ibl = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_atomic_inlined_linking = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_atomic_inlined_linking = ( 0 ) , OPTION_IS_STRING_atomic_inlined_linking = 0 , OPTION_AFFECTS_PCACHE_atomic_inlined_linking = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_inline_trace_ibl = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_inline_trace_ibl = ( 0 ) , OPTION_IS_STRING_inline_trace_ibl = 0 , OPTION_AFFECTS_PCACHE_inline_trace_ibl = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_bb_ibt_tables = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_shared_bb_ibt_tables = ( 0 ) , OPTION_IS_STRING_shared_bb_ibt_tables = 0 , OPTION_AFFECTS_PCACHE_shared_bb_ibt_tables = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_trace_ibt_tables = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_shared_trace_ibt_tables = ( 0 ) , OPTION_IS_STRING_shared_trace_ibt_tables = 0 , OPTION_AFFECTS_PCACHE_shared_trace_ibt_tables = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_ref_count_shared_ibt_tables = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_ref_count_shared_ibt_tables = ( 0 ) , OPTION_IS_STRING_ref_count_shared_ibt_tables = 0 , OPTION_AFFECTS_PCACHE_ref_count_shared_ibt_tables = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_ibl_table_in_tls = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_ibl_table_in_tls = ( 0 ) , OPTION_IS_STRING_ibl_table_in_tls = 0 , OPTION_AFFECTS_PCACHE_ibl_table_in_tls = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_bb_ibl_targets = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_bb_ibl_targets = ( 0 ) , OPTION_IS_STRING_bb_ibl_targets = 0 , OPTION_AFFECTS_PCACHE_bb_ibl_targets = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_bb_ibt_table_includes_traces = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_bb_ibt_table_includes_traces = ( 0 ) , OPTION_IS_STRING_bb_ibt_table_includes_traces = 0 , OPTION_AFFECTS_PCACHE_bb_ibt_table_includes_traces = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_bb_single_restore_prefix = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_bb_single_restore_prefix = ( 0 ) , OPTION_IS_STRING_bb_single_restore_prefix = 0 , OPTION_AFFECTS_PCACHE_bb_single_restore_prefix = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_trace_single_restore_prefix = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_trace_single_restore_prefix = ( 0 ) , OPTION_IS_STRING_trace_single_restore_prefix = 0 , OPTION_AFFECTS_PCACHE_trace_single_restore_prefix = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_rehash_unlinked_threshold = ( ptr_uint_t ) 100 , OPTION_IS_INTERNAL_rehash_unlinked_threshold = ( 1 ) , OPTION_IS_STRING_rehash_unlinked_threshold = 0 , OPTION_AFFECTS_PCACHE_rehash_unlinked_threshold = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_rehash_unlinked_always = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_rehash_unlinked_always = ( 1 ) , OPTION_IS_STRING_rehash_unlinked_always = 0 , OPTION_AFFECTS_PCACHE_rehash_unlinked_always = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_bbs_only = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_shared_bbs_only = ( 0 ) , OPTION_IS_STRING_shared_bbs_only = 0 , OPTION_AFFECTS_PCACHE_shared_bbs_only = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_trace_ibl_routine = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_shared_trace_ibl_routine = ( 0 ) , OPTION_IS_STRING_shared_trace_ibl_routine = 0 , OPTION_AFFECTS_PCACHE_shared_trace_ibl_routine = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_speculate_last_exit = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_speculate_last_exit = ( 0 ) , OPTION_IS_STRING_speculate_last_exit = 0 , OPTION_AFFECTS_PCACHE_speculate_last_exit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_max_trace_bbs = ( ptr_uint_t ) 128 , OPTION_IS_INTERNAL_max_trace_bbs = ( 0 ) , OPTION_IS_STRING_max_trace_bbs = 0 , OPTION_AFFECTS_PCACHE_max_trace_bbs = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_protect_mask = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_protect_mask = ( 0 ) , OPTION_IS_STRING_protect_mask = 0 , OPTION_AFFECTS_PCACHE_protect_mask = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_single_privileged_thread = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_single_privileged_thread = ( 1 ) , OPTION_IS_STRING_single_privileged_thread = 0 , OPTION_AFFECTS_PCACHE_single_privileged_thread = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_alt_hash_func = ( ptr_uint_t ) 1 , OPTION_IS_INTERNAL_alt_hash_func = ( 1 ) , OPTION_IS_STRING_alt_hash_func = 0 , OPTION_AFFECTS_PCACHE_alt_hash_func = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_ibl_hash_func_offset = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_ibl_hash_func_offset = ( 0 ) , OPTION_IS_STRING_ibl_hash_func_offset = 0 , OPTION_AFFECTS_PCACHE_ibl_hash_func_offset = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_ibl_indcall_hash_offset = ( ptr_uint_t ) 4 , OPTION_IS_INTERNAL_ibl_indcall_hash_offset = ( 0 ) , OPTION_IS_STRING_ibl_indcall_hash_offset = 0 , OPTION_AFFECTS_PCACHE_ibl_indcall_hash_offset = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_bb_load = ( ptr_uint_t ) 55 , OPTION_IS_INTERNAL_shared_bb_load = ( 1 ) , OPTION_IS_STRING_shared_bb_load = 0 , OPTION_AFFECTS_PCACHE_shared_bb_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_trace_load = ( ptr_uint_t ) 55 , OPTION_IS_INTERNAL_shared_trace_load = ( 1 ) , OPTION_IS_STRING_shared_trace_load = 0 , OPTION_AFFECTS_PCACHE_shared_trace_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_future_load = ( ptr_uint_t ) 60 , OPTION_IS_INTERNAL_shared_future_load = ( 1 ) , OPTION_IS_STRING_shared_future_load = 0 , OPTION_AFFECTS_PCACHE_shared_future_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_after_call_load = ( ptr_uint_t ) 80 , OPTION_IS_INTERNAL_shared_after_call_load = ( 0 ) , OPTION_IS_STRING_shared_after_call_load = 0 , OPTION_AFFECTS_PCACHE_shared_after_call_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_global_rct_ind_br_load = ( ptr_uint_t ) 80 , OPTION_IS_INTERNAL_global_rct_ind_br_load = ( 0 ) , OPTION_IS_STRING_global_rct_ind_br_load = 0 , OPTION_AFFECTS_PCACHE_global_rct_ind_br_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_trace_load = ( ptr_uint_t ) 55 , OPTION_IS_INTERNAL_private_trace_load = ( 1 ) , OPTION_IS_STRING_private_trace_load = 0 , OPTION_AFFECTS_PCACHE_private_trace_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_ibl_targets_load = ( ptr_uint_t ) 50 , OPTION_IS_INTERNAL_private_ibl_targets_load = ( 0 ) , OPTION_IS_STRING_private_ibl_targets_load = 0 , OPTION_AFFECTS_PCACHE_private_ibl_targets_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_bb_ibl_targets_load = ( ptr_uint_t ) 60 , OPTION_IS_INTERNAL_private_bb_ibl_targets_load = ( 0 ) , OPTION_IS_STRING_private_bb_ibl_targets_load = 0 , OPTION_AFFECTS_PCACHE_private_bb_ibl_targets_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_ibt_table_trace_init = ( ptr_uint_t ) 7 , OPTION_IS_INTERNAL_shared_ibt_table_trace_init = ( 0 ) , OPTION_IS_STRING_shared_ibt_table_trace_init = 0 , OPTION_AFFECTS_PCACHE_shared_ibt_table_trace_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_ibt_table_bb_init = ( ptr_uint_t ) 7 , OPTION_IS_INTERNAL_shared_ibt_table_bb_init = ( 0 ) , OPTION_IS_STRING_shared_ibt_table_bb_init = 0 , OPTION_AFFECTS_PCACHE_shared_ibt_table_bb_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_ibt_table_trace_load = ( ptr_uint_t ) 50 , OPTION_IS_INTERNAL_shared_ibt_table_trace_load = ( 0 ) , OPTION_IS_STRING_shared_ibt_table_trace_load = 0 , OPTION_AFFECTS_PCACHE_shared_ibt_table_trace_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_shared_ibt_table_bb_load = ( ptr_uint_t ) 70 , OPTION_IS_INTERNAL_shared_ibt_table_bb_load = ( 0 ) , OPTION_IS_STRING_shared_ibt_table_bb_load = 0 , OPTION_AFFECTS_PCACHE_shared_ibt_table_bb_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_htable_load = ( ptr_uint_t ) 80 , OPTION_IS_INTERNAL_coarse_htable_load = ( 0 ) , OPTION_IS_STRING_coarse_htable_load = 0 , OPTION_AFFECTS_PCACHE_coarse_htable_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_th_htable_load = ( ptr_uint_t ) 80 , OPTION_IS_INTERNAL_coarse_th_htable_load = ( 0 ) , OPTION_IS_STRING_coarse_th_htable_load = 0 , OPTION_AFFECTS_PCACHE_coarse_th_htable_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_pclookup_htable_load = ( ptr_uint_t ) 80 , OPTION_IS_INTERNAL_coarse_pclookup_htable_load = ( 0 ) , OPTION_IS_STRING_coarse_pclookup_htable_load = 0 , OPTION_AFFECTS_PCACHE_coarse_pclookup_htable_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_bb_ibt_groom = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_bb_ibt_groom = ( 0 ) , OPTION_IS_STRING_bb_ibt_groom = 0 , OPTION_AFFECTS_PCACHE_bb_ibt_groom = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_trace_ibt_groom = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_trace_ibt_groom = ( 0 ) , OPTION_IS_STRING_trace_ibt_groom = 0 , OPTION_AFFECTS_PCACHE_trace_ibt_groom = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_trace_ibl_targets_init = ( ptr_uint_t ) 7 , OPTION_IS_INTERNAL_private_trace_ibl_targets_init = ( 0 ) , OPTION_IS_STRING_private_trace_ibl_targets_init = 0 , OPTION_AFFECTS_PCACHE_private_trace_ibl_targets_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_bb_ibl_targets_init = ( ptr_uint_t ) 6 , OPTION_IS_INTERNAL_private_bb_ibl_targets_init = ( 0 ) , OPTION_IS_STRING_private_bb_ibl_targets_init = 0 , OPTION_AFFECTS_PCACHE_private_bb_ibl_targets_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_trace_ibl_targets_max = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_private_trace_ibl_targets_max = ( 0 ) , OPTION_IS_STRING_private_trace_ibl_targets_max = 0 , OPTION_AFFECTS_PCACHE_private_trace_ibl_targets_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_bb_ibl_targets_max = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_private_bb_ibl_targets_max = ( 0 ) , OPTION_IS_STRING_private_bb_ibl_targets_max = 0 , OPTION_AFFECTS_PCACHE_private_bb_ibl_targets_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_bb_load = ( ptr_uint_t ) 60 , OPTION_IS_INTERNAL_private_bb_load = ( 1 ) , OPTION_IS_STRING_private_bb_load = 0 , OPTION_AFFECTS_PCACHE_private_bb_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_private_future_load = ( ptr_uint_t ) 65 , OPTION_IS_INTERNAL_private_future_load = ( 1 ) , OPTION_IS_STRING_private_future_load = 0 , OPTION_AFFECTS_PCACHE_private_future_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_spin_yield_mutex = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_spin_yield_mutex = ( 1 ) , OPTION_IS_STRING_spin_yield_mutex = 0 , OPTION_AFFECTS_PCACHE_spin_yield_mutex = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_spin_yield_rwlock = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_spin_yield_rwlock = ( 1 ) , OPTION_IS_STRING_spin_yield_rwlock = 0 , OPTION_AFFECTS_PCACHE_spin_yield_rwlock = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_simulate_contention = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_simulate_contention = ( 1 ) , OPTION_IS_STRING_simulate_contention = 0 , OPTION_AFFECTS_PCACHE_simulate_contention = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_initial_heap_unit_size = ( ptr_uint_t ) 32 * 1024 , OPTION_IS_INTERNAL_initial_heap_unit_size = ( 1 ) , OPTION_IS_STRING_initial_heap_unit_size = 0 , OPTION_AFFECTS_PCACHE_initial_heap_unit_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_initial_global_heap_unit_size = ( ptr_uint_t ) 32 * 1024 , OPTION_IS_INTERNAL_initial_global_heap_unit_size = ( 1 ) , OPTION_IS_STRING_initial_global_heap_unit_size = 0 , OPTION_AFFECTS_PCACHE_initial_global_heap_unit_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_max_heap_unit_size = ( ptr_uint_t ) 256 * 1024 , OPTION_IS_INTERNAL_max_heap_unit_size = ( 1 ) , OPTION_IS_STRING_max_heap_unit_size = 0 , OPTION_AFFECTS_PCACHE_max_heap_unit_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_heap_commit_increment = ( ptr_uint_t ) 4 * 1024 , OPTION_IS_INTERNAL_heap_commit_increment = ( 0 ) , OPTION_IS_STRING_heap_commit_increment = 0 , OPTION_AFFECTS_PCACHE_heap_commit_increment = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_commit_increment = ( ptr_uint_t ) 4 * 1024 , OPTION_IS_INTERNAL_cache_commit_increment = ( 0 ) , OPTION_IS_STRING_cache_commit_increment = 0 , OPTION_AFFECTS_PCACHE_cache_commit_increment = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_bb_max = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_cache_bb_max = ( 0 ) , OPTION_IS_STRING_cache_bb_max = 0 , OPTION_AFFECTS_PCACHE_cache_bb_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_bb_unit_init = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_bb_unit_init = ( 0 ) , OPTION_IS_STRING_cache_bb_unit_init = 0 , OPTION_AFFECTS_PCACHE_cache_bb_unit_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_bb_unit_max = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_bb_unit_max = ( 0 ) , OPTION_IS_STRING_cache_bb_unit_max = 0 , OPTION_AFFECTS_PCACHE_cache_bb_unit_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_bb_unit_quadruple = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_bb_unit_quadruple = ( 0 ) , OPTION_IS_STRING_cache_bb_unit_quadruple = 0 , OPTION_AFFECTS_PCACHE_cache_bb_unit_quadruple = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_trace_max = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_cache_trace_max = ( 0 ) , OPTION_IS_STRING_cache_trace_max = 0 , OPTION_AFFECTS_PCACHE_cache_trace_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_trace_unit_init = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_trace_unit_init = ( 0 ) , OPTION_IS_STRING_cache_trace_unit_init = 0 , OPTION_AFFECTS_PCACHE_cache_trace_unit_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_trace_unit_max = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_trace_unit_max = ( 0 ) , OPTION_IS_STRING_cache_trace_unit_max = 0 , OPTION_AFFECTS_PCACHE_cache_trace_unit_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_trace_unit_quadruple = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_trace_unit_quadruple = ( 0 ) , OPTION_IS_STRING_cache_trace_unit_quadruple = 0 , OPTION_AFFECTS_PCACHE_cache_trace_unit_quadruple = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_bb_max = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_cache_shared_bb_max = ( 0 ) , OPTION_IS_STRING_cache_shared_bb_max = 0 , OPTION_AFFECTS_PCACHE_cache_shared_bb_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_bb_unit_init = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_shared_bb_unit_init = ( 0 ) , OPTION_IS_STRING_cache_shared_bb_unit_init = 0 , OPTION_AFFECTS_PCACHE_cache_shared_bb_unit_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_bb_unit_max = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_shared_bb_unit_max = ( 0 ) , OPTION_IS_STRING_cache_shared_bb_unit_max = 0 , OPTION_AFFECTS_PCACHE_cache_shared_bb_unit_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_bb_unit_quadruple = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_shared_bb_unit_quadruple = ( 0 ) , OPTION_IS_STRING_cache_shared_bb_unit_quadruple = 0 , OPTION_AFFECTS_PCACHE_cache_shared_bb_unit_quadruple = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_trace_max = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_cache_shared_trace_max = ( 0 ) , OPTION_IS_STRING_cache_shared_trace_max = 0 , OPTION_AFFECTS_PCACHE_cache_shared_trace_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_trace_unit_init = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_shared_trace_unit_init = ( 0 ) , OPTION_IS_STRING_cache_shared_trace_unit_init = 0 , OPTION_AFFECTS_PCACHE_cache_shared_trace_unit_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_trace_unit_max = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_shared_trace_unit_max = ( 0 ) , OPTION_IS_STRING_cache_shared_trace_unit_max = 0 , OPTION_AFFECTS_PCACHE_cache_shared_trace_unit_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_trace_unit_quadruple = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_shared_trace_unit_quadruple = ( 0 ) , OPTION_IS_STRING_cache_shared_trace_unit_quadruple = 0 , OPTION_AFFECTS_PCACHE_cache_shared_trace_unit_quadruple = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_coarse_bb_max = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_cache_coarse_bb_max = ( 0 ) , OPTION_IS_STRING_cache_coarse_bb_max = 0 , OPTION_AFFECTS_PCACHE_cache_coarse_bb_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_coarse_bb_unit_init = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_coarse_bb_unit_init = ( 0 ) , OPTION_IS_STRING_cache_coarse_bb_unit_init = 0 , OPTION_AFFECTS_PCACHE_cache_coarse_bb_unit_init = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_coarse_bb_unit_max = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_coarse_bb_unit_max = ( 0 ) , OPTION_IS_STRING_cache_coarse_bb_unit_max = 0 , OPTION_AFFECTS_PCACHE_cache_coarse_bb_unit_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_coarse_bb_unit_quadruple = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_coarse_bb_unit_quadruple = ( 0 ) , OPTION_IS_STRING_cache_coarse_bb_unit_quadruple = 0 , OPTION_AFFECTS_PCACHE_cache_coarse_bb_unit_quadruple = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_finite_bb_cache = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_finite_bb_cache = ( 0 ) , OPTION_IS_STRING_finite_bb_cache = 0 , OPTION_AFFECTS_PCACHE_finite_bb_cache = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_finite_trace_cache = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_finite_trace_cache = ( 0 ) , OPTION_IS_STRING_finite_trace_cache = 0 , OPTION_AFFECTS_PCACHE_finite_trace_cache = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_finite_shared_bb_cache = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_finite_shared_bb_cache = ( 0 ) , OPTION_IS_STRING_finite_shared_bb_cache = 0 , OPTION_AFFECTS_PCACHE_finite_shared_bb_cache = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_finite_shared_trace_cache = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_finite_shared_trace_cache = ( 0 ) , OPTION_IS_STRING_finite_shared_trace_cache = 0 , OPTION_AFFECTS_PCACHE_finite_shared_trace_cache = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_finite_coarse_bb_cache = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_finite_coarse_bb_cache = ( 0 ) , OPTION_IS_STRING_finite_coarse_bb_cache = 0 , OPTION_AFFECTS_PCACHE_finite_coarse_bb_cache = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_bb_unit_upgrade = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_bb_unit_upgrade = ( 0 ) , OPTION_IS_STRING_cache_bb_unit_upgrade = 0 , OPTION_AFFECTS_PCACHE_cache_bb_unit_upgrade = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_trace_unit_upgrade = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_trace_unit_upgrade = ( 0 ) , OPTION_IS_STRING_cache_trace_unit_upgrade = 0 , OPTION_AFFECTS_PCACHE_cache_trace_unit_upgrade = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_bb_unit_upgrade = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_shared_bb_unit_upgrade = ( 0 ) , OPTION_IS_STRING_cache_shared_bb_unit_upgrade = 0 , OPTION_AFFECTS_PCACHE_cache_shared_bb_unit_upgrade = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_trace_unit_upgrade = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_shared_trace_unit_upgrade = ( 0 ) , OPTION_IS_STRING_cache_shared_trace_unit_upgrade = 0 , OPTION_AFFECTS_PCACHE_cache_shared_trace_unit_upgrade = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_coarse_bb_unit_upgrade = ( ptr_uint_t ) ( 64 * 1024 ) , OPTION_IS_INTERNAL_cache_coarse_bb_unit_upgrade = ( 0 ) , OPTION_IS_STRING_cache_coarse_bb_unit_upgrade = 0 , OPTION_AFFECTS_PCACHE_cache_coarse_bb_unit_upgrade = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_bb_regen = ( ptr_uint_t ) 10 , OPTION_IS_INTERNAL_cache_bb_regen = ( 0 ) , OPTION_IS_STRING_cache_bb_regen = 0 , OPTION_AFFECTS_PCACHE_cache_bb_regen = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_bb_replace = ( ptr_uint_t ) 50 , OPTION_IS_INTERNAL_cache_bb_replace = ( 0 ) , OPTION_IS_STRING_cache_bb_replace = 0 , OPTION_AFFECTS_PCACHE_cache_bb_replace = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_trace_regen = ( ptr_uint_t ) 10 , OPTION_IS_INTERNAL_cache_trace_regen = ( 0 ) , OPTION_IS_STRING_cache_trace_regen = 0 , OPTION_AFFECTS_PCACHE_cache_trace_regen = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_trace_replace = ( ptr_uint_t ) 50 , OPTION_IS_INTERNAL_cache_trace_replace = ( 0 ) , OPTION_IS_STRING_cache_trace_replace = 0 , OPTION_AFFECTS_PCACHE_cache_trace_replace = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_bb_regen = ( ptr_uint_t ) 20 , OPTION_IS_INTERNAL_cache_shared_bb_regen = ( 0 ) , OPTION_IS_STRING_cache_shared_bb_regen = 0 , OPTION_AFFECTS_PCACHE_cache_shared_bb_regen = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_bb_replace = ( ptr_uint_t ) 100 , OPTION_IS_INTERNAL_cache_shared_bb_replace = ( 0 ) , OPTION_IS_STRING_cache_shared_bb_replace = 0 , OPTION_AFFECTS_PCACHE_cache_shared_bb_replace = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_trace_regen = ( ptr_uint_t ) 10 , OPTION_IS_INTERNAL_cache_shared_trace_regen = ( 0 ) , OPTION_IS_STRING_cache_shared_trace_regen = 0 , OPTION_AFFECTS_PCACHE_cache_shared_trace_regen = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_shared_trace_replace = ( ptr_uint_t ) 100 , OPTION_IS_INTERNAL_cache_shared_trace_replace = ( 0 ) , OPTION_IS_STRING_cache_shared_trace_replace = 0 , OPTION_AFFECTS_PCACHE_cache_shared_trace_replace = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_coarse_bb_regen = ( ptr_uint_t ) 20 , OPTION_IS_INTERNAL_cache_coarse_bb_regen = ( 0 ) , OPTION_IS_STRING_cache_coarse_bb_regen = 0 , OPTION_AFFECTS_PCACHE_cache_coarse_bb_regen = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_coarse_bb_replace = ( ptr_uint_t ) 100 , OPTION_IS_INTERNAL_cache_coarse_bb_replace = ( 0 ) , OPTION_IS_STRING_cache_coarse_bb_replace = 0 , OPTION_AFFECTS_PCACHE_cache_coarse_bb_replace = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_trace_align = ( ptr_uint_t ) 8 , OPTION_IS_INTERNAL_cache_trace_align = ( 0 ) , OPTION_IS_STRING_cache_trace_align = 0 , OPTION_AFFECTS_PCACHE_cache_trace_align = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_bb_align = ( ptr_uint_t ) 4 , OPTION_IS_INTERNAL_cache_bb_align = ( 0 ) , OPTION_IS_STRING_cache_bb_align = 0 , OPTION_AFFECTS_PCACHE_cache_bb_align = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_coarse_align = ( ptr_uint_t ) 1 , OPTION_IS_INTERNAL_cache_coarse_align = ( 0 ) , OPTION_IS_STRING_cache_coarse_align = 0 , OPTION_AFFECTS_PCACHE_cache_coarse_align = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_ro2sandbox_threshold = ( ptr_uint_t ) 10 , OPTION_IS_INTERNAL_ro2sandbox_threshold = ( 0 ) , OPTION_IS_STRING_ro2sandbox_threshold = 0 , OPTION_AFFECTS_PCACHE_ro2sandbox_threshold = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_sandbox2ro_threshold = ( ptr_uint_t ) 20 , OPTION_IS_INTERNAL_sandbox2ro_threshold = ( 0 ) , OPTION_IS_STRING_sandbox2ro_threshold = 0 , OPTION_AFFECTS_PCACHE_sandbox2ro_threshold = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_sandbox_writable = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_sandbox_writable = ( 0 ) , OPTION_IS_STRING_sandbox_writable = 0 , OPTION_AFFECTS_PCACHE_sandbox_writable = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_sandbox_non_text = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_sandbox_non_text = ( 0 ) , OPTION_IS_STRING_sandbox_non_text = 0 , OPTION_AFFECTS_PCACHE_sandbox_non_text = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_cache_shared_free_list = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_cache_shared_free_list = ( 0 ) , OPTION_IS_STRING_cache_shared_free_list = 0 , OPTION_AFFECTS_PCACHE_cache_shared_free_list = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_enable_reset = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_enable_reset = ( 0 ) , OPTION_IS_STRING_enable_reset = 0 , OPTION_AFFECTS_PCACHE_enable_reset = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_fragment_count = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_reset_at_fragment_count = ( 1 ) , OPTION_IS_STRING_reset_at_fragment_count = 0 , OPTION_AFFECTS_PCACHE_reset_at_fragment_count = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_nth_thread = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_reset_at_nth_thread = ( 0 ) , OPTION_IS_STRING_reset_at_nth_thread = 0 , OPTION_AFFECTS_PCACHE_reset_at_nth_thread = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_switch_to_os_at_vmm_reset_limit = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_switch_to_os_at_vmm_reset_limit = ( 0 ) , OPTION_IS_STRING_switch_to_os_at_vmm_reset_limit = 0 , OPTION_AFFECTS_PCACHE_switch_to_os_at_vmm_reset_limit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_switch_to_os_at_vmm_limit = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_reset_at_switch_to_os_at_vmm_limit = ( 0 ) , OPTION_IS_STRING_reset_at_switch_to_os_at_vmm_limit = 0 , OPTION_AFFECTS_PCACHE_reset_at_switch_to_os_at_vmm_limit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_vmm_percent_free_limit = ( ptr_uint_t ) 10 , OPTION_IS_INTERNAL_reset_at_vmm_percent_free_limit = ( 0 ) , OPTION_IS_STRING_reset_at_vmm_percent_free_limit = 0 , OPTION_AFFECTS_PCACHE_reset_at_vmm_percent_free_limit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_vmm_free_limit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_reset_at_vmm_free_limit = ( 0 ) , OPTION_IS_STRING_reset_at_vmm_free_limit = 0 , OPTION_AFFECTS_PCACHE_reset_at_vmm_free_limit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_report_reset_vmm_threshold = ( ptr_uint_t ) 3 , OPTION_IS_INTERNAL_report_reset_vmm_threshold = ( 0 ) , OPTION_IS_STRING_report_reset_vmm_threshold = 0 , OPTION_AFFECTS_PCACHE_report_reset_vmm_threshold = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_vmm_full = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_reset_at_vmm_full = ( 0 ) , OPTION_IS_STRING_reset_at_vmm_full = 0 , OPTION_AFFECTS_PCACHE_reset_at_vmm_full = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_commit_percent_free_limit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_reset_at_commit_percent_free_limit = ( 0 ) , OPTION_IS_STRING_reset_at_commit_percent_free_limit = 0 , OPTION_AFFECTS_PCACHE_reset_at_commit_percent_free_limit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_commit_free_limit = ( ptr_uint_t ) ( 32 * 1024 * 1024 ) , OPTION_IS_INTERNAL_reset_at_commit_free_limit = ( 0 ) , OPTION_IS_STRING_reset_at_commit_free_limit = 0 , OPTION_AFFECTS_PCACHE_reset_at_commit_free_limit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_report_reset_commit_threshold = ( ptr_uint_t ) 3 , OPTION_IS_INTERNAL_report_reset_commit_threshold = ( 0 ) , OPTION_IS_STRING_report_reset_commit_threshold = 0 , OPTION_AFFECTS_PCACHE_report_reset_commit_threshold = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_every_nth_pending = ( ptr_uint_t ) 35 , OPTION_IS_INTERNAL_reset_every_nth_pending = ( 0 ) , OPTION_IS_STRING_reset_every_nth_pending = 0 , OPTION_AFFECTS_PCACHE_reset_every_nth_pending = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_nth_bb_unit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_reset_at_nth_bb_unit = ( 0 ) , OPTION_IS_STRING_reset_at_nth_bb_unit = 0 , OPTION_AFFECTS_PCACHE_reset_at_nth_bb_unit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_at_nth_trace_unit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_reset_at_nth_trace_unit = ( 0 ) , OPTION_IS_STRING_reset_at_nth_trace_unit = 0 , OPTION_AFFECTS_PCACHE_reset_at_nth_trace_unit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_every_nth_bb_unit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_reset_every_nth_bb_unit = ( 0 ) , OPTION_IS_STRING_reset_every_nth_bb_unit = 0 , OPTION_AFFECTS_PCACHE_reset_every_nth_bb_unit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reset_every_nth_trace_unit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_reset_every_nth_trace_unit = ( 0 ) , OPTION_IS_STRING_reset_every_nth_trace_unit = 0 , OPTION_AFFECTS_PCACHE_reset_every_nth_trace_unit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_skip_out_of_vm_reserve_curiosity = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_skip_out_of_vm_reserve_curiosity = ( 1 ) , OPTION_IS_STRING_skip_out_of_vm_reserve_curiosity = 0 , OPTION_AFFECTS_PCACHE_skip_out_of_vm_reserve_curiosity = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vm_reserve = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_vm_reserve = ( 0 ) , OPTION_IS_STRING_vm_reserve = 0 , OPTION_AFFECTS_PCACHE_vm_reserve = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vm_size = ( ptr_uint_t ) 128 * 1024 * 1024 , OPTION_IS_INTERNAL_vm_size = ( 0 ) , OPTION_IS_STRING_vm_size = 0 , OPTION_AFFECTS_PCACHE_vm_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vm_base = ( ptr_uint_t ) ( 0x46000000 ) , OPTION_IS_INTERNAL_vm_base = ( 0 ) , OPTION_IS_STRING_vm_base = 0 , OPTION_AFFECTS_PCACHE_vm_base = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vm_max_offset = ( ptr_uint_t ) 0x10000000 , OPTION_IS_INTERNAL_vm_max_offset = ( 0 ) , OPTION_IS_STRING_vm_max_offset = 0 , OPTION_AFFECTS_PCACHE_vm_max_offset = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vm_allow_not_at_base = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_vm_allow_not_at_base = ( 0 ) , OPTION_IS_STRING_vm_allow_not_at_base = 0 , OPTION_AFFECTS_PCACHE_vm_allow_not_at_base = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vm_allow_smaller = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_vm_allow_smaller = ( 0 ) , OPTION_IS_STRING_vm_allow_smaller = 0 , OPTION_AFFECTS_PCACHE_vm_allow_smaller = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vm_base_near_app = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_vm_base_near_app = ( 0 ) , OPTION_IS_STRING_vm_base_near_app = 0 , OPTION_AFFECTS_PCACHE_vm_base_near_app = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_heap_in_lower_4GB = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_heap_in_lower_4GB = ( 0 ) , OPTION_IS_STRING_heap_in_lower_4GB = 0 , OPTION_AFFECTS_PCACHE_heap_in_lower_4GB = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_reachable_heap = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_reachable_heap = ( 0 ) , OPTION_IS_STRING_reachable_heap = 0 , OPTION_AFFECTS_PCACHE_reachable_heap = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vm_use_last = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_vm_use_last = ( 1 ) , OPTION_IS_STRING_vm_use_last = 0 , OPTION_AFFECTS_PCACHE_vm_use_last = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_silent_oom_mask = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_silent_oom_mask = ( 0 ) , OPTION_IS_STRING_silent_oom_mask = 0 , OPTION_AFFECTS_PCACHE_silent_oom_mask = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_silent_commit_oom_list = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_silent_commit_oom_list = ( 0 ) , OPTION_IS_STRING_silent_commit_oom_list = 1 , OPTION_AFFECTS_PCACHE_silent_commit_oom_list = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_oom_timeout = ( ptr_uint_t ) 5 * 1000 , OPTION_IS_INTERNAL_oom_timeout = ( 0 ) , OPTION_IS_STRING_oom_timeout = 0 , OPTION_AFFECTS_PCACHE_oom_timeout = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_follow_children = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_follow_children = ( 0 ) , OPTION_IS_STRING_follow_children = 0 , OPTION_AFFECTS_PCACHE_follow_children = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_follow_systemwide = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_follow_systemwide = ( 0 ) , OPTION_IS_STRING_follow_systemwide = 0 , OPTION_AFFECTS_PCACHE_follow_systemwide = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_follow_explicit_children = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_follow_explicit_children = ( 0 ) , OPTION_IS_STRING_follow_explicit_children = 0 , OPTION_AFFECTS_PCACHE_follow_explicit_children = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_early_inject = ( ptr_uint_t ) ( ( 0 ) ) , OPTION_IS_INTERNAL_early_inject = ( 0 ) , OPTION_IS_STRING_early_inject = 0 , OPTION_AFFECTS_PCACHE_early_inject = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_early_inject_map = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_early_inject_map = ( 0 ) , OPTION_IS_STRING_early_inject_map = 0 , OPTION_AFFECTS_PCACHE_early_inject_map = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_early_inject_location = ( ptr_uint_t ) 4 , OPTION_IS_INTERNAL_early_inject_location = ( 0 ) , OPTION_IS_STRING_early_inject_location = 0 , OPTION_AFFECTS_PCACHE_early_inject_location = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_early_inject_address = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_early_inject_address = ( 0 ) , OPTION_IS_STRING_early_inject_address = 0 , OPTION_AFFECTS_PCACHE_early_inject_address = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_early_inject_stress_helpers = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_early_inject_stress_helpers = ( 1 ) , OPTION_IS_STRING_early_inject_stress_helpers = 0 , OPTION_AFFECTS_PCACHE_early_inject_stress_helpers = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_inject_at_create_process = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_inject_at_create_process = ( 0 ) , OPTION_IS_STRING_inject_at_create_process = 0 , OPTION_AFFECTS_PCACHE_inject_at_create_process = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vista_inject_at_create_process = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_vista_inject_at_create_process = ( 0 ) , OPTION_IS_STRING_vista_inject_at_create_process = 0 , OPTION_AFFECTS_PCACHE_vista_inject_at_create_process = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_inject_primary = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_inject_primary = ( 0 ) , OPTION_IS_STRING_inject_primary = 0 , OPTION_AFFECTS_PCACHE_inject_primary = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_synch_thread_max_loops = ( ptr_uint_t ) 10000 , OPTION_IS_INTERNAL_synch_thread_max_loops = ( 0 ) , OPTION_IS_STRING_synch_thread_max_loops = 0 , OPTION_AFFECTS_PCACHE_synch_thread_max_loops = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_synch_all_threads_max_loops = ( ptr_uint_t ) 10000 , OPTION_IS_INTERNAL_synch_all_threads_max_loops = ( 0 ) , OPTION_IS_STRING_synch_all_threads_max_loops = 0 , OPTION_AFFECTS_PCACHE_synch_all_threads_max_loops = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_synch_thread_sleep_UP = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_synch_thread_sleep_UP = ( 0 ) , OPTION_IS_STRING_synch_thread_sleep_UP = 0 , OPTION_AFFECTS_PCACHE_synch_thread_sleep_UP = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_synch_thread_sleep_MP = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_synch_thread_sleep_MP = ( 0 ) , OPTION_IS_STRING_synch_thread_sleep_MP = 0 , OPTION_AFFECTS_PCACHE_synch_thread_sleep_MP = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_synch_with_sleep_time = ( ptr_uint_t ) 5 , OPTION_IS_INTERNAL_synch_with_sleep_time = ( 0 ) , OPTION_IS_STRING_synch_with_sleep_time = 0 , OPTION_AFFECTS_PCACHE_synch_with_sleep_time = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_ignore_syscalls = ( ptr_uint_t ) ( ( 1 ) ) , OPTION_IS_INTERNAL_ignore_syscalls = ( 0 ) , OPTION_IS_STRING_ignore_syscalls = 0 , OPTION_AFFECTS_PCACHE_ignore_syscalls = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_inline_ignored_syscalls = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_inline_ignored_syscalls = ( 0 ) , OPTION_IS_STRING_inline_ignored_syscalls = 0 , OPTION_AFFECTS_PCACHE_inline_ignored_syscalls = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_hook_vsyscall = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_hook_vsyscall = ( 0 ) , OPTION_IS_STRING_hook_vsyscall = 0 , OPTION_AFFECTS_PCACHE_hook_vsyscall = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_sysenter_is_int80_aka_hook_vsyscall = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_sysenter_is_int80_aka_hook_vsyscall = ( 0 ) , OPTION_IS_STRING_sysenter_is_int80_aka_hook_vsyscall = 0 , OPTION_AFFECTS_PCACHE_sysenter_is_int80_aka_hook_vsyscall = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_restart_syscalls = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_restart_syscalls = ( 0 ) , OPTION_IS_STRING_restart_syscalls = 0 , OPTION_AFFECTS_PCACHE_restart_syscalls = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_guard_pages = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_guard_pages = ( 0 ) , OPTION_IS_STRING_guard_pages = 0 , OPTION_AFFECTS_PCACHE_guard_pages = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_enable_block_mod_load = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_enable_block_mod_load = ( 0 ) , OPTION_IS_STRING_enable_block_mod_load = 0 , OPTION_AFFECTS_PCACHE_enable_block_mod_load = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_block_mod_load_list_default = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_block_mod_load_list_default = ( 0 ) , OPTION_IS_STRING_block_mod_load_list_default = 1 , OPTION_AFFECTS_PCACHE_block_mod_load_list_default = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_block_mod_load_list = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_block_mod_load_list = ( 0 ) , OPTION_IS_STRING_block_mod_load_list = 1 , OPTION_AFFECTS_PCACHE_block_mod_load_list = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_handle_DR_modify = ( ptr_uint_t ) 1 , OPTION_IS_INTERNAL_handle_DR_modify = ( 0 ) , OPTION_IS_STRING_handle_DR_modify = 0 , OPTION_AFFECTS_PCACHE_handle_DR_modify = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_handle_ntdll_modify = ( ptr_uint_t ) 1 , OPTION_IS_INTERNAL_handle_ntdll_modify = ( 0 ) , OPTION_IS_STRING_handle_ntdll_modify = 0 , OPTION_AFFECTS_PCACHE_handle_ntdll_modify = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_patch_proof_default_list = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_patch_proof_default_list = ( 0 ) , OPTION_IS_STRING_patch_proof_default_list = 1 , OPTION_AFFECTS_PCACHE_patch_proof_default_list = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_patch_proof_list = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_patch_proof_list = ( 0 ) , OPTION_IS_STRING_patch_proof_list = 1 , OPTION_AFFECTS_PCACHE_patch_proof_list = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_use_moduledb = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_use_moduledb = ( 0 ) , OPTION_IS_STRING_use_moduledb = 0 , OPTION_AFFECTS_PCACHE_use_moduledb = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_staged_aka_use_moduledb = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_staged_aka_use_moduledb = ( 0 ) , OPTION_IS_STRING_staged_aka_use_moduledb = 0 , OPTION_AFFECTS_PCACHE_staged_aka_use_moduledb = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_whitelist_company_names_default = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_whitelist_company_names_default = ( 0 ) , OPTION_IS_STRING_whitelist_company_names_default = 1 , OPTION_AFFECTS_PCACHE_whitelist_company_names_default = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_whitelist_company_names = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_whitelist_company_names = ( 0 ) , OPTION_IS_STRING_whitelist_company_names = 1 , OPTION_AFFECTS_PCACHE_whitelist_company_names = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_unknown_module_policy = ( ptr_uint_t ) 0xf , OPTION_IS_INTERNAL_unknown_module_policy = ( 0 ) , OPTION_IS_STRING_unknown_module_policy = 0 , OPTION_AFFECTS_PCACHE_unknown_module_policy = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_unknown_module_load_report_max = ( ptr_uint_t ) 10 , OPTION_IS_INTERNAL_unknown_module_load_report_max = ( 0 ) , OPTION_IS_STRING_unknown_module_load_report_max = 0 , OPTION_AFFECTS_PCACHE_unknown_module_load_report_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_moduledb_exemptions_report_max = ( ptr_uint_t ) 3 , OPTION_IS_INTERNAL_moduledb_exemptions_report_max = ( 0 ) , OPTION_IS_STRING_moduledb_exemptions_report_max = 0 , OPTION_AFFECTS_PCACHE_moduledb_exemptions_report_max = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unloaded_target_exception = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_unloaded_target_exception = ( 0 ) , OPTION_IS_STRING_unloaded_target_exception = 0 , OPTION_AFFECTS_PCACHE_unloaded_target_exception = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_cache_consistency = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_cache_consistency = ( 1 ) , OPTION_IS_STRING_cache_consistency = 0 , OPTION_AFFECTS_PCACHE_cache_consistency = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_sandbox_writes = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_sandbox_writes = ( 1 ) , OPTION_IS_STRING_sandbox_writes = 0 , OPTION_AFFECTS_PCACHE_sandbox_writes = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_safe_translate_flushed = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_safe_translate_flushed = ( 1 ) , OPTION_IS_STRING_safe_translate_flushed = 0 , OPTION_AFFECTS_PCACHE_safe_translate_flushed = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_store_translations = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_store_translations = ( 1 ) , OPTION_IS_STRING_store_translations = 0 , OPTION_AFFECTS_PCACHE_store_translations = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_translate_fpu_pc = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_translate_fpu_pc = ( 0 ) , OPTION_IS_STRING_translate_fpu_pc = 0 , OPTION_AFFECTS_PCACHE_translate_fpu_pc = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_validate_owner_dir = ( ptr_uint_t ) ( ( 0 ) ) , OPTION_IS_INTERNAL_validate_owner_dir = ( 0 ) , OPTION_IS_STRING_validate_owner_dir = 0 , OPTION_AFFECTS_PCACHE_validate_owner_dir = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_validate_owner_file = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_validate_owner_file = ( 0 ) , OPTION_IS_STRING_validate_owner_file = 0 , OPTION_AFFECTS_PCACHE_validate_owner_file = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_units = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_coarse_units = ( 0 ) , OPTION_IS_STRING_coarse_units = 0 , OPTION_AFFECTS_PCACHE_coarse_units = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_enable_full_api_aka_coarse_units = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_enable_full_api_aka_coarse_units = ( 0 ) , OPTION_IS_STRING_enable_full_api_aka_coarse_units = 0 , OPTION_AFFECTS_PCACHE_enable_full_api_aka_coarse_units = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_coarse_enable_freeze = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_coarse_enable_freeze = ( 0 ) , OPTION_IS_STRING_coarse_enable_freeze = 0 , OPTION_AFFECTS_PCACHE_coarse_enable_freeze = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_at_exit = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_coarse_freeze_at_exit = ( 0 ) , OPTION_IS_STRING_coarse_freeze_at_exit = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_at_exit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_at_unload = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_coarse_freeze_at_unload = ( 0 ) , OPTION_IS_STRING_coarse_freeze_at_unload = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_at_unload = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_min_size = ( ptr_uint_t ) 512 , OPTION_IS_INTERNAL_coarse_freeze_min_size = ( 0 ) , OPTION_IS_STRING_coarse_freeze_min_size = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_min_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_append_size = ( ptr_uint_t ) 256 , OPTION_IS_INTERNAL_coarse_freeze_append_size = ( 0 ) , OPTION_IS_STRING_coarse_freeze_append_size = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_append_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_rct_min = ( ptr_uint_t ) 2 * 1024 , OPTION_IS_INTERNAL_coarse_freeze_rct_min = ( 0 ) , OPTION_IS_STRING_coarse_freeze_rct_min = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_rct_min = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_clobber = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_coarse_freeze_clobber = ( 0 ) , OPTION_IS_STRING_coarse_freeze_clobber = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_clobber = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_rename = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_coarse_freeze_rename = ( 0 ) , OPTION_IS_STRING_coarse_freeze_rename = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_rename = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_clean = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_coarse_freeze_clean = ( 0 ) , OPTION_IS_STRING_coarse_freeze_clean = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_clean = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_merge = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_coarse_freeze_merge = ( 0 ) , OPTION_IS_STRING_coarse_freeze_merge = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_merge = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_lone_merge = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_coarse_lone_merge = ( 0 ) , OPTION_IS_STRING_coarse_lone_merge = 0 , OPTION_AFFECTS_PCACHE_coarse_lone_merge = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_disk_merge = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_coarse_disk_merge = ( 0 ) , OPTION_IS_STRING_coarse_disk_merge = 0 , OPTION_AFFECTS_PCACHE_coarse_disk_merge = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_freeze_elide_ubr = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_coarse_freeze_elide_ubr = ( 0 ) , OPTION_IS_STRING_coarse_freeze_elide_ubr = 0 , OPTION_AFFECTS_PCACHE_coarse_freeze_elide_ubr = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unsafe_freeze_elide_sole_ubr = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_unsafe_freeze_elide_sole_ubr = ( 0 ) , OPTION_IS_STRING_unsafe_freeze_elide_sole_ubr = 0 , OPTION_AFFECTS_PCACHE_unsafe_freeze_elide_sole_ubr = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_coarse_pclookup_table = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_coarse_pclookup_table = ( 0 ) , OPTION_IS_STRING_coarse_pclookup_table = 0 , OPTION_AFFECTS_PCACHE_coarse_pclookup_table = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_per_app = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_persist_per_app = ( 0 ) , OPTION_IS_STRING_persist_per_app = 0 , OPTION_AFFECTS_PCACHE_persist_per_app = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_per_user = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_persist_per_user = ( 0 ) , OPTION_IS_STRING_persist_per_user = 0 , OPTION_AFFECTS_PCACHE_persist_per_user = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_use_persisted = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_use_persisted = ( 0 ) , OPTION_IS_STRING_use_persisted = 0 , OPTION_AFFECTS_PCACHE_use_persisted = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_exclude_list = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_persist_exclude_list = ( 0 ) , OPTION_IS_STRING_persist_exclude_list = 1 , OPTION_AFFECTS_PCACHE_persist_exclude_list = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_fill_ibl = ( ptr_uint_t ) 1 , OPTION_IS_INTERNAL_coarse_fill_ibl = ( 0 ) , OPTION_IS_STRING_coarse_fill_ibl = 0 , OPTION_AFFECTS_PCACHE_coarse_fill_ibl = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_map_rw_separate = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_persist_map_rw_separate = ( 0 ) , OPTION_IS_STRING_persist_map_rw_separate = 0 , OPTION_AFFECTS_PCACHE_persist_map_rw_separate = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_lock_file = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_persist_lock_file = ( 0 ) , OPTION_IS_STRING_persist_lock_file = 0 , OPTION_AFFECTS_PCACHE_persist_lock_file = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_gen_validation = ( ptr_uint_t ) ( 0xd ) , OPTION_IS_INTERNAL_persist_gen_validation = ( 0 ) , OPTION_IS_STRING_persist_gen_validation = 0 , OPTION_AFFECTS_PCACHE_persist_gen_validation = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_load_validation = ( ptr_uint_t ) 0x5 , OPTION_IS_INTERNAL_persist_load_validation = ( 0 ) , OPTION_IS_STRING_persist_load_validation = 0 , OPTION_AFFECTS_PCACHE_persist_load_validation = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_short_digest = ( ptr_uint_t ) ( 4 * 1024 ) , OPTION_IS_INTERNAL_persist_short_digest = ( 0 ) , OPTION_IS_STRING_persist_short_digest = 0 , OPTION_AFFECTS_PCACHE_persist_short_digest = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_persist_check_options = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_persist_check_options = ( 0 ) , OPTION_IS_STRING_persist_check_options = 0 , OPTION_AFFECTS_PCACHE_persist_check_options = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_check_local_options = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_persist_check_local_options = ( 0 ) , OPTION_IS_STRING_persist_check_local_options = 0 , OPTION_AFFECTS_PCACHE_persist_check_local_options = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_persist_check_exempted_options = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_persist_check_exempted_options = ( 0 ) , OPTION_IS_STRING_persist_check_exempted_options = 0 , OPTION_AFFECTS_PCACHE_persist_check_exempted_options = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_persist_protect_stubs = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_persist_protect_stubs = ( 0 ) , OPTION_IS_STRING_persist_protect_stubs = 0 , OPTION_AFFECTS_PCACHE_persist_protect_stubs = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_protect_stubs_limit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_persist_protect_stubs_limit = ( 0 ) , OPTION_IS_STRING_persist_protect_stubs_limit = 0 , OPTION_AFFECTS_PCACHE_persist_protect_stubs_limit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_touch_stubs = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_persist_touch_stubs = ( 0 ) , OPTION_IS_STRING_persist_touch_stubs = 0 , OPTION_AFFECTS_PCACHE_persist_touch_stubs = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_coarse_merge_iat = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_coarse_merge_iat = ( 0 ) , OPTION_IS_STRING_coarse_merge_iat = 0 , OPTION_AFFECTS_PCACHE_coarse_merge_iat = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_coarse_split_calls = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_coarse_split_calls = ( 0 ) , OPTION_IS_STRING_coarse_split_calls = 0 , OPTION_AFFECTS_PCACHE_coarse_split_calls = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_coarse_split_riprel = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_coarse_split_riprel = ( 0 ) , OPTION_IS_STRING_coarse_split_riprel = 0 , OPTION_AFFECTS_PCACHE_coarse_split_riprel = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_persist_trust_textrel = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_persist_trust_textrel = ( 0 ) , OPTION_IS_STRING_persist_trust_textrel = 0 , OPTION_AFFECTS_PCACHE_persist_trust_textrel = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_dir = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_persist_dir = ( 0 ) , OPTION_IS_STRING_persist_dir = 1 , OPTION_AFFECTS_PCACHE_persist_dir = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist_shared_dir = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_persist_shared_dir = ( 0 ) , OPTION_IS_STRING_persist_shared_dir = 1 , OPTION_AFFECTS_PCACHE_persist_shared_dir = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_persist = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_persist = ( 0 ) , OPTION_IS_STRING_persist = 0 , OPTION_AFFECTS_PCACHE_persist = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_desktop = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_desktop = ( 0 ) , OPTION_IS_STRING_desktop = 0 , OPTION_AFFECTS_PCACHE_desktop = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_native_exec = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_native_exec = ( 0 ) , OPTION_IS_STRING_native_exec = 0 , OPTION_AFFECTS_PCACHE_native_exec = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_native_exec_default_list = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_native_exec_default_list = ( 0 ) , OPTION_IS_STRING_native_exec_default_list = 1 , OPTION_AFFECTS_PCACHE_native_exec_default_list = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_native_exec_list = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_native_exec_list = ( 0 ) , OPTION_IS_STRING_native_exec_list = 1 , OPTION_AFFECTS_PCACHE_native_exec_list = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_native_exec_syscalls = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_native_exec_syscalls = ( 0 ) , OPTION_IS_STRING_native_exec_syscalls = 0 , OPTION_AFFECTS_PCACHE_native_exec_syscalls = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_native_exec_dircalls = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_native_exec_dircalls = ( 0 ) , OPTION_IS_STRING_native_exec_dircalls = 0 , OPTION_AFFECTS_PCACHE_native_exec_dircalls = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_native_exec_callcall = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_native_exec_callcall = ( 0 ) , OPTION_IS_STRING_native_exec_callcall = 0 , OPTION_AFFECTS_PCACHE_native_exec_callcall = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_native_exec_guess_calls = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_native_exec_guess_calls = ( 0 ) , OPTION_IS_STRING_native_exec_guess_calls = 0 , OPTION_AFFECTS_PCACHE_native_exec_guess_calls = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_native_exec_managed_code = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_native_exec_managed_code = ( 0 ) , OPTION_IS_STRING_native_exec_managed_code = 0 , OPTION_AFFECTS_PCACHE_native_exec_managed_code = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_native_exec_dot_pexe = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_native_exec_dot_pexe = ( 0 ) , OPTION_IS_STRING_native_exec_dot_pexe = 0 , OPTION_AFFECTS_PCACHE_native_exec_dot_pexe = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_native_exec_retakeover = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_native_exec_retakeover = ( 0 ) , OPTION_IS_STRING_native_exec_retakeover = 0 , OPTION_AFFECTS_PCACHE_native_exec_retakeover = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_inline_calls = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_inline_calls = ( 1 ) , OPTION_IS_STRING_inline_calls = 0 , OPTION_AFFECTS_PCACHE_inline_calls = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_indcall2direct = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_indcall2direct = ( 0 ) , OPTION_IS_STRING_indcall2direct = 0 , OPTION_AFFECTS_PCACHE_indcall2direct = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_IAT_convert = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_IAT_convert = ( 0 ) , OPTION_IS_STRING_IAT_convert = 0 , OPTION_AFFECTS_PCACHE_IAT_convert = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_IAT_elide = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_IAT_elide = ( 0 ) , OPTION_IS_STRING_IAT_elide = 0 , OPTION_AFFECTS_PCACHE_IAT_elide = OP_PCACHE_GLOBAL , OPTION_DEFAULT_VALUE_unsafe_IAT_ignore_hooker = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_unsafe_IAT_ignore_hooker = ( 1 ) , OPTION_IS_STRING_unsafe_IAT_ignore_hooker = 0 , OPTION_AFFECTS_PCACHE_unsafe_IAT_ignore_hooker = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_thread_policy = ( ptr_uint_t ) OPTION_DISABLED | OPTION_NO_BLOCK | OPTION_NO_REPORT | OPTION_NO_CUSTOM , OPTION_IS_INTERNAL_thread_policy = ( 0 ) , OPTION_IS_STRING_thread_policy = 0 , OPTION_AFFECTS_PCACHE_thread_policy = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_min_free_disk = ( ptr_uint_t ) 50 * ( 1024 * 1024 ) , OPTION_IS_INTERNAL_min_free_disk = ( 0 ) , OPTION_IS_STRING_min_free_disk = 0 , OPTION_AFFECTS_PCACHE_min_free_disk = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_prng_seed = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_prng_seed = ( 0 ) , OPTION_IS_STRING_prng_seed = 0 , OPTION_AFFECTS_PCACHE_prng_seed = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_pad_jmps = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_pad_jmps = ( 0 ) , OPTION_IS_STRING_pad_jmps = 0 , OPTION_AFFECTS_PCACHE_pad_jmps = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_pad_jmps_mark_no_trace = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_pad_jmps_mark_no_trace = ( 0 ) , OPTION_IS_STRING_pad_jmps_mark_no_trace = 0 , OPTION_AFFECTS_PCACHE_pad_jmps_mark_no_trace = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_pad_jmps_return_excess_padding = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_pad_jmps_return_excess_padding = ( 1 ) , OPTION_IS_STRING_pad_jmps_return_excess_padding = 0 , OPTION_AFFECTS_PCACHE_pad_jmps_return_excess_padding = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_pad_jmps_shift_bb = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_pad_jmps_shift_bb = ( 1 ) , OPTION_IS_STRING_pad_jmps_shift_bb = 0 , OPTION_AFFECTS_PCACHE_pad_jmps_shift_bb = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_pad_jmps_shift_trace = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_pad_jmps_shift_trace = ( 1 ) , OPTION_IS_STRING_pad_jmps_shift_trace = 0 , OPTION_AFFECTS_PCACHE_pad_jmps_shift_trace = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_pad_jmps_set_alignment = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_pad_jmps_set_alignment = ( 1 ) , OPTION_IS_STRING_pad_jmps_set_alignment = 0 , OPTION_AFFECTS_PCACHE_pad_jmps_set_alignment = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_ibl_sentinel_check = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_ibl_sentinel_check = ( 1 ) , OPTION_IS_STRING_ibl_sentinel_check = 0 , OPTION_AFFECTS_PCACHE_ibl_sentinel_check = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_ibl_addr_prefix = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_ibl_addr_prefix = ( 0 ) , OPTION_IS_STRING_ibl_addr_prefix = 0 , OPTION_AFFECTS_PCACHE_ibl_addr_prefix = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_slowdown_ibl_found = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_slowdown_ibl_found = ( 1 ) , OPTION_IS_STRING_slowdown_ibl_found = 0 , OPTION_AFFECTS_PCACHE_slowdown_ibl_found = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_stress_recreate_pc = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_stress_recreate_pc = ( 1 ) , OPTION_IS_STRING_stress_recreate_pc = 0 , OPTION_AFFECTS_PCACHE_stress_recreate_pc = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_stress_recreate_state = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_stress_recreate_state = ( 1 ) , OPTION_IS_STRING_stress_recreate_state = 0 , OPTION_AFFECTS_PCACHE_stress_recreate_state = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_detect_dangling_fcache = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_detect_dangling_fcache = ( 1 ) , OPTION_IS_STRING_detect_dangling_fcache = 0 , OPTION_AFFECTS_PCACHE_detect_dangling_fcache = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_stress_detach_with_stacked_callbacks = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_stress_detach_with_stacked_callbacks = ( 1 ) , OPTION_IS_STRING_stress_detach_with_stacked_callbacks = 0 , OPTION_AFFECTS_PCACHE_stress_detach_with_stacked_callbacks = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_detach_fix_sysenter_on_stack = ( ptr_uint_t ) ( 1 ) , OPTION_IS_INTERNAL_detach_fix_sysenter_on_stack = ( 1 ) , OPTION_IS_STRING_detach_fix_sysenter_on_stack = 0 , OPTION_AFFECTS_PCACHE_detach_fix_sysenter_on_stack = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vmarea_initial_size = ( ptr_uint_t ) 100 , OPTION_IS_INTERNAL_vmarea_initial_size = ( 1 ) , OPTION_IS_STRING_vmarea_initial_size = 0 , OPTION_AFFECTS_PCACHE_vmarea_initial_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vmarea_increment_size = ( ptr_uint_t ) 100 , OPTION_IS_INTERNAL_vmarea_increment_size = ( 1 ) , OPTION_IS_STRING_vmarea_increment_size = 0 , OPTION_AFFECTS_PCACHE_vmarea_increment_size = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_stress_fake_userva = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_stress_fake_userva = ( 1 ) , OPTION_IS_STRING_stress_fake_userva = 0 , OPTION_AFFECTS_PCACHE_stress_fake_userva = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unsafe_crash_process = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_unsafe_crash_process = ( 1 ) , OPTION_IS_STRING_unsafe_crash_process = 0 , OPTION_AFFECTS_PCACHE_unsafe_crash_process = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unsafe_hang_process = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_unsafe_hang_process = ( 1 ) , OPTION_IS_STRING_unsafe_hang_process = 0 , OPTION_AFFECTS_PCACHE_unsafe_hang_process = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unsafe_ignore_overflow = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_unsafe_ignore_overflow = ( 1 ) , OPTION_IS_STRING_unsafe_ignore_overflow = 0 , OPTION_AFFECTS_PCACHE_unsafe_ignore_overflow = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unsafe_ignore_eflags = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_unsafe_ignore_eflags = ( 1 ) , OPTION_IS_STRING_unsafe_ignore_eflags = 0 , OPTION_AFFECTS_PCACHE_unsafe_ignore_eflags = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unsafe_ignore_eflags_trace = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_unsafe_ignore_eflags_trace = ( 1 ) , OPTION_IS_STRING_unsafe_ignore_eflags_trace = 0 , OPTION_AFFECTS_PCACHE_unsafe_ignore_eflags_trace = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unsafe_ignore_eflags_prefix = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_unsafe_ignore_eflags_prefix = ( 1 ) , OPTION_IS_STRING_unsafe_ignore_eflags_prefix = 0 , OPTION_AFFECTS_PCACHE_unsafe_ignore_eflags_prefix = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unsafe_ignore_eflags_ibl = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_unsafe_ignore_eflags_ibl = ( 1 ) , OPTION_IS_STRING_unsafe_ignore_eflags_ibl = 0 , OPTION_AFFECTS_PCACHE_unsafe_ignore_eflags_ibl = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_ignore_assert_list = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_ignore_assert_list = ( 0 ) , OPTION_IS_STRING_ignore_assert_list = 1 , OPTION_AFFECTS_PCACHE_ignore_assert_list = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_synch_at_exit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_synch_at_exit = ( 0 ) , OPTION_IS_STRING_synch_at_exit = 0 , OPTION_AFFECTS_PCACHE_synch_at_exit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_multi_thread_exit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_multi_thread_exit = ( 0 ) , OPTION_IS_STRING_multi_thread_exit = 0 , OPTION_AFFECTS_PCACHE_multi_thread_exit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_skip_thread_exit_at_exit = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_skip_thread_exit_at_exit = ( 0 ) , OPTION_IS_STRING_skip_thread_exit_at_exit = 0 , OPTION_AFFECTS_PCACHE_skip_thread_exit_at_exit = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_optimize = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_optimize = ( 0 ) , OPTION_IS_STRING_optimize = 0 , OPTION_AFFECTS_PCACHE_optimize = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_prefetch = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_prefetch = ( 0 ) , OPTION_IS_STRING_prefetch = 0 , OPTION_AFFECTS_PCACHE_prefetch = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_rlr = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_rlr = ( 0 ) , OPTION_IS_STRING_rlr = 0 , OPTION_AFFECTS_PCACHE_rlr = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_vectorize = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_vectorize = ( 0 ) , OPTION_IS_STRING_vectorize = 0 , OPTION_AFFECTS_PCACHE_vectorize = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_unroll_loops = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_unroll_loops = ( 0 ) , OPTION_IS_STRING_unroll_loops = 0 , OPTION_AFFECTS_PCACHE_unroll_loops = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_instr_counts = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_instr_counts = ( 0 ) , OPTION_IS_STRING_instr_counts = 0 , OPTION_AFFECTS_PCACHE_instr_counts = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_stack_adjust = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_stack_adjust = ( 0 ) , OPTION_IS_STRING_stack_adjust = 0 , OPTION_AFFECTS_PCACHE_stack_adjust = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_remove_dead_code = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_remove_dead_code = ( 0 ) , OPTION_IS_STRING_remove_dead_code = 0 , OPTION_AFFECTS_PCACHE_remove_dead_code = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_constant_prop = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_constant_prop = ( 0 ) , OPTION_IS_STRING_constant_prop = 0 , OPTION_AFFECTS_PCACHE_constant_prop = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_call_return_matching = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_call_return_matching = ( 0 ) , OPTION_IS_STRING_call_return_matching = 0 , OPTION_AFFECTS_PCACHE_call_return_matching = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_remove_unnecessary_zeroing = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_remove_unnecessary_zeroing = ( 0 ) , OPTION_IS_STRING_remove_unnecessary_zeroing = 0 , OPTION_AFFECTS_PCACHE_remove_unnecessary_zeroing = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_peephole = ( ptr_uint_t ) 0 , OPTION_IS_INTERNAL_peephole = ( 0 ) , OPTION_IS_STRING_peephole = 0 , OPTION_AFFECTS_PCACHE_peephole = OP_PCACHE_NOP , OPTION_DEFAULT_VALUE_thin_client = ( ptr_uint_t ) ( 0 ) , OPTION_IS_INTERNAL_thin_client = ( 0 ) , OPTION_IS_STRING_thin_client = 0 , OPTION_AFFECTS_PCACHE_thin_client = OP_PCACHE_NOP , } ;
typedef struct _options_t { bool dynamic_options ; bool dummy_version ; bool nolink ; bool link_ibl ; bool tracedump_binary ; bool tracedump_text ; bool tracedump_origins ; bool syntax_intel ; bool syntax_att ; bool decode_strict ; bool bbdump_tags ; bool gendump ; bool global_rstats ; pathstring_t logdir ; bool kstats ; bool profile_pcs ; liststring_t client_lib ; bool private_loader ; uint client_lib_tls_size ; bool privload_register_gdb ; bool code_api ; bool probe_api ; bool opt_speed ; bool opt_memory ; bool bb_prefixes ; bool full_decode ; bool fast_client_decode ; bool separate_private_bss ; uint opt_cleancall ; bool cleancall_ignore_eflags ; bool mangle_app_seg ; bool x86_to_x64 ; uint msgbox_mask ; uint syslog_mask ; uint syslog_internal_mask ; bool syslog_init ; uint dumpcore_mask ; bool pause_on_error_aka_dumpcore_mask ; uint dumpcore_violation_threshold ; bool live_dump ; uint stderr_mask ; uint appfault_mask ; bool dup_stdout_on_close ; bool dup_stderr_on_close ; bool dup_stdin_on_close ; uint steal_fds ; bool avoid_dlclose ; bool intercept_all_signals ; bool use_all_memory_areas ; bool diagnostics ; uint max_supported_os_version ; uint os_aslr ; uint os_aslr_version ; uint svchost_timeout ; uint deadlock_timeout ; uint stack_size ; bool stack_shares_gencode ; uint spinlock_count_on_SMP ; bool nop_initial_bblock ; bool nullcalls ; uint trace_threshold ; bool disable_traces ; bool enable_traces ; uint trace_counter_on_delete ; uint max_elide_jmp ; uint max_elide_call ; bool elide_back_jmps ; bool elide_back_calls ; uint selfmod_max_writes ; uint max_bb_instrs ; bool process_SEH_push ; bool check_for_SEH_push ; bool shared_bbs ; bool shared_traces ; bool thread_private ; bool remove_shared_trace_heads ; bool remove_trace_components ; bool shared_deletion ; bool syscalls_synch_flush ; uint lazy_deletion_max_pending ; bool free_unmapped_futures ; bool private_ib_in_tls ; bool single_thread_in_DR ; bool separate_private_stubs ; bool separate_shared_stubs ; bool free_private_stubs ; bool unsafe_free_shared_stubs ; bool cbr_single_stub ; bool indirect_stubs ; bool inline_bb_ibl ; bool atomic_inlined_linking ; bool inline_trace_ibl ; bool shared_bb_ibt_tables ; bool shared_trace_ibt_tables ; bool ref_count_shared_ibt_tables ; bool ibl_table_in_tls ; bool bb_ibl_targets ; bool bb_ibt_table_includes_traces ; bool bb_single_restore_prefix ; bool trace_single_restore_prefix ; uint rehash_unlinked_threshold ; bool rehash_unlinked_always ; bool shared_bbs_only ; bool shared_trace_ibl_routine ; bool speculate_last_exit ; uint max_trace_bbs ; uint protect_mask ; bool single_privileged_thread ; uint alt_hash_func ; uint ibl_hash_func_offset ; uint ibl_indcall_hash_offset ; uint shared_bb_load ; uint shared_trace_load ; uint shared_future_load ; uint shared_after_call_load ; uint global_rct_ind_br_load ; uint private_trace_load ; uint private_ibl_targets_load ; uint private_bb_ibl_targets_load ; uint shared_ibt_table_trace_init ; uint shared_ibt_table_bb_init ; uint shared_ibt_table_trace_load ; uint shared_ibt_table_bb_load ; uint coarse_htable_load ; uint coarse_th_htable_load ; uint coarse_pclookup_htable_load ; uint bb_ibt_groom ; uint trace_ibt_groom ; uint private_trace_ibl_targets_init ; uint private_bb_ibl_targets_init ; uint private_trace_ibl_targets_max ; uint private_bb_ibl_targets_max ; uint private_bb_load ; uint private_future_load ; bool spin_yield_mutex ; bool spin_yield_rwlock ; bool simulate_contention ; uint initial_heap_unit_size ; uint initial_global_heap_unit_size ; uint max_heap_unit_size ; uint heap_commit_increment ; uint cache_commit_increment ; uint cache_bb_max ; uint cache_bb_unit_init ; uint cache_bb_unit_max ; uint cache_bb_unit_quadruple ; uint cache_trace_max ; uint cache_trace_unit_init ; uint cache_trace_unit_max ; uint cache_trace_unit_quadruple ; uint cache_shared_bb_max ; uint cache_shared_bb_unit_init ; uint cache_shared_bb_unit_max ; uint cache_shared_bb_unit_quadruple ; uint cache_shared_trace_max ; uint cache_shared_trace_unit_init ; uint cache_shared_trace_unit_max ; uint cache_shared_trace_unit_quadruple ; uint cache_coarse_bb_max ; uint cache_coarse_bb_unit_init ; uint cache_coarse_bb_unit_max ; uint cache_coarse_bb_unit_quadruple ; bool finite_bb_cache ; bool finite_trace_cache ; bool finite_shared_bb_cache ; bool finite_shared_trace_cache ; bool finite_coarse_bb_cache ; uint cache_bb_unit_upgrade ; uint cache_trace_unit_upgrade ; uint cache_shared_bb_unit_upgrade ; uint cache_shared_trace_unit_upgrade ; uint cache_coarse_bb_unit_upgrade ; uint cache_bb_regen ; uint cache_bb_replace ; uint cache_trace_regen ; uint cache_trace_replace ; uint cache_shared_bb_regen ; uint cache_shared_bb_replace ; uint cache_shared_trace_regen ; uint cache_shared_trace_replace ; uint cache_coarse_bb_regen ; uint cache_coarse_bb_replace ; uint cache_trace_align ; uint cache_bb_align ; uint cache_coarse_align ; uint ro2sandbox_threshold ; uint sandbox2ro_threshold ; bool sandbox_writable ; bool sandbox_non_text ; bool cache_shared_free_list ; bool enable_reset ; uint reset_at_fragment_count ; uint reset_at_nth_thread ; bool switch_to_os_at_vmm_reset_limit ; bool reset_at_switch_to_os_at_vmm_limit ; uint reset_at_vmm_percent_free_limit ; uint reset_at_vmm_free_limit ; uint report_reset_vmm_threshold ; bool reset_at_vmm_full ; uint reset_at_commit_percent_free_limit ; uint reset_at_commit_free_limit ; uint report_reset_commit_threshold ; uint reset_every_nth_pending ; uint reset_at_nth_bb_unit ; uint reset_at_nth_trace_unit ; uint reset_every_nth_bb_unit ; uint reset_every_nth_trace_unit ; bool skip_out_of_vm_reserve_curiosity ; bool vm_reserve ; uint vm_size ; ptr_uint_t vm_base ; ptr_uint_t vm_max_offset ; bool vm_allow_not_at_base ; bool vm_allow_smaller ; bool vm_base_near_app ; bool heap_in_lower_4GB ; bool reachable_heap ; bool vm_use_last ; uint silent_oom_mask ; liststring_t silent_commit_oom_list ; uint oom_timeout ; bool follow_children ; bool follow_systemwide ; bool follow_explicit_children ; bool early_inject ; bool early_inject_map ; uint early_inject_location ; ptr_uint_t early_inject_address ; bool early_inject_stress_helpers ; bool inject_at_create_process ; bool vista_inject_at_create_process ; bool inject_primary ; uint synch_thread_max_loops ; uint synch_all_threads_max_loops ; bool synch_thread_sleep_UP ; bool synch_thread_sleep_MP ; uint synch_with_sleep_time ; bool ignore_syscalls ; bool inline_ignored_syscalls ; bool hook_vsyscall ; bool sysenter_is_int80_aka_hook_vsyscall ; bool restart_syscalls ; bool guard_pages ; bool enable_block_mod_load ; liststring_t block_mod_load_list_default ; liststring_t block_mod_load_list ; uint handle_DR_modify ; uint handle_ntdll_modify ; liststring_t patch_proof_default_list ; liststring_t patch_proof_list ; bool use_moduledb ; bool staged_aka_use_moduledb ; liststring_t whitelist_company_names_default ; liststring_t whitelist_company_names ; uint unknown_module_policy ; uint unknown_module_load_report_max ; uint moduledb_exemptions_report_max ; bool unloaded_target_exception ; bool cache_consistency ; bool sandbox_writes ; bool safe_translate_flushed ; bool store_translations ; bool translate_fpu_pc ; bool validate_owner_dir ; bool validate_owner_file ; bool coarse_units ; bool enable_full_api_aka_coarse_units ; bool coarse_enable_freeze ; bool coarse_freeze_at_exit ; bool coarse_freeze_at_unload ; uint coarse_freeze_min_size ; uint coarse_freeze_append_size ; uint coarse_freeze_rct_min ; bool coarse_freeze_clobber ; bool coarse_freeze_rename ; bool coarse_freeze_clean ; bool coarse_freeze_merge ; bool coarse_lone_merge ; bool coarse_disk_merge ; bool coarse_freeze_elide_ubr ; bool unsafe_freeze_elide_sole_ubr ; bool coarse_pclookup_table ; bool persist_per_app ; bool persist_per_user ; bool use_persisted ; liststring_t persist_exclude_list ; uint coarse_fill_ibl ; bool persist_map_rw_separate ; bool persist_lock_file ; uint persist_gen_validation ; uint persist_load_validation ; uint persist_short_digest ; bool persist_check_options ; bool persist_check_local_options ; bool persist_check_exempted_options ; bool persist_protect_stubs ; uint persist_protect_stubs_limit ; bool persist_touch_stubs ; bool coarse_merge_iat ; bool coarse_split_calls ; bool coarse_split_riprel ; bool persist_trust_textrel ; pathstring_t persist_dir ; pathstring_t persist_shared_dir ; bool persist ; bool desktop ; bool native_exec ; liststring_t native_exec_default_list ; liststring_t native_exec_list ; bool native_exec_syscalls ; bool native_exec_dircalls ; bool native_exec_callcall ; bool native_exec_guess_calls ; bool native_exec_managed_code ; bool native_exec_dot_pexe ; bool native_exec_retakeover ; bool inline_calls ; bool indcall2direct ; bool IAT_convert ; bool IAT_elide ; bool unsafe_IAT_ignore_hooker ; uint thread_policy ; uint min_free_disk ; uint prng_seed ; bool pad_jmps ; bool pad_jmps_mark_no_trace ; bool pad_jmps_return_excess_padding ; bool pad_jmps_shift_bb ; bool pad_jmps_shift_trace ; uint pad_jmps_set_alignment ; bool ibl_sentinel_check ; bool ibl_addr_prefix ; uint slowdown_ibl_found ; bool stress_recreate_pc ; bool stress_recreate_state ; bool detect_dangling_fcache ; bool stress_detach_with_stacked_callbacks ; bool detach_fix_sysenter_on_stack ; uint vmarea_initial_size ; uint vmarea_increment_size ; ptr_uint_t stress_fake_userva ; bool unsafe_crash_process ; bool unsafe_hang_process ; bool unsafe_ignore_overflow ; bool unsafe_ignore_eflags ; bool unsafe_ignore_eflags_trace ; bool unsafe_ignore_eflags_prefix ; bool unsafe_ignore_eflags_ibl ; liststring_t ignore_assert_list ; bool synch_at_exit ; bool multi_thread_exit ; bool skip_thread_exit_at_exit ; bool optimize ; bool prefetch ; bool rlr ; bool vectorize ; bool unroll_loops ; bool instr_counts ; bool stack_adjust ; uint remove_dead_code ; uint constant_prop ; bool call_return_matching ; bool remove_unnecessary_zeroing ; bool peephole ; bool thin_client ; } options_t ;
typedef enum { LIST_NO_MATCH = 0 , LIST_ON_DEFAULT = 1 , LIST_ON_APPEND = 2 , } list_default_or_append_t ;
typedef __gnuc_va_list va_list ;
enum { VM_ALLOCATION_BOUNDARY = 64 * 1024 } ;
struct _local_state_t ;
enum { HEAP_ERROR_SUCCESS = 0 , HEAP_ERROR_CANT_RESERVE_IN_REGION = 1 , HEAP_ERROR_NOT_AT_PREFERRED = 2 , } ;
typedef uint heap_error_code_t ;
enum { RAW_ALLOC_32BIT = 0x0004 , } ;
struct _local_state_extended_t ;
typedef enum { DR_STATE_ALL = ~ 0 , } dr_state_flags_t ;
typedef enum { TERMINATE_PROCESS = 0x1 , TERMINATE_THREAD = 0x2 , TERMINATE_CLEANUP = 0x4 } terminate_flags_t ;
typedef enum { ILLEGAL_INSTRUCTION_EXCEPTION , UNREADABLE_MEMORY_EXECUTION_EXCEPTION , IN_PAGE_ERROR_EXCEPTION , } exception_type_t ;
enum { DUMPCORE_INTERNAL_EXCEPTION = 0x0001 , DUMPCORE_SECURITY_VIOLATION = 0x0002 , DUMPCORE_DEADLOCK = 0x0004 , DUMPCORE_ASSERTION = 0x0008 , DUMPCORE_FATAL_USAGE_ERROR = 0x0010 , DUMPCORE_CLIENT_EXCEPTION = 0x0020 , DUMPCORE_TIMEOUT = 0x0040 , DUMPCORE_CURIOSITY = 0x0080 , DUMPCORE_OUT_OF_MEM = 0x0200 , DUMPCORE_OUT_OF_MEM_SILENT = 0x0400 , DUMPCORE_INCLUDE_STACKDUMP = 0x0800 , DUMPCORE_WAIT_FOR_DEBUGGER = 0x1000 , DUMPCORE_DR_ABORT = 0x8000 , DUMPCORE_FORGE_ILLEGAL_INST = 0x10000 , DUMPCORE_FORGE_UNREAD_EXEC = 0x20000 , DUMPCORE_APP_EXCEPTION = 0x40000 , DUMPCORE_TRY_EXCEPT = 0x80000 , DUMPCORE_UNSUPPORTED_APP = 0x100000 , DUMPCORE_OPTION_PAUSE = DUMPCORE_WAIT_FOR_DEBUGGER | DUMPCORE_INTERNAL_EXCEPTION | DUMPCORE_SECURITY_VIOLATION | DUMPCORE_DEADLOCK | DUMPCORE_ASSERTION | DUMPCORE_FATAL_USAGE_ERROR | DUMPCORE_CLIENT_EXCEPTION | DUMPCORE_UNSUPPORTED_APP | DUMPCORE_TIMEOUT | DUMPCORE_CURIOSITY | DUMPCORE_DR_ABORT | DUMPCORE_OUT_OF_MEM | DUMPCORE_OUT_OF_MEM_SILENT , } ;
typedef void * dr_auxlib_handle_t ;
typedef void ( * dr_auxlib_routine_ptr_t ) ( ) ;
typedef void * shlib_handle_t ;
typedef void ( * shlib_routine_ptr_t ) ( ) ;
typedef enum { DR_MEMTYPE_FREE , DR_MEMTYPE_IMAGE , DR_MEMTYPE_DATA , DR_MEMTYPE_RESERVED , } dr_mem_type_t ;
typedef struct _dr_mem_info_t { byte * base_pc ; size_t size ; uint prot ; dr_mem_type_t type ; } dr_mem_info_t ;
enum { SELFPROT_DATA_RARE = 0x001 , SELFPROT_DATA_FREQ = 0x002 , SELFPROT_DATA_CXTSW = 0x004 , SELFPROT_GLOBAL = 0x008 , SELFPROT_DCONTEXT = 0x010 , SELFPROT_LOCAL = 0x020 , SELFPROT_CACHE = 0x040 , SELFPROT_STACK = 0x080 , SELFPROT_GENCODE = 0x100 , SELFPROT_ON_CXT_SWITCH = ( SELFPROT_DATA_CXTSW | SELFPROT_GLOBAL | SELFPROT_DATA_FREQ ) , SELFPROT_ANY_DATA_SECTION = ( SELFPROT_DATA_RARE | SELFPROT_DATA_FREQ | SELFPROT_DATA_CXTSW ) , } ;
enum { DATASEC_NEVER_PROT = 0 , DATASEC_RARELY_PROT , DATASEC_FREQ_PROT , DATASEC_CXTSW_PROT , DATASEC_NUM , } ;
typedef enum { CREATE_DIR_ALLOW_EXISTING = 0x0 , CREATE_DIR_REQUIRE_NEW = 0x1 , CREATE_DIR_FORCE_OWNER = 0x2 , } create_directory_flags_t ;
typedef enum { ASLR_TARGET_VIOLATION = - 12 , APC_THREAD_SHELLCODE_VIOLATION = - 15 , INVALID_VIOLATION = 0 , NO_VIOLATION_BAD_INTERNAL_STATE = 3 , NO_VIOLATION_OK_INTERNAL_STATE = 4 } security_violation_t ;
typedef struct linux_event_t * event_t ;
typedef enum { AFTER_INTERCEPT_LET_GO , AFTER_INTERCEPT_LET_GO_ALT_DYN , AFTER_INTERCEPT_TAKE_OVER , AFTER_INTERCEPT_DYNAMIC_DECISION , AFTER_INTERCEPT_TAKE_OVER_SINGLE_SHOT , } after_intercept_action_t ;
typedef struct { void * callee_arg ; app_pc start_pc ; priv_mcontext_t mc ; } app_state_at_intercept_t ;
typedef after_intercept_action_t intercept_function_t ( app_state_at_intercept_t * args ) ;
enum { JMP_REL32_OPCODE = 0xe9 , JMP_REL32_SIZE = 5 , CALL_REL32_OPCODE = 0xe8 , JMP_ABS_IND64_OPCODE = 0xff , JMP_ABS_IND64_SIZE = 6 , JMP_ABS_MEM_IND64_MODRM = 0x25 , } ;
typedef struct _kernel_sigset_t { unsigned long sig [ ( ( ( 32 ) + ( 33 ) - 1 ) / 64 ) ] ; } kernel_sigset_t ;
typedef enum { IBL_NONE = - 1 , IBL_RETURN = 0 , IBL_BRANCH_TYPE_START = IBL_RETURN , IBL_INDCALL , IBL_INDJMP , IBL_GENERIC = IBL_INDJMP , IBL_SHARED_SYSCALL = IBL_GENERIC , IBL_BRANCH_TYPE_END } ibl_branch_type_t ;
struct _fragment_entry_t ;
struct _ibl_table_t ;
typedef struct _lookup_table_access_t { ptr_uint_t hash_mask ; struct _fragment_entry_t * lookuptable ; } lookup_table_access_t ;
typedef struct _table_stat_state_t { lookup_table_access_t table [ IBL_BRANCH_TYPE_END ] ; } table_stat_state_t ;
typedef struct _spill_state_t { reg_t xax , xbx , xcx , xdx ; dcontext_t * dcontext ; } spill_state_t ;
typedef struct _local_state_t { spill_state_t spill_space ; } local_state_t ;
typedef struct _local_state_extended_t { spill_state_t spill_space ; table_stat_state_t table_space ; } local_state_extended_t ;
enum { TRANSLATE_IDENTICAL = 0x0001 , TRANSLATE_OUR_MANGLING = 0x0002 , } ;
typedef struct _translation_entry_t { ushort cache_offs ; ushort flags ; app_pc app ; } translation_entry_t ;
typedef struct _translation_info_t { uint num_entries ; translation_entry_t translation [ 1 ] ; } translation_info_t ;
typedef enum { RECREATE_FAILURE , RECREATE_SUCCESS_PC , RECREATE_SUCCESS_STATE , } recreate_success_t ;
typedef linkstub_t * ( * fcache_enter_func_t ) ( dcontext_t * dcontext ) ;
enum { SYSCALL_METHOD_UNINITIALIZED , SYSCALL_METHOD_INT , SYSCALL_METHOD_SYSENTER , SYSCALL_METHOD_SYSCALL , } ;
enum { SYSCALL_METHOD_LONGEST_INSTR = 2 } ;
enum { BACK_FROM_NATIVE_RETSTUB_SIZE = 4 } ;
enum { VENDOR_INTEL , VENDOR_AMD , VENDOR_UNKNOWN , } ;
typedef struct { uint flags_edx ; uint flags_ecx ; uint ext_flags_edx ; uint ext_flags_ecx ; } features_t ;
typedef enum { FEATURE_FPU = 0 , FEATURE_VME = 1 , FEATURE_DE = 2 , FEATURE_PSE = 3 , FEATURE_TSC = 4 , FEATURE_MSR = 5 , FEATURE_PAE = 6 , FEATURE_MCE = 7 , FEATURE_CX8 = 8 , FEATURE_APIC = 9 , FEATURE_SEP = 11 , FEATURE_MTRR = 12 , FEATURE_PGE = 13 , FEATURE_MCA = 14 , FEATURE_CMOV = 15 , FEATURE_PAT = 16 , FEATURE_PSE_36 = 17 , FEATURE_PSN = 18 , FEATURE_CLFSH = 19 , FEATURE_DS = 21 , FEATURE_ACPI = 22 , FEATURE_MMX = 23 , FEATURE_FXSR = 24 , FEATURE_SSE = 25 , FEATURE_SSE2 = 26 , FEATURE_SS = 27 , FEATURE_HTT = 28 , FEATURE_TM = 29 , FEATURE_IA64 = 30 , FEATURE_PBE = 31 , FEATURE_SSE3 = 0 + 32 , FEATURE_PCLMULQDQ = 1 + 32 , FEATURE_MONITOR = 3 + 32 , FEATURE_DS_CPL = 4 + 32 , FEATURE_VMX = 5 + 32 , FEATURE_EST = 7 + 32 , FEATURE_TM2 = 8 + 32 , FEATURE_SSSE3 = 9 + 32 , FEATURE_CID = 10 + 32 , FEATURE_FMA = 12 + 32 , FEATURE_CX16 = 13 + 32 , FEATURE_xPTR = 14 + 32 , FEATURE_SSE41 = 19 + 32 , FEATURE_SSE42 = 20 + 32 , FEATURE_MOVBE = 22 + 32 , FEATURE_POPCNT = 23 + 32 , FEATURE_AES = 25 + 32 , FEATURE_XSAVE = 26 + 32 , FEATURE_OSXSAVE = 27 + 32 , FEATURE_AVX = 28 + 32 , FEATURE_SYSCALL = 11 + 64 , FEATURE_XD_Bit = 20 + 64 , FEATURE_EM64T = 29 + 64 , FEATURE_LAHF = 0 + 96 } feature_bit_t ;
typedef enum { CACHE_SIZE_8_KB , CACHE_SIZE_16_KB , CACHE_SIZE_32_KB , CACHE_SIZE_64_KB , CACHE_SIZE_128_KB , CACHE_SIZE_256_KB , CACHE_SIZE_512_KB , CACHE_SIZE_1_MB , CACHE_SIZE_2_MB , CACHE_SIZE_UNKNOWN } cache_size_t ;
enum { CALLSTACK_USE_XML = 0x00000001 , CALLSTACK_ADD_HEADER = 0x00000002 , CALLSTACK_MODULE_INFO = 0x00000004 , CALLSTACK_MODULE_PATH = 0x00000008 , CALLSTACK_FRAME_PTR = 0x00000010 , } ;
enum { NOT_HOT_PATCHABLE = ( 0 ) , HOT_PATCHABLE = ( 1 ) } ;
enum { MAX_INSTR_LENGTH = 17 , CBR_LONG_LENGTH = 6 , JMP_LONG_LENGTH = 5 , JMP_SHORT_LENGTH = 2 , CBR_SHORT_REWRITE_LENGTH = 9 , RET_0_LENGTH = 1 , PUSH_IMM32_LENGTH = 5 , CTI_IND1_LENGTH = 2 , CTI_IND2_LENGTH = 3 , CTI_IND3_LENGTH = 4 , CTI_DIRECT_LENGTH = 5 , CTI_IAT_LENGTH = 6 , CTI_FAR_ABS_LENGTH = 7 , INT_LENGTH = 2 , SYSCALL_LENGTH = 2 , SYSENTER_LENGTH = 2 , } ;
typedef struct { app_pc region_start ; app_pc region_end ; app_pc start_pc ; app_pc min_pc ; app_pc max_pc ; app_pc bb_end ; bool contiguous ; bool overlap ; } overlap_info_t ;
enum { LINK_DIRECT = 0x0001 , LINK_INDIRECT = 0x0002 , LINK_RETURN = 0x0004 , LINK_CALL = 0x0008 , LINK_JMP = 0x0010 , LINK_FAR = 0x0020 , LINK_TRACE_CMP = 0x0080 , LINK_SPECIAL_EXIT = 0x0100 , LINK_NI_SYSCALL_INT = 0x0200 , LINK_NI_SYSCALL = 0x0400 , LINK_FINAL_INSTR_SHARED_FLAG = LINK_NI_SYSCALL , LINK_FRAG_OFFS_AT_END = 0x0800 , LINK_END_OF_LIST = 0x1000 , LINK_FAKE = 0x2000 , LINK_LINKED = 0x4000 , LINK_SEPARATE_STUB = 0x8000 , } ;
struct _linkstub_t { ushort flags ; ushort cti_offset ; } ;
typedef struct _common_direct_linkstub_t { linkstub_t l ; linkstub_t * next_incoming ; } common_direct_linkstub_t ;
typedef struct _direct_linkstub_t { common_direct_linkstub_t cdl ; app_pc target_tag ; cache_pc stub_pc ; } direct_linkstub_t ;
typedef struct _cbr_fallthrough_linkstub_t { common_direct_linkstub_t cdl ; } cbr_fallthrough_linkstub_t ;
typedef struct _indirect_linkstub_t { linkstub_t l ; } indirect_linkstub_t ;
typedef struct _post_linkstub_t { ushort fragment_offset ; ushort padding ; } post_linkstub_t ;
typedef struct _coarse_incoming_t { union { cache_pc stub_pc ; linkstub_t * fine_l ; } in ; bool coarse ; struct _coarse_incoming_t * next ; } coarse_incoming_t ;
typedef struct dr_jmp_buf_t { reg_t xbx ; reg_t xcx ; reg_t xdi ; reg_t xsi ; reg_t xbp ; reg_t xsp ; reg_t xip ; reg_t r8 , r9 , r10 , r11 , r12 , r13 , r14 , r15 ; } dr_jmp_buf_t ;
struct vm_area_t ;
enum { VECTOR_SHARED = 0x0001 , VECTOR_FRAGMENT_LIST = 0x0002 , VECTOR_NEVER_MERGE_ADJACENT = 0x0004 , VECTOR_NEVER_OVERLAP = 0x0008 , VECTOR_NO_LOCK = 0x0010 , } ;
struct vm_area_vector_t { struct vm_area_t * buf ; int size ; int length ; uint flags ; read_write_lock_t lock ; void ( * free_payload_func ) ( void * ) ; void * ( * split_payload_func ) ( void * ) ; bool ( * should_merge_func ) ( bool adjacent , void * , void * ) ; void * ( * merge_payload_func ) ( void * dst , void * src ) ; } ;
typedef struct vmvector_iterator_t { vm_area_vector_t * vector ; int index ; } vmvector_iterator_t ;
enum { DO_APP_MEM_PROT_CHANGE , FAIL_APP_MEM_PROT_CHANGE , PRETEND_APP_MEM_PROT_CHANGE , SUBSET_APP_MEM_PROT_CHANGE , } ;
enum { DR_MODIFY_HALT = 0 , DR_MODIFY_NOP = 1 , DR_MODIFY_FAIL = 2 , DR_MODIFY_ALLOW = 3 , DR_MODIFY_OFF = 4 , } ;
typedef enum { APC_TARGET_NATIVE , APC_TARGET_WINDOWS , THREAD_TARGET_NATIVE , THREAD_TARGET_WINDOWS } apc_thread_type_t ;
struct _instr_list_t { instr_t * first ; instr_t * last ; int flags ; app_pc translation_target ; app_pc fall_through_bb ; } ;
typedef struct _single_stat_t { char name [ 50 ] ; stats_int_t value ; } single_stat_t ;
typedef struct _dr_statistics_t { char magicstring [ 16 ] ; process_id_t process_id ; char process_name [ 260 ] ; uint logmask ; uint loglevel ; char logdir [ 260 ] ; uint64 perfctr_vals [ 27 ] ; uint num_stats ; single_stat_t num_threads_pair ; single_stat_t peak_num_threads_pair ; single_stat_t num_threads_created_pair ; single_stat_t num_signals_pair ; single_stat_t num_signals_dropped_pair ; single_stat_t num_signals_coarse_delayed_pair ; single_stat_t pre_syscall_pair ; single_stat_t post_syscall_pair ; single_stat_t num_native_module_loads_pair ; single_stat_t num_app_mmaps_pair ; single_stat_t num_app_munmaps_pair ; single_stat_t num_bbs_pair ; single_stat_t num_traces_pair ; single_stat_t num_coarse_units_pair ; single_stat_t peak_num_coarse_units_pair ; single_stat_t perscache_loaded_pair ; single_stat_t fcache_num_live_pair ; single_stat_t peak_fcache_num_live_pair ; single_stat_t fcache_num_free_pair ; single_stat_t peak_fcache_num_free_pair ; single_stat_t heap_num_live_pair ; single_stat_t peak_heap_num_live_pair ; single_stat_t heap_num_free_pair ; single_stat_t peak_heap_num_free_pair ; } dr_statistics_t ;
typedef struct { thread_id_t thread_id ; mutex_t thread_stats_lock ; stats_int_t num_threads_thread ; stats_int_t peak_num_threads_thread ; stats_int_t num_threads_created_thread ; stats_int_t num_signals_thread ; stats_int_t num_signals_dropped_thread ; stats_int_t num_signals_coarse_delayed_thread ; stats_int_t pre_syscall_thread ; stats_int_t post_syscall_thread ; stats_int_t num_native_module_loads_thread ; stats_int_t num_app_mmaps_thread ; stats_int_t num_app_munmaps_thread ; stats_int_t num_bbs_thread ; stats_int_t num_traces_thread ; stats_int_t num_coarse_units_thread ; stats_int_t peak_num_coarse_units_thread ; stats_int_t perscache_loaded_thread ; stats_int_t fcache_num_live_thread ; stats_int_t peak_fcache_num_live_thread ; stats_int_t fcache_num_free_thread ; stats_int_t peak_fcache_num_free_thread ; stats_int_t heap_num_live_thread ; stats_int_t peak_heap_num_live_thread ; stats_int_t heap_num_free_thread ; stats_int_t peak_heap_num_free_thread ; } thread_local_statistics_t ;
typedef struct _client_to_do_list_t { instrlist_t * ilist ; app_pc tag ; struct _client_to_do_list_t * next ; } client_todo_list_t ;
typedef struct _client_flush_req_t { app_pc start ; size_t size ; uint flush_id ; void ( * flush_callback ) ( int ) ; struct _client_flush_req_t * next ; } client_flush_req_t ;
typedef struct _client_data_t { void * user_field ; client_todo_list_t * to_do ; client_flush_req_t * flush_list ; mutex_t sideline_mutex ; module_data_t * no_delete_mod_data ; bool is_client_thread ; bool client_thread_safe_for_synch ; bool suspendable ; bool left_unsuspended ; uint mutex_count ; void * client_grab_mutex ; bool in_pre_syscall ; bool in_post_syscall ; bool invoke_another_syscall ; bool mcontext_in_dcontext ; bool suspended ; priv_mcontext_t * cur_mc ; } client_data_t ;
typedef struct _pending_nudge_t { nudge_arg_t arg ; struct _pending_nudge_t * next ; } pending_nudge_t ;
typedef enum { WHERE_APP = 0 , WHERE_INTERP , WHERE_DISPATCH , WHERE_MONITOR , WHERE_SYSCALL_HANDLER , WHERE_SIGNAL_HANDLER , WHERE_TRAMPOLINE , WHERE_CONTEXT_SWITCH , WHERE_IBL , WHERE_FCACHE , WHERE_UNKNOWN , WHERE_LAST } where_am_i_t ;
enum { READONLY = ( 0 ) , WRITABLE = ( 1 ) } ;
enum { EXIT_REASON_SELFMOD = 0 , EXIT_REASON_FLOAT_PC_FNSAVE , EXIT_REASON_FLOAT_PC_FXSAVE , EXIT_REASON_FLOAT_PC_FXSAVE64 , EXIT_REASON_FLOAT_PC_XSAVE , EXIT_REASON_FLOAT_PC_XSAVE64 , } ;
enum { MAX_NATIVE_RETSTACK = 10 } ;
typedef struct _retaddr_and_retloc_t { app_pc retaddr ; app_pc retloc ; } retaddr_and_retloc_t ;
typedef struct try_except_context_t { dr_jmp_buf_t context ; struct try_except_context_t * prev_context ; } try_except_context_t ;
typedef struct _try_except_t { try_except_context_t * try_except_state ; bool unwinding_exception ; } try_except_t ;
typedef struct { priv_mcontext_t mcontext ; int errno ; bool at_syscall ; ushort exit_reason ; reg_t inline_spill_slots [ 5 ] ; } unprotected_context_t ;
struct _dcontext_t { union { unprotected_context_t * separate_upcontext ; unprotected_context_t upcontext ; } upcontext ; unprotected_context_t * upcontext_ptr ; app_pc next_tag ; linkstub_t * last_exit ; byte * dstack ; bool is_exiting ; union { app_pc src_tag ; coarse_info_t * dir_exit ; } coarse_exit ; bool initialized ; thread_id_t owning_thread ; process_id_t owning_process ; thread_record_t * thread_record ; where_am_i_t whereami ; void * allocated_start ; fragment_t * last_fragment ; int sys_num ; reg_t sys_param0 ; reg_t sys_param1 ; reg_t sys_param2 ; reg_t sys_param3 ; reg_t sys_param4 ; bool sys_was_int ; bool sys_xbp ; bool x86_mode ; void * link_field ; void * monitor_field ; void * fcache_field ; void * fragment_field ; void * heap_field ; void * vm_areas_field ; void * os_field ; void * synch_field ; void * signal_field ; void * pcprofile_field ; bool signals_pending ; void * private_code ; app_pc asynch_target ; app_pc native_exec_postsyscall ; retaddr_and_retloc_t native_retstack [ MAX_NATIVE_RETSTACK ] ; uint native_retstack_cur ; thread_kstats_t * thread_kstats ; client_data_t * client_data ; bool trace_sysenter_exit ; app_pc forged_exception_addr ; try_except_t try_except ; local_state_t * local_state ; void * bb_build_info ; pending_nudge_t * nudge_pending ; fragment_t * interrupted_for_nudge ; } ;
enum { DUMP_XML = ( 1 ) , DUMP_NOT_XML = ( 0 ) } ;
struct instr_info_t ;
enum { DR_REG_NULL , DR_REG_RAX , DR_REG_RCX , DR_REG_RDX , DR_REG_RBX , DR_REG_RSP , DR_REG_RBP , DR_REG_RSI , DR_REG_RDI , DR_REG_R8 , DR_REG_R9 , DR_REG_R10 , DR_REG_R11 , DR_REG_R12 , DR_REG_R13 , DR_REG_R14 , DR_REG_R15 , DR_REG_EAX , DR_REG_ECX , DR_REG_EDX , DR_REG_EBX , DR_REG_ESP , DR_REG_EBP , DR_REG_ESI , DR_REG_EDI , DR_REG_R8D , DR_REG_R9D , DR_REG_R10D , DR_REG_R11D , DR_REG_R12D , DR_REG_R13D , DR_REG_R14D , DR_REG_R15D , DR_REG_AX , DR_REG_CX , DR_REG_DX , DR_REG_BX , DR_REG_SP , DR_REG_BP , DR_REG_SI , DR_REG_DI , DR_REG_R8W , DR_REG_R9W , DR_REG_R10W , DR_REG_R11W , DR_REG_R12W , DR_REG_R13W , DR_REG_R14W , DR_REG_R15W , DR_REG_AL , DR_REG_CL , DR_REG_DL , DR_REG_BL , DR_REG_AH , DR_REG_CH , DR_REG_DH , DR_REG_BH , DR_REG_R8L , DR_REG_R9L , DR_REG_R10L , DR_REG_R11L , DR_REG_R12L , DR_REG_R13L , DR_REG_R14L , DR_REG_R15L , DR_REG_SPL , DR_REG_BPL , DR_REG_SIL , DR_REG_DIL , DR_REG_MM0 , DR_REG_MM1 , DR_REG_MM2 , DR_REG_MM3 , DR_REG_MM4 , DR_REG_MM5 , DR_REG_MM6 , DR_REG_MM7 , DR_REG_XMM0 , DR_REG_XMM1 , DR_REG_XMM2 , DR_REG_XMM3 , DR_REG_XMM4 , DR_REG_XMM5 , DR_REG_XMM6 , DR_REG_XMM7 , DR_REG_XMM8 , DR_REG_XMM9 , DR_REG_XMM10 , DR_REG_XMM11 , DR_REG_XMM12 , DR_REG_XMM13 , DR_REG_XMM14 , DR_REG_XMM15 , DR_REG_ST0 , DR_REG_ST1 , DR_REG_ST2 , DR_REG_ST3 , DR_REG_ST4 , DR_REG_ST5 , DR_REG_ST6 , DR_REG_ST7 , DR_SEG_ES , DR_SEG_CS , DR_SEG_SS , DR_SEG_DS , DR_SEG_FS , DR_SEG_GS , DR_REG_DR0 , DR_REG_DR1 , DR_REG_DR2 , DR_REG_DR3 , DR_REG_DR4 , DR_REG_DR5 , DR_REG_DR6 , DR_REG_DR7 , DR_REG_DR8 , DR_REG_DR9 , DR_REG_DR10 , DR_REG_DR11 , DR_REG_DR12 , DR_REG_DR13 , DR_REG_DR14 , DR_REG_DR15 , DR_REG_CR0 , DR_REG_CR1 , DR_REG_CR2 , DR_REG_CR3 , DR_REG_CR4 , DR_REG_CR5 , DR_REG_CR6 , DR_REG_CR7 , DR_REG_CR8 , DR_REG_CR9 , DR_REG_CR10 , DR_REG_CR11 , DR_REG_CR12 , DR_REG_CR13 , DR_REG_CR14 , DR_REG_CR15 , DR_REG_INVALID , DR_REG_YMM0 , DR_REG_YMM1 , DR_REG_YMM2 , DR_REG_YMM3 , DR_REG_YMM4 , DR_REG_YMM5 , DR_REG_YMM6 , DR_REG_YMM7 , DR_REG_YMM8 , DR_REG_YMM9 , DR_REG_YMM10 , DR_REG_YMM11 , DR_REG_YMM12 , DR_REG_YMM13 , DR_REG_YMM14 , DR_REG_YMM15 , } ;
typedef byte reg_id_t ;
typedef byte opnd_size_t ;
struct _opnd_t { byte kind ; opnd_size_t size ; union { ushort far_pc_seg_selector ; reg_id_t segment : 8 ; ushort disp ; ushort shift ; } seg ; union { ptr_int_t immed_int ; float immed_float ; app_pc pc ; instr_t * instr ; reg_id_t reg ; struct { int disp ; reg_id_t base_reg : 8 ; reg_id_t index_reg : 8 ; byte scale : 4 ; byte encode_zero_disp : 1 ; byte force_full_disp : 1 ; byte disp_short_addr : 1 ; } base_disp ; void * addr ; } value ; } ;
enum { NULL_kind , IMMED_INTEGER_kind , IMMED_FLOAT_kind , PC_kind , INSTR_kind , REG_kind , BASE_DISP_kind , FAR_PC_kind , FAR_INSTR_kind , REL_ADDR_kind , ABS_ADDR_kind , MEM_INSTR_kind , LAST_kind , } ;
enum { INSTR_DIRECT_EXIT = LINK_DIRECT , INSTR_INDIRECT_EXIT = LINK_INDIRECT , INSTR_RETURN_EXIT = LINK_RETURN , INSTR_CALL_EXIT = LINK_CALL , INSTR_JMP_EXIT = LINK_JMP , INSTR_IND_JMP_PLT_EXIT = ( INSTR_JMP_EXIT | INSTR_CALL_EXIT ) , INSTR_FAR_EXIT = LINK_FAR , INSTR_BRANCH_SPECIAL_EXIT = LINK_SPECIAL_EXIT , INSTR_TRACE_CMP_EXIT = LINK_TRACE_CMP , INSTR_NI_SYSCALL_INT = LINK_NI_SYSCALL_INT , INSTR_NI_SYSCALL = LINK_NI_SYSCALL , INSTR_NI_SYSCALL_ALL = ( LINK_NI_SYSCALL | LINK_NI_SYSCALL_INT ) , EXIT_CTI_TYPES = ( INSTR_DIRECT_EXIT | INSTR_INDIRECT_EXIT | INSTR_RETURN_EXIT | INSTR_CALL_EXIT | INSTR_JMP_EXIT | INSTR_FAR_EXIT | INSTR_BRANCH_SPECIAL_EXIT | INSTR_TRACE_CMP_EXIT | INSTR_NI_SYSCALL_INT | INSTR_NI_SYSCALL ) , INSTR_OPERANDS_VALID = 0x00010000 , INSTR_FIRST_NON_LINK_SHARED_FLAG = INSTR_OPERANDS_VALID , INSTR_EFLAGS_VALID = 0x00020000 , INSTR_EFLAGS_6_VALID = 0x00040000 , INSTR_RAW_BITS_VALID = 0x00080000 , INSTR_RAW_BITS_ALLOCATED = 0x00100000 , INSTR_DO_NOT_MANGLE = 0x00200000 , INSTR_HAS_CUSTOM_STUB = 0x00400000 , INSTR_IND_CALL_DIRECT = 0x00800000 , INSTR_CLOBBER_RETADDR = 0x02000000 , INSTR_HOT_PATCHABLE = 0x04000000 , INSTR_DO_NOT_EMIT = 0x10000000 , INSTR_RIP_REL_VALID = 0x20000000 , INSTR_X86_MODE = 0x40000000 , INSTR_OUR_MANGLING = 0x80000000 , } ;
typedef struct _dr_instr_label_data_t { ptr_uint_t data [ 4 ] ; } dr_instr_label_data_t ;

struct _instr_t {
    uint flags;
    byte * bytes;
    uint length;
    app_pc translation;
    uint16_t opcode;
    uint16_t granary_policy; // code cache policy for granary
    uint8_t granary_flags; // flags, e.g. delay, don't mangle, etc.
    byte rip_rel_pos;
    byte num_dsts;
    byte num_srcs;
    union {
        struct {
            opnd_t src0;
            opnd_t * srcs;
            opnd_t * dsts;
        } o;
        dr_instr_label_data_t label_data;
    } u;
    uint prefixes;
    uint eflags;
    void * note;
    instr_t * prev;
    instr_t * next;
};
typedef enum { DR_FP_STATE , DR_FP_MOVE , DR_FP_CONVERT , DR_FP_MATH , } dr_fp_type_t ;
enum { EFLAGS_CF = 0x00000001 , EFLAGS_PF = 0x00000004 , EFLAGS_AF = 0x00000010 , EFLAGS_ZF = 0x00000040 , EFLAGS_SF = 0x00000080 , EFLAGS_DF = 0x00000400 , EFLAGS_OF = 0x00000800 , } ;
enum { RAW_OPCODE_nop = 0x90 , RAW_OPCODE_jmp_short = 0xeb , RAW_OPCODE_call = 0xe8 , RAW_OPCODE_ret = 0xc3 , RAW_OPCODE_jmp = 0xe9 , RAW_OPCODE_push_imm32 = 0x68 , RAW_OPCODE_jcc_short_start = 0x70 , RAW_OPCODE_jcc_short_end = 0x7f , RAW_OPCODE_jcc_byte1 = 0x0f , RAW_OPCODE_jcc_byte2_start = 0x80 , RAW_OPCODE_jcc_byte2_end = 0x8f , RAW_OPCODE_loop_start = 0xe0 , RAW_OPCODE_loop_end = 0xe3 , RAW_OPCODE_lea = 0x8d , RAW_PREFIX_jcc_not_taken = 0x2e , RAW_PREFIX_jcc_taken = 0x3e , RAW_PREFIX_lock = 0xf0 , } ;
enum { FS_SEG_OPCODE = 0x64 , GS_SEG_OPCODE = 0x65 , TLS_SEG_OPCODE = GS_SEG_OPCODE , DATA_PREFIX_OPCODE = 0x66 , ADDR_PREFIX_OPCODE = 0x67 , REPNE_PREFIX_OPCODE = 0xf2 , REP_PREFIX_OPCODE = 0xf3 , REX_PREFIX_BASE_OPCODE = 0x40 , REX_PREFIX_W_OPFLAG = 0x8 , REX_PREFIX_R_OPFLAG = 0x4 , REX_PREFIX_X_OPFLAG = 0x2 , REX_PREFIX_B_OPFLAG = 0x1 , REX_PREFIX_ALL_OPFLAGS = 0xf , MOV_REG2MEM_OPCODE = 0x89 , MOV_MEM2REG_OPCODE = 0x8b , MOV_XAX2MEM_OPCODE = 0xa3 , MOV_MEM2XAX_OPCODE = 0xa1 , MOV_IMM2XAX_OPCODE = 0xb8 , MOV_IMM2XBX_OPCODE = 0xbb , MOV_IMM2MEM_OPCODE = 0xc7 , JECXZ_OPCODE = 0xe3 , JMP_SHORT_OPCODE = 0xeb , JMP_OPCODE = 0xe9 , JNE_OPCODE_1 = 0x0f , SAHF_OPCODE = 0x9e , LAHF_OPCODE = 0x9f , SETO_OPCODE_1 = 0x0f , SETO_OPCODE_2 = 0x90 , ADD_AL_OPCODE = 0x04 , INC_MEM32_OPCODE_1 = 0xff , MODRM16_DISP16 = 0x06 , SIB_DISP32 = 0x25 , } ;
enum { NUM_REGPARM = 6 , REGPARM_0 = DR_REG_RDI , REGPARM_1 = DR_REG_RSI , REGPARM_2 = DR_REG_RDX , REGPARM_3 = DR_REG_RCX , REGPARM_4 = DR_REG_R8 , REGPARM_5 = DR_REG_R9 , REGPARM_MINSTACK = 0 , REDZONE_SIZE = 128 , REGPARM_END_ALIGN = 16 , } ;
enum { OP_INVALID , OP_UNDECODED , OP_CONTD , OP_LABEL , OP_add , OP_or , OP_adc , OP_sbb , OP_and , OP_daa , OP_sub , OP_das , OP_xor , OP_aaa , OP_cmp , OP_aas , OP_inc , OP_dec , OP_push , OP_push_imm , OP_pop , OP_pusha , OP_popa , OP_bound , OP_arpl , OP_imul , OP_jo_short , OP_jno_short , OP_jb_short , OP_jnb_short , OP_jz_short , OP_jnz_short , OP_jbe_short , OP_jnbe_short , OP_js_short , OP_jns_short , OP_jp_short , OP_jnp_short , OP_jl_short , OP_jnl_short , OP_jle_short , OP_jnle_short , OP_call , OP_call_ind , OP_call_far , OP_call_far_ind , OP_jmp , OP_jmp_short , OP_jmp_ind , OP_jmp_far , OP_jmp_far_ind , OP_loopne , OP_loope , OP_loop , OP_jecxz , OP_mov_ld , OP_mov_st , OP_mov_imm , OP_mov_seg , OP_mov_priv , OP_test , OP_lea , OP_xchg , OP_cwde , OP_cdq , OP_fwait , OP_pushf , OP_popf , OP_sahf , OP_lahf , OP_ret , OP_ret_far , OP_les , OP_lds , OP_enter , OP_leave , OP_int3 , OP_int , OP_into , OP_iret , OP_aam , OP_aad , OP_xlat , OP_in , OP_out , OP_hlt , OP_cmc , OP_clc , OP_stc , OP_cli , OP_sti , OP_cld , OP_std , OP_lar , OP_lsl , OP_syscall , OP_clts , OP_sysret , OP_invd , OP_wbinvd , OP_ud2a , OP_nop_modrm , OP_movntps , OP_movntpd , OP_wrmsr , OP_rdtsc , OP_rdmsr , OP_rdpmc , OP_sysenter , OP_sysexit , OP_cmovo , OP_cmovno , OP_cmovb , OP_cmovnb , OP_cmovz , OP_cmovnz , OP_cmovbe , OP_cmovnbe , OP_cmovs , OP_cmovns , OP_cmovp , OP_cmovnp , OP_cmovl , OP_cmovnl , OP_cmovle , OP_cmovnle , OP_punpcklbw , OP_punpcklwd , OP_punpckldq , OP_packsswb , OP_pcmpgtb , OP_pcmpgtw , OP_pcmpgtd , OP_packuswb , OP_punpckhbw , OP_punpckhwd , OP_punpckhdq , OP_packssdw , OP_punpcklqdq , OP_punpckhqdq , OP_movd , OP_movq , OP_movdqu , OP_movdqa , OP_pshufw , OP_pshufd , OP_pshufhw , OP_pshuflw , OP_pcmpeqb , OP_pcmpeqw , OP_pcmpeqd , OP_emms , OP_jo , OP_jno , OP_jb , OP_jnb , OP_jz , OP_jnz , OP_jbe , OP_jnbe , OP_js , OP_jns , OP_jp , OP_jnp , OP_jl , OP_jnl , OP_jle , OP_jnle , OP_seto , OP_setno , OP_setb , OP_setnb , OP_setz , OP_setnz , OP_setbe , OP_setnbe , OP_sets , OP_setns , OP_setp , OP_setnp , OP_setl , OP_setnl , OP_setle , OP_setnle , OP_cpuid , OP_bt , OP_shld , OP_rsm , OP_bts , OP_shrd , OP_cmpxchg , OP_lss , OP_btr , OP_lfs , OP_lgs , OP_movzx , OP_ud2b , OP_btc , OP_bsf , OP_bsr , OP_movsx , OP_xadd , OP_movnti , OP_pinsrw , OP_pextrw , OP_bswap , OP_psrlw , OP_psrld , OP_psrlq , OP_paddq , OP_pmullw , OP_pmovmskb , OP_psubusb , OP_psubusw , OP_pminub , OP_pand , OP_paddusb , OP_paddusw , OP_pmaxub , OP_pandn , OP_pavgb , OP_psraw , OP_psrad , OP_pavgw , OP_pmulhuw , OP_pmulhw , OP_movntq , OP_movntdq , OP_psubsb , OP_psubsw , OP_pminsw , OP_por , OP_paddsb , OP_paddsw , OP_pmaxsw , OP_pxor , OP_psllw , OP_pslld , OP_psllq , OP_pmuludq , OP_pmaddwd , OP_psadbw , OP_maskmovq , OP_maskmovdqu , OP_psubb , OP_psubw , OP_psubd , OP_psubq , OP_paddb , OP_paddw , OP_paddd , OP_psrldq , OP_pslldq , OP_rol , OP_ror , OP_rcl , OP_rcr , OP_shl , OP_shr , OP_sar , OP_not , OP_neg , OP_mul , OP_div , OP_idiv , OP_sldt , OP_str , OP_lldt , OP_ltr , OP_verr , OP_verw , OP_sgdt , OP_sidt , OP_lgdt , OP_lidt , OP_smsw , OP_lmsw , OP_invlpg , OP_cmpxchg8b , OP_fxsave32 , OP_fxrstor32 , OP_ldmxcsr , OP_stmxcsr , OP_lfence , OP_mfence , OP_clflush , OP_sfence , OP_prefetchnta , OP_prefetcht0 , OP_prefetcht1 , OP_prefetcht2 , OP_prefetch , OP_prefetchw , OP_movups , OP_movss , OP_movupd , OP_movsd , OP_movlps , OP_movlpd , OP_unpcklps , OP_unpcklpd , OP_unpckhps , OP_unpckhpd , OP_movhps , OP_movhpd , OP_movaps , OP_movapd , OP_cvtpi2ps , OP_cvtsi2ss , OP_cvtpi2pd , OP_cvtsi2sd , OP_cvttps2pi , OP_cvttss2si , OP_cvttpd2pi , OP_cvttsd2si , OP_cvtps2pi , OP_cvtss2si , OP_cvtpd2pi , OP_cvtsd2si , OP_ucomiss , OP_ucomisd , OP_comiss , OP_comisd , OP_movmskps , OP_movmskpd , OP_sqrtps , OP_sqrtss , OP_sqrtpd , OP_sqrtsd , OP_rsqrtps , OP_rsqrtss , OP_rcpps , OP_rcpss , OP_andps , OP_andpd , OP_andnps , OP_andnpd , OP_orps , OP_orpd , OP_xorps , OP_xorpd , OP_addps , OP_addss , OP_addpd , OP_addsd , OP_mulps , OP_mulss , OP_mulpd , OP_mulsd , OP_cvtps2pd , OP_cvtss2sd , OP_cvtpd2ps , OP_cvtsd2ss , OP_cvtdq2ps , OP_cvttps2dq , OP_cvtps2dq , OP_subps , OP_subss , OP_subpd , OP_subsd , OP_minps , OP_minss , OP_minpd , OP_minsd , OP_divps , OP_divss , OP_divpd , OP_divsd , OP_maxps , OP_maxss , OP_maxpd , OP_maxsd , OP_cmpps , OP_cmpss , OP_cmppd , OP_cmpsd , OP_shufps , OP_shufpd , OP_cvtdq2pd , OP_cvttpd2dq , OP_cvtpd2dq , OP_nop , OP_pause , OP_ins , OP_rep_ins , OP_outs , OP_rep_outs , OP_movs , OP_rep_movs , OP_stos , OP_rep_stos , OP_lods , OP_rep_lods , OP_cmps , OP_rep_cmps , OP_repne_cmps , OP_scas , OP_rep_scas , OP_repne_scas , OP_fadd , OP_fmul , OP_fcom , OP_fcomp , OP_fsub , OP_fsubr , OP_fdiv , OP_fdivr , OP_fld , OP_fst , OP_fstp , OP_fldenv , OP_fldcw , OP_fnstenv , OP_fnstcw , OP_fiadd , OP_fimul , OP_ficom , OP_ficomp , OP_fisub , OP_fisubr , OP_fidiv , OP_fidivr , OP_fild , OP_fist , OP_fistp , OP_frstor , OP_fnsave , OP_fnstsw , OP_fbld , OP_fbstp , OP_fxch , OP_fnop , OP_fchs , OP_fabs , OP_ftst , OP_fxam , OP_fld1 , OP_fldl2t , OP_fldl2e , OP_fldpi , OP_fldlg2 , OP_fldln2 , OP_fldz , OP_f2xm1 , OP_fyl2x , OP_fptan , OP_fpatan , OP_fxtract , OP_fprem1 , OP_fdecstp , OP_fincstp , OP_fprem , OP_fyl2xp1 , OP_fsqrt , OP_fsincos , OP_frndint , OP_fscale , OP_fsin , OP_fcos , OP_fcmovb , OP_fcmove , OP_fcmovbe , OP_fcmovu , OP_fucompp , OP_fcmovnb , OP_fcmovne , OP_fcmovnbe , OP_fcmovnu , OP_fnclex , OP_fninit , OP_fucomi , OP_fcomi , OP_ffree , OP_fucom , OP_fucomp , OP_faddp , OP_fmulp , OP_fcompp , OP_fsubrp , OP_fsubp , OP_fdivrp , OP_fdivp , OP_fucomip , OP_fcomip , OP_fisttp , OP_haddpd , OP_haddps , OP_hsubpd , OP_hsubps , OP_addsubpd , OP_addsubps , OP_lddqu , OP_monitor , OP_mwait , OP_movsldup , OP_movshdup , OP_movddup , OP_femms , OP_unknown_3dnow , OP_pavgusb , OP_pfadd , OP_pfacc , OP_pfcmpge , OP_pfcmpgt , OP_pfcmpeq , OP_pfmin , OP_pfmax , OP_pfmul , OP_pfrcp , OP_pfrcpit1 , OP_pfrcpit2 , OP_pfrsqrt , OP_pfrsqit1 , OP_pmulhrw , OP_pfsub , OP_pfsubr , OP_pi2fd , OP_pf2id , OP_pi2fw , OP_pf2iw , OP_pfnacc , OP_pfpnacc , OP_pswapd , OP_pshufb , OP_phaddw , OP_phaddd , OP_phaddsw , OP_pmaddubsw , OP_phsubw , OP_phsubd , OP_phsubsw , OP_psignb , OP_psignw , OP_psignd , OP_pmulhrsw , OP_pabsb , OP_pabsw , OP_pabsd , OP_palignr , OP_popcnt , OP_movntss , OP_movntsd , OP_extrq , OP_insertq , OP_lzcnt , OP_pblendvb , OP_blendvps , OP_blendvpd , OP_ptest , OP_pmovsxbw , OP_pmovsxbd , OP_pmovsxbq , OP_pmovsxdw , OP_pmovsxwq , OP_pmovsxdq , OP_pmuldq , OP_pcmpeqq , OP_movntdqa , OP_packusdw , OP_pmovzxbw , OP_pmovzxbd , OP_pmovzxbq , OP_pmovzxdw , OP_pmovzxwq , OP_pmovzxdq , OP_pcmpgtq , OP_pminsb , OP_pminsd , OP_pminuw , OP_pminud , OP_pmaxsb , OP_pmaxsd , OP_pmaxuw , OP_pmaxud , OP_pmulld , OP_phminposuw , OP_crc32 , OP_pextrb , OP_pextrd , OP_extractps , OP_roundps , OP_roundpd , OP_roundss , OP_roundsd , OP_blendps , OP_blendpd , OP_pblendw , OP_pinsrb , OP_insertps , OP_pinsrd , OP_dpps , OP_dppd , OP_mpsadbw , OP_pcmpestrm , OP_pcmpestri , OP_pcmpistrm , OP_pcmpistri , OP_movsxd , OP_swapgs , OP_vmcall , OP_vmlaunch , OP_vmresume , OP_vmxoff , OP_vmptrst , OP_vmptrld , OP_vmxon , OP_vmclear , OP_vmread , OP_vmwrite , OP_int1 , OP_salc , OP_ffreep , OP_vmrun , OP_vmmcall , OP_vmload , OP_vmsave , OP_stgi , OP_clgi , OP_skinit , OP_invlpga , OP_rdtscp , OP_invept , OP_invvpid , OP_pclmulqdq , OP_aesimc , OP_aesenc , OP_aesenclast , OP_aesdec , OP_aesdeclast , OP_aeskeygenassist , OP_movbe , OP_xgetbv , OP_xsetbv , OP_xsave32 , OP_xrstor32 , OP_xsaveopt32 , OP_vmovss , OP_vmovsd , OP_vmovups , OP_vmovupd , OP_vmovlps , OP_vmovsldup , OP_vmovlpd , OP_vmovddup , OP_vunpcklps , OP_vunpcklpd , OP_vunpckhps , OP_vunpckhpd , OP_vmovhps , OP_vmovshdup , OP_vmovhpd , OP_vmovaps , OP_vmovapd , OP_vcvtsi2ss , OP_vcvtsi2sd , OP_vmovntps , OP_vmovntpd , OP_vcvttss2si , OP_vcvttsd2si , OP_vcvtss2si , OP_vcvtsd2si , OP_vucomiss , OP_vucomisd , OP_vcomiss , OP_vcomisd , OP_vmovmskps , OP_vmovmskpd , OP_vsqrtps , OP_vsqrtss , OP_vsqrtpd , OP_vsqrtsd , OP_vrsqrtps , OP_vrsqrtss , OP_vrcpps , OP_vrcpss , OP_vandps , OP_vandpd , OP_vandnps , OP_vandnpd , OP_vorps , OP_vorpd , OP_vxorps , OP_vxorpd , OP_vaddps , OP_vaddss , OP_vaddpd , OP_vaddsd , OP_vmulps , OP_vmulss , OP_vmulpd , OP_vmulsd , OP_vcvtps2pd , OP_vcvtss2sd , OP_vcvtpd2ps , OP_vcvtsd2ss , OP_vcvtdq2ps , OP_vcvttps2dq , OP_vcvtps2dq , OP_vsubps , OP_vsubss , OP_vsubpd , OP_vsubsd , OP_vminps , OP_vminss , OP_vminpd , OP_vminsd , OP_vdivps , OP_vdivss , OP_vdivpd , OP_vdivsd , OP_vmaxps , OP_vmaxss , OP_vmaxpd , OP_vmaxsd , OP_vpunpcklbw , OP_vpunpcklwd , OP_vpunpckldq , OP_vpacksswb , OP_vpcmpgtb , OP_vpcmpgtw , OP_vpcmpgtd , OP_vpackuswb , OP_vpunpckhbw , OP_vpunpckhwd , OP_vpunpckhdq , OP_vpackssdw , OP_vpunpcklqdq , OP_vpunpckhqdq , OP_vmovd , OP_vpshufhw , OP_vpshufd , OP_vpshuflw , OP_vpcmpeqb , OP_vpcmpeqw , OP_vpcmpeqd , OP_vmovq , OP_vcmpps , OP_vcmpss , OP_vcmppd , OP_vcmpsd , OP_vpinsrw , OP_vpextrw , OP_vshufps , OP_vshufpd , OP_vpsrlw , OP_vpsrld , OP_vpsrlq , OP_vpaddq , OP_vpmullw , OP_vpmovmskb , OP_vpsubusb , OP_vpsubusw , OP_vpminub , OP_vpand , OP_vpaddusb , OP_vpaddusw , OP_vpmaxub , OP_vpandn , OP_vpavgb , OP_vpsraw , OP_vpsrad , OP_vpavgw , OP_vpmulhuw , OP_vpmulhw , OP_vcvtdq2pd , OP_vcvttpd2dq , OP_vcvtpd2dq , OP_vmovntdq , OP_vpsubsb , OP_vpsubsw , OP_vpminsw , OP_vpor , OP_vpaddsb , OP_vpaddsw , OP_vpmaxsw , OP_vpxor , OP_vpsllw , OP_vpslld , OP_vpsllq , OP_vpmuludq , OP_vpmaddwd , OP_vpsadbw , OP_vmaskmovdqu , OP_vpsubb , OP_vpsubw , OP_vpsubd , OP_vpsubq , OP_vpaddb , OP_vpaddw , OP_vpaddd , OP_vpsrldq , OP_vpslldq , OP_vmovdqu , OP_vmovdqa , OP_vhaddpd , OP_vhaddps , OP_vhsubpd , OP_vhsubps , OP_vaddsubpd , OP_vaddsubps , OP_vlddqu , OP_vpshufb , OP_vphaddw , OP_vphaddd , OP_vphaddsw , OP_vpmaddubsw , OP_vphsubw , OP_vphsubd , OP_vphsubsw , OP_vpsignb , OP_vpsignw , OP_vpsignd , OP_vpmulhrsw , OP_vpabsb , OP_vpabsw , OP_vpabsd , OP_vpalignr , OP_vpblendvb , OP_vblendvps , OP_vblendvpd , OP_vptest , OP_vpmovsxbw , OP_vpmovsxbd , OP_vpmovsxbq , OP_vpmovsxdw , OP_vpmovsxwq , OP_vpmovsxdq , OP_vpmuldq , OP_vpcmpeqq , OP_vmovntdqa , OP_vpackusdw , OP_vpmovzxbw , OP_vpmovzxbd , OP_vpmovzxbq , OP_vpmovzxdw , OP_vpmovzxwq , OP_vpmovzxdq , OP_vpcmpgtq , OP_vpminsb , OP_vpminsd , OP_vpminuw , OP_vpminud , OP_vpmaxsb , OP_vpmaxsd , OP_vpmaxuw , OP_vpmaxud , OP_vpmulld , OP_vphminposuw , OP_vaesimc , OP_vaesenc , OP_vaesenclast , OP_vaesdec , OP_vaesdeclast , OP_vpextrb , OP_vpextrd , OP_vextractps , OP_vroundps , OP_vroundpd , OP_vroundss , OP_vroundsd , OP_vblendps , OP_vblendpd , OP_vpblendw , OP_vpinsrb , OP_vinsertps , OP_vpinsrd , OP_vdpps , OP_vdppd , OP_vmpsadbw , OP_vpcmpestrm , OP_vpcmpestri , OP_vpcmpistrm , OP_vpcmpistri , OP_vpclmulqdq , OP_vaeskeygenassist , OP_vtestps , OP_vtestpd , OP_vzeroupper , OP_vzeroall , OP_vldmxcsr , OP_vstmxcsr , OP_vbroadcastss , OP_vbroadcastsd , OP_vbroadcastf128 , OP_vmaskmovps , OP_vmaskmovpd , OP_vpermilps , OP_vpermilpd , OP_vperm2f128 , OP_vinsertf128 , OP_vextractf128 , OP_vcvtph2ps , OP_vcvtps2ph , OP_vfmadd132ps , OP_vfmadd132pd , OP_vfmadd213ps , OP_vfmadd213pd , OP_vfmadd231ps , OP_vfmadd231pd , OP_vfmadd132ss , OP_vfmadd132sd , OP_vfmadd213ss , OP_vfmadd213sd , OP_vfmadd231ss , OP_vfmadd231sd , OP_vfmaddsub132ps , OP_vfmaddsub132pd , OP_vfmaddsub213ps , OP_vfmaddsub213pd , OP_vfmaddsub231ps , OP_vfmaddsub231pd , OP_vfmsubadd132ps , OP_vfmsubadd132pd , OP_vfmsubadd213ps , OP_vfmsubadd213pd , OP_vfmsubadd231ps , OP_vfmsubadd231pd , OP_vfmsub132ps , OP_vfmsub132pd , OP_vfmsub213ps , OP_vfmsub213pd , OP_vfmsub231ps , OP_vfmsub231pd , OP_vfmsub132ss , OP_vfmsub132sd , OP_vfmsub213ss , OP_vfmsub213sd , OP_vfmsub231ss , OP_vfmsub231sd , OP_vfnmadd132ps , OP_vfnmadd132pd , OP_vfnmadd213ps , OP_vfnmadd213pd , OP_vfnmadd231ps , OP_vfnmadd231pd , OP_vfnmadd132ss , OP_vfnmadd132sd , OP_vfnmadd213ss , OP_vfnmadd213sd , OP_vfnmadd231ss , OP_vfnmadd231sd , OP_vfnmsub132ps , OP_vfnmsub132pd , OP_vfnmsub213ps , OP_vfnmsub213pd , OP_vfnmsub231ps , OP_vfnmsub231pd , OP_vfnmsub132ss , OP_vfnmsub132sd , OP_vfnmsub213ss , OP_vfnmsub213sd , OP_vfnmsub231ss , OP_vfnmsub231sd , OP_movq2dq , OP_movdq2q , OP_fxsave64 , OP_fxrstor64 , OP_xsave64 , OP_xrstor64 , OP_xsaveopt64 , OP_AFTER_LAST , OP_FIRST = OP_add , OP_LAST = OP_AFTER_LAST - 1 , } ;
typedef struct __locale_struct { struct locale_data * __locales [ 13 ] ; const unsigned short int * __ctype_b ; const int * __ctype_tolower ; const int * __ctype_toupper ; const char * __names [ 13 ] ; } * __locale_t ;
typedef __locale_t locale_t ;
instrlist_t * instrlist_create ( dcontext_t * dcontext ) ;
void instrlist_init ( instrlist_t * ilist ) ;
void instrlist_destroy ( dcontext_t * dcontext , instrlist_t * ilist ) ;
void instrlist_clear ( dcontext_t * dcontext , instrlist_t * ilist ) ;
void instrlist_clear_and_destroy ( dcontext_t * dcontext , instrlist_t * ilist ) ;
bool instrlist_set_fall_through_target ( instrlist_t * bb , app_pc tgt ) ;
app_pc instrlist_get_fall_through_target ( instrlist_t * bb ) ;
bool instrlist_set_return_target ( instrlist_t * bb , app_pc tgt ) ;
app_pc instrlist_get_return_target ( instrlist_t * bb ) ;
void instrlist_set_translation_target ( instrlist_t * ilist , app_pc pc ) ;
app_pc instrlist_get_translation_target ( instrlist_t * ilist ) ;
void instrlist_set_our_mangling ( instrlist_t * ilist , bool ours ) ;
bool instrlist_get_our_mangling ( instrlist_t * ilist ) ;
instr_t * instrlist_first ( instrlist_t * ilist ) ;
instr_t * instrlist_last ( instrlist_t * ilist ) ;
void instrlist_append ( instrlist_t * ilist , instr_t * inst ) ;
void instrlist_prepend ( instrlist_t * ilist , instr_t * inst ) ;
void instrlist_preinsert ( instrlist_t * ilist , instr_t * where , instr_t * inst ) ;
void instrlist_postinsert ( instrlist_t * ilist , instr_t * where , instr_t * inst ) ;
instr_t * instrlist_replace ( instrlist_t * ilist , instr_t * oldinst , instr_t * newinst ) ;
void instrlist_remove ( instrlist_t * ilist , instr_t * inst ) ;
instrlist_t * instrlist_clone ( dcontext_t * dcontext , instrlist_t * old ) ;
void instrlist_prepend_instrlist ( dcontext_t * dcontext , instrlist_t * ilist , instrlist_t * prependee ) ;
void instrlist_append_instrlist ( dcontext_t * dcontext , instrlist_t * ilist , instrlist_t * appendee ) ;
typedef long int ptrdiff_t ;
enum { OPCODE_TWOBYTES = 0x00000010 , OPCODE_REG = 0x00000020 , OPCODE_MODRM = 0x00000040 , OPCODE_SUFFIX = 0x00000080 , OPCODE_THREEBYTES = 0x00000008 , } ;
typedef struct instr_info_t { int type ; uint opcode ; const char * name ; byte dst1_type ; opnd_size_t dst1_size ; byte dst2_type ; opnd_size_t dst2_size ; byte src1_type ; opnd_size_t src1_size ; byte src2_type ; opnd_size_t src2_size ; byte src3_type ; opnd_size_t src3_size ; byte flags ; uint eflags ; ptr_int_t code ; } instr_info_t ;
enum { INVALID = OP_LAST + 1 , PREFIX , ESCAPE , FLOAT_EXT , EXTENSION , PREFIX_EXT , REP_EXT , REPNE_EXT , MOD_EXT , RM_EXT , SUFFIX_EXT , X64_EXT , ESCAPE_3BYTE_38 , ESCAPE_3BYTE_3a , REX_B_EXT , REX_W_EXT , VEX_PREFIX_EXT , VEX_EXT , VEX_L_EXT , VEX_W_EXT , } ;
typedef struct decode_info_t { uint opcode ; uint prefixes ; byte seg_override ; byte modrm ; byte mod ; byte reg ; byte rm ; bool has_sib ; byte scale ; byte index ; byte base ; bool has_disp ; int disp ; opnd_size_t size_immed ; opnd_size_t size_immed2 ; bool immed_pc_relativize : 1 ; bool immed_subtract_length : 1 ; bool immed_pc_rel_offs : 1 ; ushort immed_shift ; ptr_int_t immed ; ptr_int_t immed2 ; byte * start_pc ; byte * final_pc ; uint len ; byte * disp_abs ; bool x86_mode ; byte * orig_pc ; bool data_prefix ; bool rep_prefix ; bool repne_prefix ; byte vex_vvvv ; bool vex_encoded ; ptr_int_t cur_note ; bool has_instr_opnds ; } decode_info_t ;
enum { TYPE_NONE , TYPE_A , TYPE_C , TYPE_D , TYPE_E , TYPE_G , TYPE_H , TYPE_I , TYPE_J , TYPE_L , TYPE_M , TYPE_O , TYPE_P , TYPE_Q , TYPE_R , TYPE_S , TYPE_V , TYPE_W , TYPE_X , TYPE_Y , TYPE_P_MODRM , TYPE_V_MODRM , TYPE_1 , TYPE_FLOATCONST , TYPE_XLAT , TYPE_MASKMOVQ , TYPE_FLOATMEM , TYPE_REG , TYPE_VAR_REG , TYPE_VARZ_REG , TYPE_VAR_XREG , TYPE_VAR_ADDR_XREG , TYPE_REG_EX , TYPE_VAR_REG_EX , TYPE_VAR_XREG_EX , TYPE_VAR_REGX_EX , TYPE_INDIR_E , TYPE_INDIR_REG , TYPE_INDIR_VAR_XREG , TYPE_INDIR_VAR_REG , TYPE_INDIR_VAR_XIREG , TYPE_INDIR_VAR_XREG_OFFS_1 , TYPE_INDIR_VAR_XREG_OFFS_8 , TYPE_INDIR_VAR_XREG_OFFS_N , TYPE_INDIR_VAR_XIREG_OFFS_1 , TYPE_INDIR_VAR_REG_OFFS_2 , TYPE_INDIR_VAR_XREG_SIZEx8 , TYPE_INDIR_VAR_REG_SIZEx2 , TYPE_INDIR_VAR_REG_SIZEx3x5 , } ;
enum { OPSZ_NA = DR_REG_INVALID + 1 , OPSZ_FIRST = OPSZ_NA , OPSZ_0 , OPSZ_1 , OPSZ_2 , OPSZ_4 , OPSZ_6 , OPSZ_8 , OPSZ_10 , OPSZ_16 , OPSZ_14 , OPSZ_28 , OPSZ_94 , OPSZ_108 , OPSZ_512 , OPSZ_2_short1 , OPSZ_4_short2 , OPSZ_4_rex8_short2 , OPSZ_4_rex8 , OPSZ_6_irex10_short4 , OPSZ_8_short2 , OPSZ_8_short4 , OPSZ_28_short14 , OPSZ_108_short94 , OPSZ_4x8 , OPSZ_6x10 , OPSZ_4x8_short2 , OPSZ_4x8_short2xi8 , OPSZ_4_short2xi4 , OPSZ_1_reg4 , OPSZ_2_reg4 , OPSZ_4_reg16 , OPSZ_xsave , OPSZ_12 , OPSZ_32 , OPSZ_40 , OPSZ_32_short16 , OPSZ_8_rex16 , OPSZ_8_rex16_short4 , OPSZ_12_rex40_short6 , OPSZ_16_vex32 , OPSZ_LAST , } ;
enum { OPSZ_4_of_8 = OPSZ_LAST , OPSZ_4_of_16 , OPSZ_8_of_16 , OPSZ_8_of_16_vex32 , OPSZ_16_of_32 , OPSZ_LAST_ENUM , } ;
typedef enum { IBL_UNLINKED , IBL_DELETE , IBL_FAR , IBL_FAR_UNLINKED , IBL_TRACE_CMP , IBL_TRACE_CMP_UNLINKED , IBL_LINKED , IBL_TEMPLATE , IBL_LINK_STATE_END } ibl_entry_point_type_t ;
typedef enum { IBL_BB_SHARED , IBL_SOURCE_TYPE_START = IBL_BB_SHARED , IBL_TRACE_SHARED , IBL_BB_PRIVATE , IBL_TRACE_PRIVATE , IBL_COARSE_SHARED , IBL_SOURCE_TYPE_END } ibl_source_fragment_type_t ;
typedef struct { ibl_entry_point_type_t link_state ; ibl_source_fragment_type_t source_fragment_type ; ibl_branch_type_t branch_type ; } ibl_type_t ;
typedef enum { GENCODE_X64 = 0 , GENCODE_X86 , GENCODE_X86_TO_X64 , GENCODE_FROM_DCONTEXT , } gencode_mode_t ;
typedef struct _clean_call_info_t { void * callee ; uint num_args ; bool save_fpstate ; bool opt_inline ; bool should_align ; bool save_all_regs ; bool skip_save_aflags ; bool skip_clear_eflags ; uint num_xmms_skip ; bool xmm_skip [ 16 ] ; uint num_regs_skip ; bool reg_skip [ ( 1 + ( DR_REG_R15 - DR_REG_RAX ) ) ] ; bool preserve_mcontext ; void * callee_info ; instrlist_t * ilist ; } clean_call_info_t ;
enum { FCACHE_ENTER_TARGET_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xax ) ) , MANGLE_NEXT_TAG_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xax ) ) , DIRECT_STUB_SPILL_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xax ) ) , MANGLE_RIPREL_SPILL_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xax ) ) , INDIRECT_STUB_SPILL_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xbx ) ) , MANGLE_FAR_SPILL_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xbx ) ) , FLOAT_PC_STATE_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xbx ) ) , MANGLE_XCX_SPILL_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xcx ) ) , DCONTEXT_BASE_SPILL_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xdx ) ) , PREFIX_XAX_SPILL_SLOT = ( ( ushort ) __builtin_offsetof ( spill_state_t , xax ) ) , } ;
typedef struct patch_entry_t { union { instr_t * instr ; size_t offset ; } where ; ptr_uint_t value_location_offset ; ushort patch_flags ; short instr_offset ; } patch_entry_t ;
enum { MAX_PATCH_ENTRIES = 7 , PATCH_TAKE_ADDRESS = 0x01 , PATCH_PER_THREAD = 0x02 , PATCH_UNPROT_STAT = 0x04 , PATCH_MARKER = 0x08 , PATCH_ASSEMBLE_ABSOLUTE = 0x10 , PATCH_OFFSET_VALID = 0x20 , PATCH_UINT_SIZED = 0x40 , } ;
typedef enum { PATCH_TYPE_ABSOLUTE = 0x0 , PATCH_TYPE_INDIRECT_XDI = 0x1 , PATCH_TYPE_INDIRECT_FS = 0x2 , } patch_list_type_t ;
typedef struct patch_list_t { ushort num_relocations ; ushort type ; patch_entry_t entry [ MAX_PATCH_ENTRIES ] ; } patch_list_t ;
typedef struct _far_ref_t { uint pc ; ushort selector ; } far_ref_t ;
typedef struct ibl_code_t { bool initialized : 1 ; bool thread_shared_routine : 1 ; bool ibl_head_is_inlined : 1 ; byte * indirect_branch_lookup_routine ; byte * far_ibl ; byte * far_ibl_unlinked ; byte * trace_cmp_entry ; byte * trace_cmp_unlinked ; bool x86_mode ; bool x86_to_x64_mode ; far_ref_t far_jmp_opnd ; far_ref_t far_jmp_unlinked_opnd ; byte * unlinked_ibl_entry ; byte * target_delete_entry ; uint ibl_routine_length ; patch_list_t ibl_patch ; ibl_branch_type_t branch_type ; ibl_source_fragment_type_t source_fragment_type ; byte * inline_ibl_stub_template ; patch_list_t ibl_stub_patch ; uint inline_stub_length ; uint inline_linkstub_first_offs ; uint inline_linkstub_second_offs ; uint inline_unlink_offs ; uint inline_linkedjmp_offs ; uint inline_unlinkedjmp_offs ; } ibl_code_t ;
typedef struct _generated_code_t { byte * fcache_enter ; byte * fcache_return ; ibl_code_t trace_ibl [ IBL_BRANCH_TYPE_END ] ; ibl_code_t bb_ibl [ IBL_BRANCH_TYPE_END ] ; ibl_code_t coarse_ibl [ IBL_BRANCH_TYPE_END ] ; byte * do_syscall ; uint do_syscall_offs ; byte * do_int_syscall ; uint do_int_syscall_offs ; byte * do_clone_syscall ; uint do_clone_syscall_offs ; byte * new_thread_dynamo_start ; byte * reset_exit_stub ; byte * fcache_return_coarse ; byte * trace_head_return_coarse ; byte * client_ibl_xfer ; uint client_ibl_unlink_offs ; byte * clean_call_save ; byte * clean_call_restore ; bool thread_shared ; bool writable ; gencode_mode_t gencode_mode ; byte * gen_start_pc ; byte * gen_end_pc ; byte * commit_end_pc ; } generated_code_t ;
bool set_x86_mode ( dcontext_t * dcontext , bool x86 ) ;
bool get_x86_mode ( dcontext_t * dcontext ) ;
opnd_size_t resolve_var_reg_size ( opnd_size_t sz , bool is_reg ) ;
opnd_size_t resolve_variable_size ( decode_info_t * di , opnd_size_t sz , bool is_reg ) ;
opnd_size_t resolve_variable_size_dc ( dcontext_t * dcontext , uint prefixes , opnd_size_t sz , bool is_reg ) ;
opnd_size_t resolve_addr_size ( decode_info_t * di ) ;
bool optype_is_indir_reg ( int optype ) ;
opnd_size_t indir_var_reg_size ( decode_info_t * di , int optype ) ;
int indir_var_reg_offs_factor ( int optype ) ;
typedef enum { DECODE_REG_REG , DECODE_REG_BASE , DECODE_REG_INDEX , DECODE_REG_RM , } decode_reg_t ;
reg_id_t resolve_var_reg ( decode_info_t * di , reg_id_t reg32 , bool addr , bool can_shrink , bool default_64 , bool can_grow , bool extendable ) ;
byte * decode_eflags_usage ( dcontext_t * dcontext , byte * pc , uint * usage ) ;
byte * decode_opcode ( dcontext_t * dcontext , byte * pc , instr_t * instr ) ;
byte * decode ( dcontext_t * dcontext , byte * pc , instr_t * instr ) ;
byte * decode_from_copy ( dcontext_t * dcontext , byte * copy_pc , byte * orig_pc , instr_t * instr ) ;
const instr_info_t * get_next_instr_info ( const instr_info_t * info ) ;
byte decode_first_opcode_byte ( int opcode ) ;
typedef enum { DR_DISASM_DR = 0x0 , DR_DISASM_INTEL = 0x1 , DR_DISASM_ATT = 0x2 , DR_DISASM_STRICT_INVALID = 0x4 , } dr_disasm_flags_t ;
const instr_info_t * instr_info_extra_opnds ( const instr_info_t * info ) ;
bool instr_is_encoding_possible ( instr_t * instr ) ;
const instr_info_t * get_encoding_info ( instr_t * instr ) ;
byte instr_info_opnd_type ( const instr_info_t * info , bool src , int num ) ;
byte * copy_and_re_relativize_raw_instr ( dcontext_t * dcontext , instr_t * instr , byte * dst_pc , byte * final_pc ) ;
byte * instr_encode_ignore_reachability ( dcontext_t * dcontext , instr_t * instr , byte * pc ) ;
byte * instr_encode_check_reachability ( dcontext_t * dcontext , instr_t * instr , byte * pc , bool * has_instr_opnds ) ;
byte * instr_encode_to_copy ( dcontext_t * dcontext , instr_t * instr , byte * copy_pc , byte * final_pc ) ;
byte * instr_encode ( dcontext_t * dcontext , instr_t * instr , byte * pc ) ;
byte * instrlist_encode_to_copy ( dcontext_t * dcontext , instrlist_t * ilist , byte * copy_pc , byte * final_pc , byte * max_pc , bool has_instr_jmp_targets ) ;
byte * instrlist_encode ( dcontext_t * dcontext , instrlist_t * ilist , byte * pc , bool has_instr_jmp_targets ) ;

bool opnd_is_null ( opnd_t op ) ;
bool opnd_is_immed_int ( opnd_t op ) ;
bool opnd_is_immed_float ( opnd_t op ) ;
bool opnd_is_near_pc ( opnd_t op ) ;
bool opnd_is_near_instr ( opnd_t op ) ;
bool opnd_is_reg ( opnd_t op ) ;
bool opnd_is_base_disp ( opnd_t op ) ;
bool opnd_is_far_pc ( opnd_t op ) ;
bool opnd_is_far_instr ( opnd_t op ) ;
bool opnd_is_mem_instr ( opnd_t op ) ;
bool opnd_is_valid ( opnd_t op ) ;
bool opnd_is_rel_addr ( opnd_t op ) ;
bool opnd_is_abs_addr ( opnd_t opnd ) ;
bool opnd_is_near_abs_addr ( opnd_t opnd ) ;
bool opnd_is_far_abs_addr ( opnd_t opnd ) ;
bool opnd_is_reg_32bit ( opnd_t opnd ) ;
bool reg_is_32bit ( reg_id_t reg ) ;
bool opnd_is_reg_64bit ( opnd_t opnd ) ;
bool reg_is_64bit ( reg_id_t reg ) ;
bool opnd_is_reg_pointer_sized ( opnd_t opnd ) ;
bool reg_is_pointer_sized ( reg_id_t reg ) ;
reg_id_t opnd_get_reg ( opnd_t opnd ) ;
opnd_size_t opnd_get_size ( opnd_t opnd ) ;
void opnd_set_size ( opnd_t * opnd , opnd_size_t newsize ) ;
opnd_t opnd_create_immed_int ( ptr_int_t i , opnd_size_t size ) ;
opnd_t opnd_create_immed_float ( float i ) ;
enum { FLOAT_ZERO = 0x00000000 , FLOAT_ONE = 0x3f800000 , FLOAT_LOG2_10 = 0x40549a78 , FLOAT_LOG2_E = 0x3fb8aa3b , FLOAT_PI = 0x40490fdb , FLOAT_LOG10_2 = 0x3e9a209a , FLOAT_LOGE_2 = 0x3f317218 , } ;
opnd_t opnd_create_immed_float_for_opcode ( uint opcode ) ;
ptr_int_t opnd_get_immed_int ( opnd_t opnd ) ;
float opnd_get_immed_float ( opnd_t opnd ) ;
opnd_t opnd_create_far_pc ( ushort seg_selector , app_pc pc ) ;
opnd_t opnd_create_instr_ex ( instr_t * instr , opnd_size_t size , ushort shift ) ;
opnd_t opnd_create_instr ( instr_t * instr ) ;
opnd_t opnd_create_far_instr ( ushort seg_selector , instr_t * instr ) ;
 opnd_t opnd_create_mem_instr ( instr_t * instr , short disp , opnd_size_t data_size ) ;
app_pc opnd_get_pc ( opnd_t opnd ) ;
ushort opnd_get_segment_selector ( opnd_t opnd ) ;
instr_t * opnd_get_instr ( opnd_t opnd ) ;
 ushort opnd_get_shift ( opnd_t opnd ) ;
short opnd_get_mem_instr_disp ( opnd_t opnd ) ;
opnd_t opnd_create_base_disp_ex ( reg_id_t base_reg , reg_id_t index_reg , int scale , int disp , opnd_size_t size , bool encode_zero_disp , bool force_full_disp , bool disp_short_addr ) ;
opnd_t opnd_create_base_disp ( reg_id_t base_reg , reg_id_t index_reg , int scale , int disp , opnd_size_t size ) ;
opnd_t opnd_create_far_base_disp_ex ( reg_id_t seg , reg_id_t base_reg , reg_id_t index_reg , int scale , int disp , opnd_size_t size , bool encode_zero_disp , bool force_full_disp , bool disp_short_addr ) ;
opnd_t opnd_create_far_base_disp ( reg_id_t seg , reg_id_t base_reg , reg_id_t index_reg , int scale , int disp , opnd_size_t size ) ;
reg_id_t opnd_get_base ( opnd_t opnd ) ;
int opnd_get_disp ( opnd_t opnd ) ;
reg_id_t opnd_get_index ( opnd_t opnd ) ;
int opnd_get_scale ( opnd_t opnd ) ;
reg_id_t opnd_get_segment ( opnd_t opnd ) ;
bool opnd_is_disp_encode_zero ( opnd_t opnd ) ;
bool opnd_is_disp_force_full ( opnd_t opnd ) ;
bool opnd_is_disp_short_addr ( opnd_t opnd ) ;
void opnd_set_disp ( opnd_t * opnd , int disp ) ;
void opnd_set_disp_ex ( opnd_t * opnd , int disp , bool encode_zero_disp , bool force_full_disp , bool disp_short_addr ) ;
opnd_t opnd_create_abs_addr ( void * addr , opnd_size_t data_size ) ;
opnd_t opnd_create_far_abs_addr ( reg_id_t seg , void * addr , opnd_size_t data_size ) ;
opnd_t opnd_create_rel_addr ( void * addr , opnd_size_t data_size ) ;
opnd_t opnd_create_far_rel_addr ( reg_id_t seg , void * addr , opnd_size_t data_size ) ;
void * opnd_get_addr ( opnd_t opnd ) ;
bool opnd_is_memory_reference ( opnd_t opnd ) ;
bool opnd_is_far_memory_reference ( opnd_t opnd ) ;
bool opnd_is_near_memory_reference ( opnd_t opnd ) ;
int opnd_num_regs_used ( opnd_t opnd ) ;
reg_id_t opnd_get_reg_used ( opnd_t opnd , int index ) ;
bool opnd_uses_reg ( opnd_t opnd , reg_id_t reg ) ;
bool opnd_replace_reg ( opnd_t * opnd , reg_id_t old_reg , reg_id_t new_reg ) ;
bool opnd_same_address ( opnd_t op1 , opnd_t op2 ) ;
bool opnd_same ( opnd_t op1 , opnd_t op2 ) ;
bool opnd_share_reg ( opnd_t op1 , opnd_t op2 ) ;
bool opnd_defines_use ( opnd_t def , opnd_t use ) ;
uint opnd_size_in_bytes ( opnd_size_t size ) ;
 opnd_size_t opnd_size_from_bytes ( uint bytes ) ;
opnd_t opnd_shrink_to_16_bits ( opnd_t opnd ) ;
opnd_t opnd_shrink_to_32_bits ( opnd_t opnd ) ;
reg_t reg_get_value_priv ( reg_id_t reg , priv_mcontext_t * mc ) ;
 reg_t reg_get_value ( reg_id_t reg , dr_mcontext_t * mc ) ;
void reg_set_value_priv ( reg_id_t reg , priv_mcontext_t * mc , reg_t value ) ;
 void reg_set_value ( reg_id_t reg , dr_mcontext_t * mc , reg_t value ) ;
app_pc opnd_compute_address_priv ( opnd_t opnd , priv_mcontext_t * mc ) ;
 app_pc opnd_compute_address ( opnd_t opnd , dr_mcontext_t * mc ) ;
const char * get_register_name ( reg_id_t reg ) ;
reg_id_t reg_to_pointer_sized ( reg_id_t reg ) ;
reg_id_t reg_32_to_16 ( reg_id_t reg ) ;
reg_id_t reg_32_to_8 ( reg_id_t reg ) ;
reg_id_t reg_32_to_64 ( reg_id_t reg ) ;
reg_id_t reg_64_to_32 ( reg_id_t reg ) ;
bool reg_is_extended ( reg_id_t reg ) ;
reg_id_t reg_32_to_opsz ( reg_id_t reg , opnd_size_t sz ) ;
reg_id_t reg_resize_to_opsz ( reg_id_t reg , opnd_size_t sz ) ;
int reg_parameter_num ( reg_id_t reg ) ;
int opnd_get_reg_dcontext_offs ( reg_id_t reg ) ;
int opnd_get_reg_mcontext_offs ( reg_id_t reg ) ;
bool reg_overlap ( reg_id_t r1 , reg_id_t r2 ) ;
enum { REG_INVALID_BITS = 0x0 } ;
byte reg_get_bits ( reg_id_t reg ) ;
opnd_size_t reg_get_size ( reg_id_t reg ) ;
instr_t * instr_create ( dcontext_t * dcontext ) ;
void instr_destroy ( dcontext_t * dcontext , instr_t * instr ) ;
instr_t * instr_clone ( dcontext_t * dcontext , instr_t * orig ) ;
void instr_init ( dcontext_t * dcontext , instr_t * instr ) ;
void instr_free ( dcontext_t * dcontext , instr_t * instr ) ;
int instr_mem_usage ( instr_t * instr ) ;
void instr_reset ( dcontext_t * dcontext , instr_t * instr ) ;
void instr_reuse ( dcontext_t * dcontext , instr_t * instr ) ;
instr_t * instr_build ( dcontext_t * dcontext , int opcode , int instr_num_dsts , int instr_num_srcs ) ;
instr_t * instr_build_bits ( dcontext_t * dcontext , int opcode , uint num_bytes ) ;
int instr_get_opcode ( instr_t * instr ) ;
void instr_set_opcode ( instr_t * instr , int opcode ) ;
bool instr_valid ( instr_t * instr ) ;
 app_pc instr_get_app_pc ( instr_t * instr ) ;
bool instr_opcode_valid ( instr_t * instr ) ;
const instr_info_t * instr_get_instr_info ( instr_t * instr ) ;
const instr_info_t * get_instr_info ( int opcode ) ;
opnd_t instr_get_src ( instr_t * instr , uint pos ) ;
opnd_t instr_get_dst ( instr_t * instr , uint pos ) ;
void instr_set_num_opnds ( dcontext_t * dcontext , instr_t * instr , int instr_num_dsts , int instr_num_srcs ) ;
void instr_set_src ( instr_t * instr , uint pos , opnd_t opnd ) ;
void instr_set_dst ( instr_t * instr , uint pos , opnd_t opnd ) ;
opnd_t instr_get_target ( instr_t * instr ) ;
void instr_set_target ( instr_t * instr , opnd_t target ) ;
instr_t * instr_set_prefix_flag ( instr_t * instr , uint prefix ) ;
bool instr_get_prefix_flag ( instr_t * instr , uint prefix ) ;
void instr_set_prefixes ( instr_t * instr , uint prefixes ) ;
uint instr_get_prefixes ( instr_t * instr ) ;
void instr_set_x86_mode ( instr_t * instr , bool x86 ) ;
bool instr_get_x86_mode ( instr_t * instr ) ;
bool instr_branch_special_exit ( instr_t * instr ) ;
void instr_branch_set_special_exit ( instr_t * instr , bool val ) ;
int instr_exit_branch_type ( instr_t * instr ) ;
void instr_exit_branch_set_type ( instr_t * instr , uint type ) ;
void instr_set_ok_to_mangle ( instr_t * instr , bool val ) ;
bool instr_is_meta_may_fault ( instr_t * instr ) ;
void instr_set_meta_may_fault ( instr_t * instr , bool val ) ;
void instr_set_meta_no_translation ( instr_t * instr ) ;
void instr_set_ok_to_emit ( instr_t * instr , bool val ) ;
uint instr_get_eflags ( instr_t * instr ) ;
 uint instr_get_opcode_eflags ( int opcode ) ;
uint instr_get_arith_flags ( instr_t * instr ) ;
bool instr_eflags_valid ( instr_t * instr ) ;
void instr_set_eflags_valid ( instr_t * instr , bool valid ) ;
bool instr_arith_flags_valid ( instr_t * instr ) ;
void instr_set_arith_flags_valid ( instr_t * instr , bool valid ) ;
void instr_set_operands_valid ( instr_t * instr , bool valid ) ;
void instr_set_raw_bits ( instr_t * instr , byte * addr , uint length ) ;
void instr_shift_raw_bits ( instr_t * instr , ssize_t offs ) ;
void instr_set_raw_bits_valid ( instr_t * instr , bool valid ) ;
void instr_free_raw_bits ( dcontext_t * dcontext , instr_t * instr ) ;
void instr_allocate_raw_bits ( dcontext_t * dcontext , instr_t * instr , uint num_bytes ) ;
instr_t * instr_set_translation ( instr_t * instr , app_pc addr ) ;
app_pc instr_get_translation ( instr_t * instr ) ;
void instr_make_persistent ( dcontext_t * dcontext , instr_t * instr ) ;
byte * instr_get_raw_bits ( instr_t * instr ) ;
byte instr_get_raw_byte ( instr_t * instr , uint pos ) ;
uint instr_get_raw_word ( instr_t * instr , uint pos ) ;
void instr_set_raw_byte ( instr_t * instr , uint pos , byte val ) ;
void instr_set_raw_bytes ( instr_t * instr , byte * start , uint num_bytes ) ;
void instr_set_raw_word ( instr_t * instr , uint pos , uint word ) ;
int instr_length ( dcontext_t * dcontext , instr_t * instr ) ;
instr_t * instr_expand ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr ) ;
bool instr_is_level_0 ( instr_t * instr ) ;
instr_t * instr_get_next_expanded ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr ) ;
instr_t * instr_get_prev_expanded ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr ) ;
instr_t * instrlist_first_expanded ( dcontext_t * dcontext , instrlist_t * ilist ) ;
instr_t * instrlist_last_expanded ( dcontext_t * dcontext , instrlist_t * ilist ) ;
void instr_decode_cti ( dcontext_t * dcontext , instr_t * instr ) ;
void instr_decode_opcode ( dcontext_t * dcontext , instr_t * instr ) ;
void instr_decode ( dcontext_t * dcontext , instr_t * instr ) ;
__attribute__ ( ( noinline ) ) instr_t * instr_decode_with_current_dcontext ( instr_t * instr ) ;
void instrlist_decode_cti ( dcontext_t * dcontext , instrlist_t * ilist ) ;
void loginst ( dcontext_t * dcontext , uint level , instr_t * instr , const char * string ) ;
void logopnd ( dcontext_t * dcontext , uint level , opnd_t opnd , const char * string ) ;
void logtrace ( dcontext_t * dcontext , uint level , instrlist_t * trace , const char * string ) ;
void instr_shrink_to_16_bits ( instr_t * instr ) ;
void instr_shrink_to_32_bits ( instr_t * instr ) ;
bool instr_uses_reg ( instr_t * instr , reg_id_t reg ) ;
bool instr_reg_in_dst ( instr_t * instr , reg_id_t reg ) ;
bool instr_reg_in_src ( instr_t * instr , reg_id_t reg ) ;
bool instr_reads_from_reg ( instr_t * instr , reg_id_t reg ) ;
bool instr_writes_to_reg ( instr_t * instr , reg_id_t reg ) ;
bool instr_writes_to_exact_reg ( instr_t * instr , reg_id_t reg ) ;
bool instr_replace_src_opnd ( instr_t * instr , opnd_t old_opnd , opnd_t new_opnd ) ;
bool instr_same ( instr_t * inst1 , instr_t * inst2 ) ;
bool instr_reads_memory ( instr_t * instr ) ;
bool instr_writes_memory ( instr_t * instr ) ;
bool instr_rip_rel_valid ( instr_t * instr ) ;
void instr_set_rip_rel_valid ( instr_t * instr , bool valid ) ;
uint instr_get_rip_rel_pos ( instr_t * instr ) ;
void instr_set_rip_rel_pos ( instr_t * instr , uint pos ) ;
bool instr_get_rel_addr_target ( instr_t * instr , app_pc * target ) ;
bool instr_has_rel_addr_reference ( instr_t * instr ) ;
int instr_get_rel_addr_dst_idx ( instr_t * instr ) ;
int instr_get_rel_addr_src_idx ( instr_t * instr ) ;
bool instr_is_our_mangling ( instr_t * instr ) ;
void instr_set_our_mangling ( instr_t * instr , bool ours ) ;
bool instr_compute_address_ex_priv ( instr_t * instr , priv_mcontext_t * mc , uint index , app_pc * addr , bool * is_write , uint * pos ) ;
 bool instr_compute_address_ex ( instr_t * instr , dr_mcontext_t * mc , uint index , app_pc * addr , bool * is_write ) ;
 bool instr_compute_address_ex_pos ( instr_t * instr , dr_mcontext_t * mc , uint index , app_pc * addr , bool * is_write , uint * pos ) ;
app_pc instr_compute_address_priv ( instr_t * instr , priv_mcontext_t * mc ) ;
 app_pc instr_compute_address ( instr_t * instr , dr_mcontext_t * mc ) ;
uint instr_memory_reference_size ( instr_t * instr ) ;
app_pc decode_memory_reference_size ( dcontext_t * dcontext , app_pc pc , uint * size_in_bytes ) ;
 dr_instr_label_data_t * instr_get_label_data_area ( instr_t * instr ) ;
uint instr_branch_type ( instr_t * cti_instr ) ;
 app_pc instr_get_branch_target_pc ( instr_t * cti_instr ) ;
 void instr_set_branch_target_pc ( instr_t * cti_instr , app_pc pc ) ;
bool instr_is_exit_cti ( instr_t * instr ) ;
bool instr_is_mov ( instr_t * instr ) ;
bool instr_is_call ( instr_t * instr ) ;
bool instr_is_jmp ( instr_t * instr ) ;
bool instr_is_call_direct ( instr_t * instr ) ;
bool instr_is_near_call_direct ( instr_t * instr ) ;
bool instr_is_call_indirect ( instr_t * instr ) ;
bool instr_is_return ( instr_t * instr ) ;
bool instr_is_cbr ( instr_t * instr ) ;
bool instr_is_mbr ( instr_t * instr ) ;
bool instr_is_far_cti ( instr_t * instr ) ;
bool instr_is_far_abs_cti ( instr_t * instr ) ;
bool instr_is_ubr ( instr_t * instr ) ;
bool instr_is_near_ubr ( instr_t * instr ) ;
bool instr_is_cti ( instr_t * instr ) ;
bool instr_is_cti_short ( instr_t * instr ) ;
bool instr_is_cti_loop ( instr_t * instr ) ;
bool instr_is_cti_short_rewrite ( instr_t * instr , byte * pc ) ;
bool instr_is_interrupt ( instr_t * instr ) ;
int instr_get_interrupt_number ( instr_t * instr ) ;
bool instr_is_syscall ( instr_t * instr ) ;
bool instr_is_mov_constant ( instr_t * instr , ptr_int_t * value ) ;
bool instr_is_prefetch ( instr_t * instr ) ;
bool instr_is_floating_ex ( instr_t * instr , dr_fp_type_t * type ) ;
bool instr_is_floating ( instr_t * instr ) ;
bool instr_saves_float_pc ( instr_t * instr ) ;
bool opcode_is_mmx ( int op ) ;
bool opcode_is_sse_or_sse2 ( int op ) ;
bool type_is_sse ( int type ) ;
bool instr_is_mmx ( instr_t * instr ) ;
bool instr_is_sse_or_sse2 ( instr_t * instr ) ;
bool instr_is_mov_imm_to_tos ( instr_t * instr ) ;
bool instr_is_label ( instr_t * instr ) ;
bool instr_is_undefined ( instr_t * instr ) ;
 void instr_invert_cbr ( instr_t * instr ) ;
 instr_t * instr_convert_short_meta_jmp_to_long ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr ) ;
bool instr_cbr_taken ( instr_t * instr , priv_mcontext_t * mcontext , bool pre ) ;
bool instr_jcc_taken ( instr_t * instr , reg_t eflags ) ;
 int instr_cmovcc_to_jcc ( int cmovcc_opcode ) ;
 bool instr_cmovcc_triggered ( instr_t * instr , reg_t eflags ) ;
bool instr_uses_fp_reg ( instr_t * instr ) ;
bool reg_is_gpr ( reg_id_t reg ) ;
bool reg_is_segment ( reg_id_t reg ) ;
bool reg_is_ymm ( reg_id_t reg ) ;
bool reg_is_xmm ( reg_id_t reg ) ;
bool reg_is_mmx ( reg_id_t reg ) ;
bool reg_is_fp ( reg_id_t reg ) ;
instr_t * instr_create_0dst_0src ( dcontext_t * dcontext , int opcode ) ;
instr_t * instr_create_0dst_1src ( dcontext_t * dcontext , int opcode , opnd_t src ) ;
instr_t * instr_create_0dst_2src ( dcontext_t * dcontext , int opcode , opnd_t src1 , opnd_t src2 ) ;
instr_t * instr_create_0dst_3src ( dcontext_t * dcontext , int opcode , opnd_t src1 , opnd_t src2 , opnd_t src3 ) ;
instr_t * instr_create_1dst_0src ( dcontext_t * dcontext , int opcode , opnd_t dst ) ;
instr_t * instr_create_1dst_1src ( dcontext_t * dcontext , int opcode , opnd_t dst , opnd_t src ) ;
instr_t * instr_create_1dst_2src ( dcontext_t * dcontext , int opcode , opnd_t dst , opnd_t src1 , opnd_t src2 ) ;
instr_t * instr_create_1dst_3src ( dcontext_t * dcontext , int opcode , opnd_t dst , opnd_t src1 , opnd_t src2 , opnd_t src3 ) ;
instr_t * instr_create_1dst_5src ( dcontext_t * dcontext , int opcode , opnd_t dst , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 , opnd_t src5 ) ;
instr_t * instr_create_2dst_0src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 ) ;
instr_t * instr_create_2dst_1src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t src ) ;
instr_t * instr_create_2dst_2src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t src1 , opnd_t src2 ) ;
instr_t * instr_create_2dst_3src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t src1 , opnd_t src2 , opnd_t src3 ) ;
instr_t * instr_create_2dst_4src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 ) ;
instr_t * instr_create_3dst_0src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 ) ;
instr_t * instr_create_3dst_3src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t src1 , opnd_t src2 , opnd_t src3 ) ;
instr_t * instr_create_3dst_4src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 ) ;
instr_t * instr_create_3dst_5src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 , opnd_t src5 ) ;
instr_t * instr_create_4dst_1src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t dst4 , opnd_t src ) ;
instr_t * instr_create_4dst_4src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t dst4 , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 ) ;
instr_t * instr_create_popa ( dcontext_t * dcontext ) ;
instr_t * instr_create_pusha ( dcontext_t * dcontext ) ;
instr_t * instr_create_raw_1byte ( dcontext_t * dcontext , byte byte1 ) ;
instr_t * instr_create_raw_2bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 ) ;
instr_t * instr_create_raw_3bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 ) ;
instr_t * instr_create_raw_4bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 ) ;
instr_t * instr_create_raw_5bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 , byte byte5 ) ;
instr_t * instr_create_raw_6bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 , byte byte5 , byte byte6 ) ;
instr_t * instr_create_raw_7bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 , byte byte5 , byte byte6 , byte byte7 ) ;
instr_t * instr_create_raw_8bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 , byte byte5 , byte byte6 , byte byte7 , byte byte8 ) ;
instr_t * instr_create_nbyte_nop ( dcontext_t * dcontext , uint num_bytes , bool raw ) ;
bool instr_is_nop ( instr_t * inst ) ;
struct _module_handle_t ;
typedef struct _module_handle_t * module_handle_t ;
typedef unsigned char uint8_t ;
typedef unsigned short int uint16_t ;
typedef unsigned int uint32_t ;
typedef unsigned long int uint64_t ;
typedef signed char int_least8_t ;
typedef short int int_least16_t ;
typedef int int_least32_t ;
typedef long int int_least64_t ;
typedef unsigned char uint_least8_t ;
typedef unsigned short int uint_least16_t ;
typedef unsigned int uint_least32_t ;
typedef unsigned long int uint_least64_t ;
typedef signed char int_fast8_t ;
typedef long int int_fast16_t ;
typedef long int int_fast32_t ;
typedef long int int_fast64_t ;
typedef unsigned char uint_fast8_t ;
typedef unsigned long int uint_fast16_t ;
typedef unsigned long int uint_fast32_t ;
typedef unsigned long int uint_fast64_t ;
typedef long int intptr_t ;
typedef unsigned long int uintptr_t ;
typedef long int intmax_t ;
typedef unsigned long int uintmax_t ;
typedef uint16_t Elf32_Half ;
typedef uint16_t Elf64_Half ;
typedef uint32_t Elf32_Word ;
typedef int32_t Elf32_Sword ;
typedef uint32_t Elf64_Word ;
typedef int32_t Elf64_Sword ;
typedef uint64_t Elf32_Xword ;
typedef int64_t Elf32_Sxword ;
typedef uint64_t Elf64_Xword ;
typedef int64_t Elf64_Sxword ;
typedef uint32_t Elf32_Addr ;
typedef uint64_t Elf64_Addr ;
typedef uint32_t Elf32_Off ;
typedef uint64_t Elf64_Off ;
typedef uint16_t Elf32_Section ;
typedef uint16_t Elf64_Section ;
typedef Elf32_Half Elf32_Versym ;
typedef Elf64_Half Elf64_Versym ;
typedef struct { unsigned char e_ident [ ( 16 ) ] ; Elf32_Half e_type ; Elf32_Half e_machine ; Elf32_Word e_version ; Elf32_Addr e_entry ; Elf32_Off e_phoff ; Elf32_Off e_shoff ; Elf32_Word e_flags ; Elf32_Half e_ehsize ; Elf32_Half e_phentsize ; Elf32_Half e_phnum ; Elf32_Half e_shentsize ; Elf32_Half e_shnum ; Elf32_Half e_shstrndx ; } Elf32_Ehdr ;
typedef struct { unsigned char e_ident [ ( 16 ) ] ; Elf64_Half e_type ; Elf64_Half e_machine ; Elf64_Word e_version ; Elf64_Addr e_entry ; Elf64_Off e_phoff ; Elf64_Off e_shoff ; Elf64_Word e_flags ; Elf64_Half e_ehsize ; Elf64_Half e_phentsize ; Elf64_Half e_phnum ; Elf64_Half e_shentsize ; Elf64_Half e_shnum ; Elf64_Half e_shstrndx ; } Elf64_Ehdr ;
typedef struct { Elf32_Word sh_name ; Elf32_Word sh_type ; Elf32_Word sh_flags ; Elf32_Addr sh_addr ; Elf32_Off sh_offset ; Elf32_Word sh_size ; Elf32_Word sh_link ; Elf32_Word sh_info ; Elf32_Word sh_addralign ; Elf32_Word sh_entsize ; } Elf32_Shdr ;
typedef struct { Elf64_Word sh_name ; Elf64_Word sh_type ; Elf64_Xword sh_flags ; Elf64_Addr sh_addr ; Elf64_Off sh_offset ; Elf64_Xword sh_size ; Elf64_Word sh_link ; Elf64_Word sh_info ; Elf64_Xword sh_addralign ; Elf64_Xword sh_entsize ; } Elf64_Shdr ;
typedef struct { Elf32_Word st_name ; Elf32_Addr st_value ; Elf32_Word st_size ; unsigned char st_info ; unsigned char st_other ; Elf32_Section st_shndx ; } Elf32_Sym ;
typedef struct { Elf64_Word st_name ; unsigned char st_info ; unsigned char st_other ; Elf64_Section st_shndx ; Elf64_Addr st_value ; Elf64_Xword st_size ; } Elf64_Sym ;
typedef struct { Elf32_Half si_boundto ; Elf32_Half si_flags ; } Elf32_Syminfo ;
typedef struct { Elf64_Half si_boundto ; Elf64_Half si_flags ; } Elf64_Syminfo ;
typedef struct { Elf32_Addr r_offset ; Elf32_Word r_info ; } Elf32_Rel ;
typedef struct { Elf64_Addr r_offset ; Elf64_Xword r_info ; } Elf64_Rel ;
typedef struct { Elf32_Addr r_offset ; Elf32_Word r_info ; Elf32_Sword r_addend ; } Elf32_Rela ;
typedef struct { Elf64_Addr r_offset ; Elf64_Xword r_info ; Elf64_Sxword r_addend ; } Elf64_Rela ;
typedef struct { Elf32_Word p_type ; Elf32_Off p_offset ; Elf32_Addr p_vaddr ; Elf32_Addr p_paddr ; Elf32_Word p_filesz ; Elf32_Word p_memsz ; Elf32_Word p_flags ; Elf32_Word p_align ; } Elf32_Phdr ;
typedef struct { Elf64_Word p_type ; Elf64_Word p_flags ; Elf64_Off p_offset ; Elf64_Addr p_vaddr ; Elf64_Addr p_paddr ; Elf64_Xword p_filesz ; Elf64_Xword p_memsz ; Elf64_Xword p_align ; } Elf64_Phdr ;
typedef struct { Elf32_Sword d_tag ; union { Elf32_Word d_val ; Elf32_Addr d_ptr ; } d_un ; } Elf32_Dyn ;
typedef struct { Elf64_Sxword d_tag ; union { Elf64_Xword d_val ; Elf64_Addr d_ptr ; } d_un ; } Elf64_Dyn ;
typedef struct { Elf32_Half vd_version ; Elf32_Half vd_flags ; Elf32_Half vd_ndx ; Elf32_Half vd_cnt ; Elf32_Word vd_hash ; Elf32_Word vd_aux ; Elf32_Word vd_next ; } Elf32_Verdef ;
typedef struct { Elf64_Half vd_version ; Elf64_Half vd_flags ; Elf64_Half vd_ndx ; Elf64_Half vd_cnt ; Elf64_Word vd_hash ; Elf64_Word vd_aux ; Elf64_Word vd_next ; } Elf64_Verdef ;
typedef struct { Elf32_Word vda_name ; Elf32_Word vda_next ; } Elf32_Verdaux ;
typedef struct { Elf64_Word vda_name ; Elf64_Word vda_next ; } Elf64_Verdaux ;
typedef struct { Elf32_Half vn_version ; Elf32_Half vn_cnt ; Elf32_Word vn_file ; Elf32_Word vn_aux ; Elf32_Word vn_next ; } Elf32_Verneed ;
typedef struct { Elf64_Half vn_version ; Elf64_Half vn_cnt ; Elf64_Word vn_file ; Elf64_Word vn_aux ; Elf64_Word vn_next ; } Elf64_Verneed ;
typedef struct { Elf32_Word vna_hash ; Elf32_Half vna_flags ; Elf32_Half vna_other ; Elf32_Word vna_name ; Elf32_Word vna_next ; } Elf32_Vernaux ;
typedef struct { Elf64_Word vna_hash ; Elf64_Half vna_flags ; Elf64_Half vna_other ; Elf64_Word vna_name ; Elf64_Word vna_next ; } Elf64_Vernaux ;
typedef struct { uint32_t a_type ; union { uint32_t a_val ; } a_un ; } Elf32_auxv_t ;
typedef struct { uint64_t a_type ; union { uint64_t a_val ; } a_un ; } Elf64_auxv_t ;
typedef struct { Elf32_Word n_namesz ; Elf32_Word n_descsz ; Elf32_Word n_type ; } Elf32_Nhdr ;
typedef struct { Elf64_Word n_namesz ; Elf64_Word n_descsz ; Elf64_Word n_type ; } Elf64_Nhdr ;
typedef struct { Elf32_Xword m_value ; Elf32_Word m_info ; Elf32_Word m_poffset ; Elf32_Half m_repeat ; Elf32_Half m_stride ; } Elf32_Move ;
typedef struct { Elf64_Xword m_value ; Elf64_Xword m_info ; Elf64_Xword m_poffset ; Elf64_Half m_repeat ; Elf64_Half m_stride ; } Elf64_Move ;
typedef union { struct { Elf32_Word gt_current_g_value ; Elf32_Word gt_unused ; } gt_header ; struct { Elf32_Word gt_g_value ; Elf32_Word gt_bytes ; } gt_entry ; } Elf32_gptab ;
typedef struct { Elf32_Word ri_gprmask ; Elf32_Word ri_cprmask [ 4 ] ; Elf32_Sword ri_gp_value ; } Elf32_RegInfo ;
typedef struct { unsigned char kind ; unsigned char size ; Elf32_Section section ; Elf32_Word info ; } Elf_Options ;
typedef struct { Elf32_Word hwp_flags1 ; Elf32_Word hwp_flags2 ; } Elf_Options_Hw ;
typedef struct { Elf32_Word l_name ; Elf32_Word l_time_stamp ; Elf32_Word l_checksum ; Elf32_Word l_version ; Elf32_Word l_flags ; } Elf32_Lib ;
typedef struct { Elf64_Word l_name ; Elf64_Word l_time_stamp ; Elf64_Word l_checksum ; Elf64_Word l_version ; Elf64_Word l_flags ; } Elf64_Lib ;
typedef Elf32_Addr Elf32_Conflict ;
typedef struct _module_segment_t { app_pc start ; app_pc end ; uint prot ; } module_segment_t ;
typedef struct _os_module_data_t { app_pc base_address ; size_t alignment ; size_t checksum ; size_t timestamp ; bool hash_is_gnu ; app_pc hashtab ; size_t num_buckets ; app_pc buckets ; size_t num_chain ; app_pc chain ; app_pc dynsym ; app_pc dynstr ; size_t dynstr_size ; size_t symentry_size ; app_pc gnu_bitmask ; ptr_uint_t gnu_shift ; ptr_uint_t gnu_bitidx ; size_t gnu_symbias ; bool contiguous ; uint num_segments ; uint alloc_segments ; module_segment_t * segments ; } os_module_data_t ;
typedef void ( * fp_t ) ( int argc , char * * argv , char * * env ) ;
typedef struct _os_privmod_data_t { os_module_data_t os_data ; Elf64_Dyn * dyn ; ptr_int_t load_delta ; char * soname ; Elf64_Addr pltgot ; size_t pltrelsz ; Elf64_Xword pltrel ; bool textrel ; app_pc jmprel ; Elf64_Rel * rel ; size_t relsz ; size_t relent ; Elf64_Rela * rela ; size_t relasz ; size_t relaent ; app_pc verneed ; int verneednum ; int relcount ; Elf64_Half * versym ; fp_t init ; fp_t fini ; fp_t * init_array ; fp_t * fini_array ; size_t init_arraysz ; size_t fini_arraysz ; uint tls_block_size ; uint tls_align ; uint tls_modid ; uint tls_offset ; uint tls_image_size ; uint tls_first_byte ; app_pc tls_image ; } os_privmod_data_t ;
typedef struct elf_loader_t { const char * filename ; file_t fd ; Elf64_Ehdr * ehdr ; Elf64_Phdr * phdrs ; app_pc load_base ; ptr_int_t load_delta ; size_t image_size ; void * file_map ; size_t file_size ; byte buf [ sizeof ( Elf64_Ehdr ) + sizeof ( Elf64_Phdr ) * 12 ] ; } elf_loader_t ;
typedef byte * ( * map_fn_t ) ( file_t f , size_t * size , uint64 offs , app_pc addr , uint prot , map_flags_t map_flags ) ;
typedef bool ( * unmap_fn_t ) ( byte * map , size_t size ) ;
typedef bool ( * prot_fn_t ) ( byte * map , size_t size , uint prot ) ;
typedef struct _module_names_t { const char * module_name ; const char * file_name ; uint64 inode ; } module_names_t ;
typedef struct _module_area_t { app_pc start ; app_pc end ; app_pc entry_point ; uint flags ; module_names_t names ; char * full_path ; os_module_data_t os_data ; } module_area_t ;
enum { MODULE_HAS_PRIMARY_COARSE = 0x00000001 , MODULE_BEING_UNLOADED = 0x00000008 , MODULE_WAS_EXEMPTED = 0x00000010 , MODULE_NULL_INSTRUMENT = 0x00000080 , } ;
typedef struct _module_iterator_t module_iterator_t ;
typedef void * module_base_t ;
typedef struct { byte full_MD5 [ 16 ] ; byte short_MD5 [ 16 ] ; } module_digest_t ;
typedef struct _privmod_t { app_pc base ; size_t size ; const char * name ; char path [ 260 ] ; uint ref_count ; bool externally_loaded ; bool is_client ; struct _privmod_t * next ; struct _privmod_t * prev ; void * os_privmod_data ; } privmod_t ;
typedef struct _generic_entry_t { ptr_uint_t key ; void * payload ; } generic_entry_t ;
typedef struct _generic_table_t { ptr_uint_t hash_mask ; generic_entry_t * * table ; uint ref_count ; uint hash_bits ; hash_function_t hash_func ; uint hash_mask_offset ; uint capacity ; uint entries ; uint unlinked_entries ; uint load_factor_percent ; uint resize_threshold ; uint groom_factor_percent ; uint groom_threshold ; uint max_capacity_bits ; uint table_flags ; read_write_lock_t rwlock ; generic_entry_t * * table_unaligned ; void ( * free_payload_func ) ( void * ) ; } generic_table_t ;
typedef struct _strhash_entry_t { const char * key ; void * payload ; } strhash_entry_t ;
typedef struct _strhash_table_t { ptr_uint_t hash_mask ; strhash_entry_t * * table ; uint ref_count ; uint hash_bits ; hash_function_t hash_func ; uint hash_mask_offset ; uint capacity ; uint entries ; uint unlinked_entries ; uint load_factor_percent ; uint resize_threshold ; uint groom_factor_percent ; uint groom_threshold ; uint max_capacity_bits ; uint table_flags ; read_write_lock_t rwlock ; strhash_entry_t * * table_unaligned ; void ( * free_payload_func ) ( void * ) ; } strhash_table_t ;
enum { MAX_FRAGMENT_SIZE = ( 32767 * 2 + 1 ) } ;
struct _fragment_t { app_pc tag ; uint flags ; ushort size ; byte prefix_size ; byte fcache_extra ; cache_pc start_pc ; union { linkstub_t * incoming_stubs ; translation_info_t * translation_info ; } in_xlate ; fragment_t * next_vmarea ; fragment_t * prev_vmarea ; union { fragment_t * also_vmarea ; uint flushtime ; } also ; } ;
typedef struct _private_fragment_t { fragment_t f ; fragment_t * next_fcache ; fragment_t * prev_fcache ; } private_fragment_t ;
struct _future_fragment_t { app_pc tag ; uint flags ; linkstub_t * incoming_stubs ; } ;
typedef struct _trace_bb_info_t { app_pc tag ; } trace_bb_info_t ;
typedef struct _trace_only_t { trace_bb_info_t * bbs ; uint num_bbs ; } trace_only_t ;
struct _trace_t { fragment_t f ; trace_only_t t ; } ;
typedef struct _private_trace_t { private_fragment_t f ; trace_only_t t ; } private_trace_t ;
typedef struct _fragment_entry_t { app_pc tag_fragment ; cache_pc start_pc_fragment ; } fragment_entry_t ;
typedef struct _fragment_table_t { ptr_uint_t hash_mask ; fragment_t * * table ; uint ref_count ; uint hash_bits ; hash_function_t hash_func ; uint hash_mask_offset ; uint capacity ; uint entries ; uint unlinked_entries ; uint load_factor_percent ; uint resize_threshold ; uint groom_factor_percent ; uint groom_threshold ; uint max_capacity_bits ; uint table_flags ; read_write_lock_t rwlock ; fragment_t * * table_unaligned ; } fragment_table_t ;
typedef struct _ibl_table_t { ptr_uint_t hash_mask ; fragment_entry_t * table ; uint ref_count ; uint hash_bits ; hash_function_t hash_func ; uint hash_mask_offset ; uint capacity ; uint entries ; uint unlinked_entries ; uint load_factor_percent ; uint resize_threshold ; uint groom_factor_percent ; uint groom_threshold ; uint max_capacity_bits ; uint table_flags ; read_write_lock_t rwlock ; fragment_entry_t * table_unaligned ; ibl_branch_type_t branch_type ; } ibl_table_t ;
typedef struct _per_thread_t { ibl_table_t trace_ibt [ IBL_BRANCH_TYPE_END ] ; ibl_table_t bb_ibt [ IBL_BRANCH_TYPE_END ] ; fragment_table_t bb ; fragment_table_t trace ; fragment_table_t future ; mutex_t fragment_delete_mutex ; file_t tracefile ; bool could_be_linking ; bool wait_for_unlink ; bool about_to_exit ; bool flush_queue_nonempty ; event_t waiting_for_unlink ; event_t finished_with_unlink ; event_t finished_all_unlink ; mutex_t linking_lock ; bool soon_to_be_linking ; uint flushtime_last_update ; bool at_syscall_at_flush ; } per_thread_t ;
enum { FRAGDEL_ALL = 0x000 , FRAGDEL_NO_OUTPUT = 0x001 , FRAGDEL_NO_UNLINK = 0x002 , FRAGDEL_NO_HTABLE = 0x004 , FRAGDEL_NO_FCACHE = 0x008 , FRAGDEL_NO_HEAP = 0x010 , FRAGDEL_NO_MONITOR = 0x020 , FRAGDEL_NO_VMAREA = 0x040 , FRAGDEL_NEED_CHLINK_LOCK = 0x080 , } ;
typedef struct _app_to_cache_t { app_pc app ; cache_pc cache ; } app_to_cache_t ;
typedef struct _coarse_table_t { ptr_uint_t hash_mask ; app_to_cache_t * table ; uint ref_count ; uint hash_bits ; hash_function_t hash_func ; uint hash_mask_offset ; uint capacity ; uint entries ; uint unlinked_entries ; uint load_factor_percent ; uint resize_threshold ; uint groom_factor_percent ; uint groom_threshold ; uint max_capacity_bits ; uint table_flags ; read_write_lock_t rwlock ; app_to_cache_t * table_unaligned ; ssize_t mod_shift ; } coarse_table_t ;
typedef struct _tracedump_file_header_t { int version ; bool x64 ; int linkcount_size ; } tracedump_file_header_t ;
typedef struct _tracedump_trace_header_t { int frag_id ; app_pc tag ; app_pc cache_start_pc ; int entry_offs ; int num_exits ; int code_size ; uint num_bbs ; bool x64 ; } tracedump_trace_header_t ;
typedef struct _tracedump_stub_data { int cti_offs ; app_pc stub_pc ; app_pc target ; bool linked ; int stub_size ; union { uint count32 ; uint64 count64 ; } count ; byte stub_code [ 1 ] ; } tracedump_stub_data_t ;
enum { RESET_ALL = 0x001 , RESET_BASIC_BLOCKS = 0x002 , RESET_TRACES = 0x004 , RESET_PENDING_DELETION = 0x008 , } ;
 void disassemble_set_syntax ( dr_disasm_flags_t flags ) ;
void opnd_disassemble ( dcontext_t * dcontext , opnd_t opnd , file_t outfile ) ;
size_t opnd_disassemble_to_buffer ( dcontext_t * dcontext , opnd_t opnd , char * buf , size_t bufsz ) ;
byte * disassemble ( dcontext_t * dcontext , byte * pc , file_t outfile ) ;
byte * disassemble_with_bytes ( dcontext_t * dcontext , byte * pc , file_t outfile ) ;
byte * disassemble_with_info ( dcontext_t * dcontext , byte * pc , file_t outfile , bool show_pc , bool show_bytes ) ;
byte * disassemble_from_copy ( dcontext_t * dcontext , byte * copy_pc , byte * orig_pc , file_t outfile , bool show_pc , bool show_bytes ) ;
byte * disassemble_to_buffer ( dcontext_t * dcontext , byte * pc , byte * orig_pc , bool show_pc , bool show_bytes , char * buf , size_t bufsz , int * printed ) ;
void instr_disassemble ( dcontext_t * dcontext , instr_t * instr , file_t outfile ) ;
size_t instr_disassemble_to_buffer ( dcontext_t * dcontext , instr_t * instr , char * buf , size_t bufsz ) ;
void instrlist_disassemble ( dcontext_t * dcontext , app_pc tag , instrlist_t * ilist , file_t outfile ) ;
enum { VARLEN_NONE , VARLEN_MODRM , VARLEN_FP_OP , VARLEN_ESCAPE , VARLEN_3BYTE_38_ESCAPE , VARLEN_3BYTE_3A_ESCAPE , } ;
int decode_sizeof ( dcontext_t * dcontext , byte * start_pc , int * num_prefixes , uint * rip_rel_pos ) ;
byte * decode_cti ( dcontext_t * dcontext , byte * pc , instr_t * instr ) ;
byte * decode_next_pc ( dcontext_t * dcontext , byte * pc ) ;
byte * decode_raw ( dcontext_t * dcontext , byte * pc , instr_t * instr ) ;
typedef enum { DR_EMIT_DEFAULT = 0 , DR_EMIT_STORE_TRANSLATIONS = 0x01 , DR_EMIT_PERSISTABLE = 0x02 , DR_EMIT_MUST_END_TRACE = 0x04 , DR_EMIT_GO_NATIVE = 0x08 , } dr_emit_flags_t ;
typedef enum { CUSTOM_TRACE_DR_DECIDES , CUSTOM_TRACE_END_NOW , CUSTOM_TRACE_CONTINUE } dr_custom_trace_action_t ;
typedef struct _dr_fault_fragment_info_t { void * tag ; byte * cache_start_pc ; bool is_trace ; bool app_code_consistent ; } dr_fault_fragment_info_t ;
typedef struct _dr_restore_state_info_t { dr_mcontext_t * mcontext ; bool raw_mcontext_valid ; dr_mcontext_t * raw_mcontext ; dr_fault_fragment_info_t fragment_info ; } dr_restore_state_info_t ;
typedef enum { DR_EXIT_MULTI_THREAD = 0x01 , DR_EXIT_SKIP_THREAD_EXIT = 0x02 , } dr_exit_flags_t ;
typedef struct _dr_siginfo_t { int sig ; void * drcontext ; dr_mcontext_t * mcontext ; dr_mcontext_t * raw_mcontext ; bool raw_mcontext_valid ; byte * access_address ; bool blocked ; dr_fault_fragment_info_t fault_fragment_info ; } dr_siginfo_t ;
typedef enum { DR_SIGNAL_DELIVER , DR_SIGNAL_SUPPRESS , DR_SIGNAL_BYPASS , DR_SIGNAL_REDIRECT , } dr_signal_action_t ;
typedef enum { DR_ALLOC_NON_HEAP = 0x0001 , DR_ALLOC_THREAD_PRIVATE = 0x0002 , DR_ALLOC_CACHE_REACHABLE = 0x0004 , DR_ALLOC_FIXED_LOCATION = 0x0008 , DR_ALLOC_LOW_2GB = 0x0010 , DR_ALLOC_NON_DR = 0x0020 , } dr_alloc_flags_t ;
typedef void * dr_module_iterator_t ;
typedef struct _module_segment_data_t { app_pc start ; app_pc end ; uint prot ; } module_segment_data_t ;
struct _module_data_t { union { app_pc start ; module_handle_t handle ; } ; app_pc end ; app_pc entry_point ; uint flags ; module_names_t names ; char * full_path ; bool contiguous ; uint num_segments ; module_segment_data_t * segments ; } ;
struct _dr_module_import_iterator_t ;
typedef struct _dr_module_import_iterator_t dr_module_import_iterator_t ;
struct _dr_module_import_desc_t ;
typedef struct _dr_module_import_desc_t dr_module_import_desc_t ;
typedef struct _dr_module_import_t { const char * modname ; dr_module_import_desc_t * module_import_desc ; } dr_module_import_t ;
struct _dr_symbol_import_iterator_t ;
typedef struct _dr_symbol_import_iterator_t dr_symbol_import_iterator_t ;
typedef struct _dr_symbol_import_t { const char * name ; const char * modname ; bool delay_load ; bool by_ordinal ; ptr_uint_t ordinal ; } dr_symbol_import_t ;
typedef struct _dr_export_info_t { generic_func_t address ; bool is_indirect_code ; } dr_export_info_t ;
enum { DR_MAP_PRIVATE = 0x0001 , DR_MAP_FIXED = 0x0002 , DR_MAP_CACHE_REACHABLE = 0x0008 , } ;
typedef enum { DR_SUSPEND_NATIVE = 0x0001 , } dr_suspend_flags_t ;
typedef enum { SPILL_SLOT_1 = 0 , SPILL_SLOT_2 = 1 , SPILL_SLOT_3 = 2 , SPILL_SLOT_4 = 3 , SPILL_SLOT_5 = 4 , SPILL_SLOT_6 = 5 , SPILL_SLOT_7 = 6 , SPILL_SLOT_8 = 7 , SPILL_SLOT_9 = 8 , SPILL_SLOT_10 = 9 , SPILL_SLOT_11 = 10 , SPILL_SLOT_12 = 11 , SPILL_SLOT_13 = 12 , SPILL_SLOT_14 = 13 , SPILL_SLOT_15 = 14 , SPILL_SLOT_16 = 15 , SPILL_SLOT_17 = 16 , SPILL_SLOT_MAX = SPILL_SLOT_17 } dr_spill_slot_t ;
typedef enum { DR_CLEANCALL_SAVE_FLOAT = 0x0001 , DR_CLEANCALL_NOSAVE_FLAGS = 0x0002 , DR_CLEANCALL_NOSAVE_XMM = 0x0004 , DR_CLEANCALL_NOSAVE_XMM_NONPARAM = 0x0008 , DR_CLEANCALL_NOSAVE_XMM_NONRET = 0x0010 , DR_CLEANCALL_INDIRECT = 0x0020 , } dr_cleancall_save_t ;
instr_t * convert_to_near_rel_meta ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr ) ;
void convert_to_near_rel ( dcontext_t * dcontext , instr_t * instr ) ;
byte * remangle_short_rewrite ( dcontext_t * dcontext , instr_t * instr , byte * pc , app_pc target ) ;

byte * instr_raw_is_rip_rel_lea ( byte * pc , byte * read_end ) ;

#define DR_REG_LAST_ENUM   DR_REG_YMM15 /**< Last value of register enums */


void instr_set_note(instr_t *instr, void *value);

void *
instr_get_note(instr_t *instr);

int
instr_num_dsts(instr_t *instr);

int
instr_num_srcs(instr_t *instr);

bool
instr_ok_to_mangle(instr_t *instr);

opnd_t
opnd_create_pc(app_pc pc);

opnd_t
opnd_create_reg(reg_id_t r);

opnd_t
opnd_create_null(void);

bool
opnd_is_far_rel_addr(opnd_t opnd);

bool
opnd_is_near_rel_addr(opnd_t opnd);

bool
opnd_is_far_base_disp(opnd_t op);

bool
opnd_is_near_base_disp(opnd_t op);

bool
opnd_is_instr(opnd_t op);

bool
opnd_is_pc(opnd_t op);

bool
opnd_is_immed(opnd_t op);

/* opnd_t predicates */

/* Simple predicates */
#define OPND_IS_NULL(op)        ((op).kind == NULL_kind)
#define OPND_IS_IMMED_INT(op)   ((op).kind == IMMED_INTEGER_kind)
#define OPND_IS_IMMED_FLOAT(op) ((op).kind == IMMED_FLOAT_kind)
#define OPND_IS_NEAR_PC(op)     ((op).kind == PC_kind)
#define OPND_IS_NEAR_INSTR(op)  ((op).kind == INSTR_kind)
#define OPND_IS_REG(op)         ((op).kind == REG_kind)
#define OPND_IS_BASE_DISP(op)   ((op).kind == BASE_DISP_kind)
#define OPND_IS_FAR_PC(op)      ((op).kind == FAR_PC_kind)
#define OPND_IS_FAR_INSTR(op)   ((op).kind == FAR_INSTR_kind)
#define OPND_IS_MEM_INSTR(op)   ((op).kind == MEM_INSTR_kind)
#define OPND_IS_VALID(op)       ((op).kind < LAST_kind)

#define opnd_is_null            OPND_IS_NULL
#define opnd_is_immed_int       OPND_IS_IMMED_INT
#define opnd_is_immed_float     OPND_IS_IMMED_FLOAT
#define opnd_is_near_pc         OPND_IS_NEAR_PC
#define opnd_is_near_instr      OPND_IS_NEAR_INSTR
#define opnd_is_reg             OPND_IS_REG
#define opnd_is_base_disp       OPND_IS_BASE_DISP
#define opnd_is_far_pc          OPND_IS_FAR_PC
#define opnd_is_far_instr       OPND_IS_FAR_INSTR
#define opnd_is_mem_instr       OPND_IS_MEM_INSTR
#define opnd_is_valid           OPND_IS_VALID



instr_t *convert_to_near_rel_common(dcontext_t *dcontext,
                                    instrlist_t *ilist,
                                    instr_t *instr);


dcontext_t * get_thread_private_dcontext(void);

#ifdef X64
# define OPSZ_PTR OPSZ_8       /**< Operand size for pointer values. */
# define OPSZ_STACK OPSZ_8     /**< Operand size for stack push/pop operand sizes. */
#else
# define OPSZ_PTR OPSZ_4       /**< Operand size for pointer values. */
# define OPSZ_STACK OPSZ_4     /**< Operand size for stack push/pop operand sizes. */
#endif
#define OPSZ_VARSTACK OPSZ_4x8_short2 /**< Operand size for prefix-varying stack
 * push/pop operand sizes. */
#define OPSZ_REXVARSTACK OPSZ_4_rex8_short2 /* Operand size for prefix/rex-varying
 * stack push/pop like operand sizes. */

#define OPSZ_ret OPSZ_4x8_short2xi8 /**< Operand size for ret instruction. */
#define OPSZ_call OPSZ_ret         /**< Operand size for push portion of call. */

/* Convenience defines for specific opcodes */
#define OPSZ_lea OPSZ_0              /**< Operand size for lea memory reference. */
#define OPSZ_invlpg OPSZ_0           /**< Operand size for invlpg memory reference. */
#define OPSZ_xlat OPSZ_1             /**< Operand size for xlat memory reference. */
#define OPSZ_clflush OPSZ_1          /**< Operand size for clflush memory reference. */
#define OPSZ_prefetch OPSZ_1         /**< Operand size for prefetch memory references. */
#define OPSZ_lgdt OPSZ_6x10          /**< Operand size for lgdt memory reference. */
#define OPSZ_sgdt OPSZ_6x10          /**< Operand size for sgdt memory reference. */
#define OPSZ_lidt OPSZ_6x10          /**< Operand size for lidt memory reference. */
#define OPSZ_sidt OPSZ_6x10          /**< Operand size for sidt memory reference. */
#define OPSZ_bound OPSZ_8_short4     /**< Operand size for bound memory reference. */
#define OPSZ_maskmovq OPSZ_8         /**< Operand size for maskmovq memory reference. */
#define OPSZ_maskmovdqu OPSZ_16      /**< Operand size for maskmovdqu memory reference. */
#define OPSZ_fldenv OPSZ_28_short14  /**< Operand size for fldenv memory reference. */
#define OPSZ_fnstenv OPSZ_28_short14 /**< Operand size for fnstenv memory reference. */
#define OPSZ_fnsave OPSZ_108_short94 /**< Operand size for fnsave memory reference. */
#define OPSZ_frstor OPSZ_108_short94 /**< Operand size for frstor memory reference. */
#define OPSZ_fxsave OPSZ_512         /**< Operand size for fxsave memory reference. */
#define OPSZ_fxrstor OPSZ_512        /**< Operand size for fxrstor memory reference. */

#ifndef INT8_MIN
# define INT8_MIN   SCHAR_MIN
# define INT8_MAX   SCHAR_MAX
# define INT16_MIN  SHRT_MIN
# define INT16_MAX  SHRT_MAX
# define INT32_MIN  INT_MIN
# define INT32_MAX  INT_MAX
#endif
/* DR_API EXPORT END */

/* alternative names */
/* we do not equate the fwait+op opcodes
 *   fstsw, fstcw, fstenv, finit, fclex
 * for us that has to be a sequence of instructions: a separate fwait
 */
/* 16-bit versions that have different names */
#define OP_cbw        OP_cwde /**< Alternative opcode name for 16-bit version. */
#define OP_cwd        OP_cdq /**< Alternative opcode name for 16-bit version. */
#define OP_jcxz       OP_jecxz /**< Alternative opcode name for 16-bit version. */
/* 64-bit versions that have different names */
#define OP_jrcxz      OP_jecxz     /**< Alternative opcode name for 64-bit version. */
#define OP_cmpxchg16b OP_cmpxchg8b /**< Alternative opcode name for 64-bit version. */
#define OP_pextrq     OP_pextrd    /**< Alternative opcode name for 64-bit version. */
#define OP_pinsrq     OP_pinsrd    /**< Alternative opcode name for 64-bit version. */
/* reg-reg version has different name */
#define OP_movhlps    OP_movlps /**< Alternative opcode name for reg-reg version. */
#define OP_movlhps    OP_movhps /**< Alternative opcode name for reg-reg version. */
/* condition codes */
#define OP_jae_short  OP_jnb_short  /**< Alternative opcode name. */
#define OP_jnae_short OP_jb_short   /**< Alternative opcode name. */
#define OP_ja_short   OP_jnbe_short /**< Alternative opcode name. */
#define OP_jna_short  OP_jbe_short  /**< Alternative opcode name. */
#define OP_je_short   OP_jz_short   /**< Alternative opcode name. */
#define OP_jne_short  OP_jnz_short  /**< Alternative opcode name. */
#define OP_jge_short  OP_jnl_short  /**< Alternative opcode name. */
#define OP_jg_short   OP_jnle_short /**< Alternative opcode name. */
#define OP_jae  OP_jnb        /**< Alternative opcode name. */
#define OP_jnae OP_jb         /**< Alternative opcode name. */
#define OP_ja   OP_jnbe       /**< Alternative opcode name. */
#define OP_jna  OP_jbe        /**< Alternative opcode name. */
#define OP_je   OP_jz         /**< Alternative opcode name. */
#define OP_jne  OP_jnz        /**< Alternative opcode name. */
#define OP_jge  OP_jnl        /**< Alternative opcode name. */
#define OP_jg   OP_jnle       /**< Alternative opcode name. */
#define OP_setae  OP_setnb    /**< Alternative opcode name. */
#define OP_setnae OP_setb     /**< Alternative opcode name. */
#define OP_seta   OP_setnbe   /**< Alternative opcode name. */
#define OP_setna  OP_setbe    /**< Alternative opcode name. */
#define OP_sete   OP_setz     /**< Alternative opcode name. */
#define OP_setne  OP_setnz    /**< Alternative opcode name. */
#define OP_setge  OP_setnl    /**< Alternative opcode name. */
#define OP_setg   OP_setnle   /**< Alternative opcode name. */
#define OP_cmovae  OP_cmovnb  /**< Alternative opcode name. */
#define OP_cmovnae OP_cmovb   /**< Alternative opcode name. */
#define OP_cmova   OP_cmovnbe /**< Alternative opcode name. */
#define OP_cmovna  OP_cmovbe  /**< Alternative opcode name. */
#define OP_cmove   OP_cmovz   /**< Alternative opcode name. */
#define OP_cmovne  OP_cmovnz  /**< Alternative opcode name. */
#define OP_cmovge  OP_cmovnl  /**< Alternative opcode name. */
#define OP_cmovg   OP_cmovnle /**< Alternative opcode name. */
/* undocumented opcodes */
#define OP_icebp OP_int1
#define OP_setalc OP_salc

#ifdef __cplusplus
} /* extern */
} /* kernel namespace */
#endif

#endif /* Granary_DYNAMORIO_H_ */
