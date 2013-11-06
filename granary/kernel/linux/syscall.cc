/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * syscall.cc
 *
 *  Created on: 2013-11-06
 *          Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/detach.h"
#include "granary/state.h"
#include "granary/policy.h"
#include "granary/code_cache.h"

#if CONFIG_INSTRUMENT_HOST && CONFIG_ENABLE_TRACE_ALLOCATOR

namespace granary {


    /// Initialise an individual system call. This will create a new allocator
    /// for basic blocks executed by that system call, thus contributing to
    /// "tracing" that basic block.
    static void init_syscall(uintptr_t addr_) throw() {
        app_pc addr(reinterpret_cast<app_pc>(addr_));

        cpu_state_handle cpu;

        // Free up some memory.
        IF_TEST( cpu->in_granary = false; )
        cpu.free_transient_allocators();

        // Create a syscall-specific memory allocator.
        generic_fragment_allocator *allocator(
            allocate_memory<generic_fragment_allocator>());
        cpu->current_fragment_allocator = allocator;

        // Set up the CTI; we already know it will be indirectly JMP/CALLed to
        // by the system call entrypoint so we deal with those properties here
        // as well.
        instrumentation_policy policy(START_POLICY);
        policy.indirect_cti_target(true);
        mangled_address am(addr, policy);

        // Add the basic block.
        code_cache::find(cpu, am);
    }


    STATIC_INITIALISE_ID(syscall_vector_entrypoints, {
#ifdef DETACH_ADDR_sys_arch_prctl
        init_syscall(DETACH_ADDR_sys_arch_prctl);
#endif
#ifdef DETACH_ADDR_sys_rt_sigreturn
        init_syscall(DETACH_ADDR_sys_rt_sigreturn);
#endif
#ifdef DETACH_ADDR_sys_ioperm
        init_syscall(DETACH_ADDR_sys_ioperm);
#endif
#ifdef DETACH_ADDR_sys_iopl
        init_syscall(DETACH_ADDR_sys_iopl);
#endif
#ifdef DETACH_ADDR_sys_modify_ldt
        init_syscall(DETACH_ADDR_sys_modify_ldt);
#endif
#ifdef DETACH_ADDR_sys_mmap
        init_syscall(DETACH_ADDR_sys_mmap);
#endif
#ifdef DETACH_ADDR_sys_set_thread_area
        init_syscall(DETACH_ADDR_sys_set_thread_area);
#endif
#ifdef DETACH_ADDR_sys_get_thread_area
        init_syscall(DETACH_ADDR_sys_get_thread_area);
#endif
#ifdef DETACH_ADDR_sys_set_tid_address
        init_syscall(DETACH_ADDR_sys_set_tid_address);
#endif
#ifdef DETACH_ADDR_sys_fork
        init_syscall(DETACH_ADDR_sys_fork);
#endif
#ifdef DETACH_ADDR_sys_vfork
        init_syscall(DETACH_ADDR_sys_vfork);
#endif
#ifdef DETACH_ADDR_sys_clone
        init_syscall(DETACH_ADDR_sys_clone);
#endif
#ifdef DETACH_ADDR_sys_unshare
        init_syscall(DETACH_ADDR_sys_unshare);
#endif
#ifdef DETACH_ADDR_sys_personality
        init_syscall(DETACH_ADDR_sys_personality);
#endif
#ifdef DETACH_ADDR_sys_exit
        init_syscall(DETACH_ADDR_sys_exit);
#endif
#ifdef DETACH_ADDR_sys_exit_group
        init_syscall(DETACH_ADDR_sys_exit_group);
#endif
#ifdef DETACH_ADDR_sys_waitid
        init_syscall(DETACH_ADDR_sys_waitid);
#endif
#ifdef DETACH_ADDR_sys_wait4
        init_syscall(DETACH_ADDR_sys_wait4);
#endif
#ifdef DETACH_ADDR_sys_waitpid
        init_syscall(DETACH_ADDR_sys_waitpid);
#endif
#ifdef DETACH_ADDR_sys_getitimer
        init_syscall(DETACH_ADDR_sys_getitimer);
#endif
#ifdef DETACH_ADDR_sys_setitimer
        init_syscall(DETACH_ADDR_sys_setitimer);
#endif
#ifdef DETACH_ADDR_sys_time
        init_syscall(DETACH_ADDR_sys_time);
#endif
#ifdef DETACH_ADDR_sys_stime
        init_syscall(DETACH_ADDR_sys_stime);
#endif
#ifdef DETACH_ADDR_sys_gettimeofday
        init_syscall(DETACH_ADDR_sys_gettimeofday);
#endif
#ifdef DETACH_ADDR_sys_settimeofday
        init_syscall(DETACH_ADDR_sys_settimeofday);
#endif
#ifdef DETACH_ADDR_sys_adjtimex
        init_syscall(DETACH_ADDR_sys_adjtimex);
#endif
#ifdef DETACH_ADDR_sys_sysctl
        init_syscall(DETACH_ADDR_sys_sysctl);
#endif
#ifdef DETACH_ADDR_sys_capget
        init_syscall(DETACH_ADDR_sys_capget);
#endif
#ifdef DETACH_ADDR_sys_capset
        init_syscall(DETACH_ADDR_sys_capset);
#endif
#ifdef DETACH_ADDR_sys_ptrace
        init_syscall(DETACH_ADDR_sys_ptrace);
#endif
#ifdef DETACH_ADDR_sys_alarm
        init_syscall(DETACH_ADDR_sys_alarm);
#endif
#ifdef DETACH_ADDR_sys_restart_syscall
        init_syscall(DETACH_ADDR_sys_restart_syscall);
#endif
#ifdef DETACH_ADDR_sys_rt_sigprocmask
        init_syscall(DETACH_ADDR_sys_rt_sigprocmask);
#endif
#ifdef DETACH_ADDR_sys_rt_sigpending
        init_syscall(DETACH_ADDR_sys_rt_sigpending);
#endif
#ifdef DETACH_ADDR_sys_rt_sigtimedwait
        init_syscall(DETACH_ADDR_sys_rt_sigtimedwait);
#endif
#ifdef DETACH_ADDR_sys_kill
        init_syscall(DETACH_ADDR_sys_kill);
#endif
#ifdef DETACH_ADDR_sys_tgkill
        init_syscall(DETACH_ADDR_sys_tgkill);
#endif
#ifdef DETACH_ADDR_sys_tkill
        init_syscall(DETACH_ADDR_sys_tkill);
#endif
#ifdef DETACH_ADDR_sys_rt_sigqueueinfo
        init_syscall(DETACH_ADDR_sys_rt_sigqueueinfo);
#endif
#ifdef DETACH_ADDR_sys_rt_tgsigqueueinfo
        init_syscall(DETACH_ADDR_sys_rt_tgsigqueueinfo);
#endif
#ifdef DETACH_ADDR_sys_sigaltstack
        init_syscall(DETACH_ADDR_sys_sigaltstack);
#endif
#ifdef DETACH_ADDR_sys_sigpending
        init_syscall(DETACH_ADDR_sys_sigpending);
#endif
#ifdef DETACH_ADDR_sys_sigprocmask
        init_syscall(DETACH_ADDR_sys_sigprocmask);
#endif
#ifdef DETACH_ADDR_sys_rt_sigaction
        init_syscall(DETACH_ADDR_sys_rt_sigaction);
#endif
#ifdef DETACH_ADDR_sys_sgetmask
        init_syscall(DETACH_ADDR_sys_sgetmask);
#endif
#ifdef DETACH_ADDR_sys_ssetmask
        init_syscall(DETACH_ADDR_sys_ssetmask);
#endif
#ifdef DETACH_ADDR_sys_signal
        init_syscall(DETACH_ADDR_sys_signal);
#endif
#ifdef DETACH_ADDR_sys_pause
        init_syscall(DETACH_ADDR_sys_pause);
#endif
#ifdef DETACH_ADDR_sys_rt_sigsuspend
        init_syscall(DETACH_ADDR_sys_rt_sigsuspend);
#endif
#ifdef DETACH_ADDR_sys_sigsuspend
        init_syscall(DETACH_ADDR_sys_sigsuspend);
#endif
#ifdef DETACH_ADDR_sys_setpriority
        init_syscall(DETACH_ADDR_sys_setpriority);
#endif
#ifdef DETACH_ADDR_sys_getpriority
        init_syscall(DETACH_ADDR_sys_getpriority);
#endif
#ifdef DETACH_ADDR_sys_setregid
        init_syscall(DETACH_ADDR_sys_setregid);
#endif
#ifdef DETACH_ADDR_sys_setgid
        init_syscall(DETACH_ADDR_sys_setgid);
#endif
#ifdef DETACH_ADDR_sys_setreuid
        init_syscall(DETACH_ADDR_sys_setreuid);
#endif
#ifdef DETACH_ADDR_sys_setuid
        init_syscall(DETACH_ADDR_sys_setuid);
#endif
#ifdef DETACH_ADDR_sys_setresuid
        init_syscall(DETACH_ADDR_sys_setresuid);
#endif
#ifdef DETACH_ADDR_sys_getresuid
        init_syscall(DETACH_ADDR_sys_getresuid);
#endif
#ifdef DETACH_ADDR_sys_setresgid
        init_syscall(DETACH_ADDR_sys_setresgid);
#endif
#ifdef DETACH_ADDR_sys_getresgid
        init_syscall(DETACH_ADDR_sys_getresgid);
#endif
#ifdef DETACH_ADDR_sys_setfsuid
        init_syscall(DETACH_ADDR_sys_setfsuid);
#endif
#ifdef DETACH_ADDR_sys_setfsgid
        init_syscall(DETACH_ADDR_sys_setfsgid);
#endif
#ifdef DETACH_ADDR_sys_getpid
        init_syscall(DETACH_ADDR_sys_getpid);
#endif
#ifdef DETACH_ADDR_sys_gettid
        init_syscall(DETACH_ADDR_sys_gettid);
#endif
#ifdef DETACH_ADDR_sys_getppid
        init_syscall(DETACH_ADDR_sys_getppid);
#endif
#ifdef DETACH_ADDR_sys_getuid
        init_syscall(DETACH_ADDR_sys_getuid);
#endif
#ifdef DETACH_ADDR_sys_geteuid
        init_syscall(DETACH_ADDR_sys_geteuid);
#endif
#ifdef DETACH_ADDR_sys_getgid
        init_syscall(DETACH_ADDR_sys_getgid);
#endif
#ifdef DETACH_ADDR_sys_getegid
        init_syscall(DETACH_ADDR_sys_getegid);
#endif
#ifdef DETACH_ADDR_sys_times
        init_syscall(DETACH_ADDR_sys_times);
#endif
#ifdef DETACH_ADDR_sys_setpgid
        init_syscall(DETACH_ADDR_sys_setpgid);
#endif
#ifdef DETACH_ADDR_sys_getpgid
        init_syscall(DETACH_ADDR_sys_getpgid);
#endif
#ifdef DETACH_ADDR_sys_getpgrp
        init_syscall(DETACH_ADDR_sys_getpgrp);
#endif
#ifdef DETACH_ADDR_sys_getsid
        init_syscall(DETACH_ADDR_sys_getsid);
#endif
#ifdef DETACH_ADDR_sys_setsid
        init_syscall(DETACH_ADDR_sys_setsid);
#endif
#ifdef DETACH_ADDR_sys_newuname
        init_syscall(DETACH_ADDR_sys_newuname);
#endif
#ifdef DETACH_ADDR_sys_uname
        init_syscall(DETACH_ADDR_sys_uname);
#endif
#ifdef DETACH_ADDR_sys_olduname
        init_syscall(DETACH_ADDR_sys_olduname);
#endif
#ifdef DETACH_ADDR_sys_sethostname
        init_syscall(DETACH_ADDR_sys_sethostname);
#endif
#ifdef DETACH_ADDR_sys_gethostname
        init_syscall(DETACH_ADDR_sys_gethostname);
#endif
#ifdef DETACH_ADDR_sys_setdomainname
        init_syscall(DETACH_ADDR_sys_setdomainname);
#endif
#ifdef DETACH_ADDR_sys_old_getrlimit
        init_syscall(DETACH_ADDR_sys_old_getrlimit);
#endif
#ifdef DETACH_ADDR_sys_getrlimit
        init_syscall(DETACH_ADDR_sys_getrlimit);
#endif
#ifdef DETACH_ADDR_sys_prlimit64
        init_syscall(DETACH_ADDR_sys_prlimit64);
#endif
#ifdef DETACH_ADDR_sys_setrlimit
        init_syscall(DETACH_ADDR_sys_setrlimit);
#endif
#ifdef DETACH_ADDR_sys_getrusage
        init_syscall(DETACH_ADDR_sys_getrusage);
#endif
#ifdef DETACH_ADDR_sys_umask
        init_syscall(DETACH_ADDR_sys_umask);
#endif
#ifdef DETACH_ADDR_sys_prctl
        init_syscall(DETACH_ADDR_sys_prctl);
#endif
#ifdef DETACH_ADDR_sys_getcpu
        init_syscall(DETACH_ADDR_sys_getcpu);
#endif
#ifdef DETACH_ADDR_sys_sysinfo
        init_syscall(DETACH_ADDR_sys_sysinfo);
#endif
#ifdef DETACH_ADDR_sys_timer_create
        init_syscall(DETACH_ADDR_sys_timer_create);
#endif
#ifdef DETACH_ADDR_sys_timer_gettime
        init_syscall(DETACH_ADDR_sys_timer_gettime);
#endif
#ifdef DETACH_ADDR_sys_timer_getoverrun
        init_syscall(DETACH_ADDR_sys_timer_getoverrun);
#endif
#ifdef DETACH_ADDR_sys_timer_settime
        init_syscall(DETACH_ADDR_sys_timer_settime);
#endif
#ifdef DETACH_ADDR_sys_timer_delete
        init_syscall(DETACH_ADDR_sys_timer_delete);
#endif
#ifdef DETACH_ADDR_sys_clock_settime
        init_syscall(DETACH_ADDR_sys_clock_settime);
#endif
#ifdef DETACH_ADDR_sys_clock_gettime
        init_syscall(DETACH_ADDR_sys_clock_gettime);
#endif
#ifdef DETACH_ADDR_sys_clock_adjtime
        init_syscall(DETACH_ADDR_sys_clock_adjtime);
#endif
#ifdef DETACH_ADDR_sys_clock_getres
        init_syscall(DETACH_ADDR_sys_clock_getres);
#endif
#ifdef DETACH_ADDR_sys_clock_nanosleep
        init_syscall(DETACH_ADDR_sys_clock_nanosleep);
#endif
#ifdef DETACH_ADDR_sys_ni_syscall
        init_syscall(DETACH_ADDR_sys_ni_syscall);
#endif
#ifdef DETACH_ADDR_sys_ipc
        init_syscall(DETACH_ADDR_sys_ipc);
#endif
#ifdef DETACH_ADDR_sys_pciconfig_iobase
        init_syscall(DETACH_ADDR_sys_pciconfig_iobase);
#endif
#ifdef DETACH_ADDR_sys_pciconfig_read
        init_syscall(DETACH_ADDR_sys_pciconfig_read);
#endif
#ifdef DETACH_ADDR_sys_pciconfig_write
        init_syscall(DETACH_ADDR_sys_pciconfig_write);
#endif
#ifdef DETACH_ADDR_sys_spu_create
        init_syscall(DETACH_ADDR_sys_spu_create);
#endif
#ifdef DETACH_ADDR_sys_spu_run
        init_syscall(DETACH_ADDR_sys_spu_run);
#endif
#ifdef DETACH_ADDR_sys_subpage_prot
        init_syscall(DETACH_ADDR_sys_subpage_prot);
#endif
#ifdef DETACH_ADDR_sys_vm86
        init_syscall(DETACH_ADDR_sys_vm86);
#endif
#ifdef DETACH_ADDR_sys_vm86old
        init_syscall(DETACH_ADDR_sys_vm86old);
#endif
#ifdef DETACH_ADDR_sys_nanosleep
        init_syscall(DETACH_ADDR_sys_nanosleep);
#endif
#ifdef DETACH_ADDR_sys_setns
        init_syscall(DETACH_ADDR_sys_setns);
#endif
#ifdef DETACH_ADDR_sys_reboot
        init_syscall(DETACH_ADDR_sys_reboot);
#endif
#ifdef DETACH_ADDR_sys_getgroups
        init_syscall(DETACH_ADDR_sys_getgroups);
#endif
#ifdef DETACH_ADDR_sys_setgroups
        init_syscall(DETACH_ADDR_sys_setgroups);
#endif
#ifdef DETACH_ADDR_sys_nice
        init_syscall(DETACH_ADDR_sys_nice);
#endif
#ifdef DETACH_ADDR_sys_sched_setscheduler
        init_syscall(DETACH_ADDR_sys_sched_setscheduler);
#endif
#ifdef DETACH_ADDR_sys_sched_setparam
        init_syscall(DETACH_ADDR_sys_sched_setparam);
#endif
#ifdef DETACH_ADDR_sys_sched_getscheduler
        init_syscall(DETACH_ADDR_sys_sched_getscheduler);
#endif
#ifdef DETACH_ADDR_sys_sched_getparam
        init_syscall(DETACH_ADDR_sys_sched_getparam);
#endif
#ifdef DETACH_ADDR_sys_sched_getaffinity
        init_syscall(DETACH_ADDR_sys_sched_getaffinity);
#endif
#ifdef DETACH_ADDR_sys_sched_yield
        init_syscall(DETACH_ADDR_sys_sched_yield);
#endif
#ifdef DETACH_ADDR_sys_sched_get_priority_max
        init_syscall(DETACH_ADDR_sys_sched_get_priority_max);
#endif
#ifdef DETACH_ADDR_sys_sched_get_priority_min
        init_syscall(DETACH_ADDR_sys_sched_get_priority_min);
#endif
#ifdef DETACH_ADDR_sys_sched_rr_get_interval
        init_syscall(DETACH_ADDR_sys_sched_rr_get_interval);
#endif
#ifdef DETACH_ADDR_sys_sched_setaffinity
        init_syscall(DETACH_ADDR_sys_sched_setaffinity);
#endif
#ifdef DETACH_ADDR_sys_syslog
        init_syscall(DETACH_ADDR_sys_syslog);
#endif
#ifdef DETACH_ADDR_sys_kcmp
        init_syscall(DETACH_ADDR_sys_kcmp);
#endif
#ifdef DETACH_ADDR_sys_set_robust_list
        init_syscall(DETACH_ADDR_sys_set_robust_list);
#endif
#ifdef DETACH_ADDR_sys_get_robust_list
        init_syscall(DETACH_ADDR_sys_get_robust_list);
#endif
#ifdef DETACH_ADDR_sys_futex
        init_syscall(DETACH_ADDR_sys_futex);
#endif
#ifdef DETACH_ADDR_sys_chown16
        init_syscall(DETACH_ADDR_sys_chown16);
#endif
#ifdef DETACH_ADDR_sys_lchown16
        init_syscall(DETACH_ADDR_sys_lchown16);
#endif
#ifdef DETACH_ADDR_sys_fchown16
        init_syscall(DETACH_ADDR_sys_fchown16);
#endif
#ifdef DETACH_ADDR_sys_setregid16
        init_syscall(DETACH_ADDR_sys_setregid16);
#endif
#ifdef DETACH_ADDR_sys_setgid16
        init_syscall(DETACH_ADDR_sys_setgid16);
#endif
#ifdef DETACH_ADDR_sys_setreuid16
        init_syscall(DETACH_ADDR_sys_setreuid16);
#endif
#ifdef DETACH_ADDR_sys_setuid16
        init_syscall(DETACH_ADDR_sys_setuid16);
#endif
#ifdef DETACH_ADDR_sys_setresuid16
        init_syscall(DETACH_ADDR_sys_setresuid16);
#endif
#ifdef DETACH_ADDR_sys_getresuid16
        init_syscall(DETACH_ADDR_sys_getresuid16);
#endif
#ifdef DETACH_ADDR_sys_setresgid16
        init_syscall(DETACH_ADDR_sys_setresgid16);
#endif
#ifdef DETACH_ADDR_sys_getresgid16
        init_syscall(DETACH_ADDR_sys_getresgid16);
#endif
#ifdef DETACH_ADDR_sys_setfsuid16
        init_syscall(DETACH_ADDR_sys_setfsuid16);
#endif
#ifdef DETACH_ADDR_sys_setfsgid16
        init_syscall(DETACH_ADDR_sys_setfsgid16);
#endif
#ifdef DETACH_ADDR_sys_getgroups16
        init_syscall(DETACH_ADDR_sys_getgroups16);
#endif
#ifdef DETACH_ADDR_sys_setgroups16
        init_syscall(DETACH_ADDR_sys_setgroups16);
#endif
#ifdef DETACH_ADDR_sys_getuid16
        init_syscall(DETACH_ADDR_sys_getuid16);
#endif
#ifdef DETACH_ADDR_sys_geteuid16
        init_syscall(DETACH_ADDR_sys_geteuid16);
#endif
#ifdef DETACH_ADDR_sys_getgid16
        init_syscall(DETACH_ADDR_sys_getgid16);
#endif
#ifdef DETACH_ADDR_sys_getegid16
        init_syscall(DETACH_ADDR_sys_getegid16);
#endif
#ifdef DETACH_ADDR_sys_delete_module
        init_syscall(DETACH_ADDR_sys_delete_module);
#endif
#ifdef DETACH_ADDR_sys_init_module
        init_syscall(DETACH_ADDR_sys_init_module);
#endif
#ifdef DETACH_ADDR_sys_finit_module
        init_syscall(DETACH_ADDR_sys_finit_module);
#endif
#ifdef DETACH_ADDR_sys_acct
        init_syscall(DETACH_ADDR_sys_acct);
#endif
#ifdef DETACH_ADDR_sys_kexec_load
        init_syscall(DETACH_ADDR_sys_kexec_load);
#endif
#ifdef DETACH_ADDR_sys_perf_event_open
        init_syscall(DETACH_ADDR_sys_perf_event_open);
#endif
#ifdef DETACH_ADDR_sys_fadvise64_64
        init_syscall(DETACH_ADDR_sys_fadvise64_64);
#endif
#ifdef DETACH_ADDR_sys_fadvise64
        init_syscall(DETACH_ADDR_sys_fadvise64);
#endif
#ifdef DETACH_ADDR_sys_readahead
        init_syscall(DETACH_ADDR_sys_readahead);
#endif
#ifdef DETACH_ADDR_sys_remap_file_pages
        init_syscall(DETACH_ADDR_sys_remap_file_pages);
#endif
#ifdef DETACH_ADDR_sys_madvise
        init_syscall(DETACH_ADDR_sys_madvise);
#endif
#ifdef DETACH_ADDR_sys_mincore
        init_syscall(DETACH_ADDR_sys_mincore);
#endif
#ifdef DETACH_ADDR_sys_mlock
        init_syscall(DETACH_ADDR_sys_mlock);
#endif
#ifdef DETACH_ADDR_sys_munlock
        init_syscall(DETACH_ADDR_sys_munlock);
#endif
#ifdef DETACH_ADDR_sys_mlockall
        init_syscall(DETACH_ADDR_sys_mlockall);
#endif
#ifdef DETACH_ADDR_sys_munlockall
        init_syscall(DETACH_ADDR_sys_munlockall);
#endif
#ifdef DETACH_ADDR_sys_mmap_pgoff
        init_syscall(DETACH_ADDR_sys_mmap_pgoff);
#endif
#ifdef DETACH_ADDR_sys_brk
        init_syscall(DETACH_ADDR_sys_brk);
#endif
#ifdef DETACH_ADDR_sys_munmap
        init_syscall(DETACH_ADDR_sys_munmap);
#endif
#ifdef DETACH_ADDR_sys_mprotect
        init_syscall(DETACH_ADDR_sys_mprotect);
#endif
#ifdef DETACH_ADDR_sys_mremap
        init_syscall(DETACH_ADDR_sys_mremap);
#endif
#ifdef DETACH_ADDR_sys_msync
        init_syscall(DETACH_ADDR_sys_msync);
#endif
#ifdef DETACH_ADDR_sys_process_vm_readv
        init_syscall(DETACH_ADDR_sys_process_vm_readv);
#endif
#ifdef DETACH_ADDR_sys_process_vm_writev
        init_syscall(DETACH_ADDR_sys_process_vm_writev);
#endif
#ifdef DETACH_ADDR_sys_swapoff
        init_syscall(DETACH_ADDR_sys_swapoff);
#endif
#ifdef DETACH_ADDR_sys_swapon
        init_syscall(DETACH_ADDR_sys_swapon);
#endif
#ifdef DETACH_ADDR_sys_set_mempolicy
        init_syscall(DETACH_ADDR_sys_set_mempolicy);
#endif
#ifdef DETACH_ADDR_sys_migrate_pages
        init_syscall(DETACH_ADDR_sys_migrate_pages);
#endif
#ifdef DETACH_ADDR_sys_get_mempolicy
        init_syscall(DETACH_ADDR_sys_get_mempolicy);
#endif
#ifdef DETACH_ADDR_sys_mbind
        init_syscall(DETACH_ADDR_sys_mbind);
#endif
#ifdef DETACH_ADDR_sys_move_pages
        init_syscall(DETACH_ADDR_sys_move_pages);
#endif
#ifdef DETACH_ADDR_sys_truncate
        init_syscall(DETACH_ADDR_sys_truncate);
#endif
#ifdef DETACH_ADDR_sys_ftruncate
        init_syscall(DETACH_ADDR_sys_ftruncate);
#endif
#ifdef DETACH_ADDR_sys_fallocate
        init_syscall(DETACH_ADDR_sys_fallocate);
#endif
#ifdef DETACH_ADDR_sys_faccessat
        init_syscall(DETACH_ADDR_sys_faccessat);
#endif
#ifdef DETACH_ADDR_sys_access
        init_syscall(DETACH_ADDR_sys_access);
#endif
#ifdef DETACH_ADDR_sys_chdir
        init_syscall(DETACH_ADDR_sys_chdir);
#endif
#ifdef DETACH_ADDR_sys_fchdir
        init_syscall(DETACH_ADDR_sys_fchdir);
#endif
#ifdef DETACH_ADDR_sys_chroot
        init_syscall(DETACH_ADDR_sys_chroot);
#endif
#ifdef DETACH_ADDR_sys_fchmod
        init_syscall(DETACH_ADDR_sys_fchmod);
#endif
#ifdef DETACH_ADDR_sys_fchmodat
        init_syscall(DETACH_ADDR_sys_fchmodat);
#endif
#ifdef DETACH_ADDR_sys_chmod
        init_syscall(DETACH_ADDR_sys_chmod);
#endif
#ifdef DETACH_ADDR_sys_fchownat
        init_syscall(DETACH_ADDR_sys_fchownat);
#endif
#ifdef DETACH_ADDR_sys_chown
        init_syscall(DETACH_ADDR_sys_chown);
#endif
#ifdef DETACH_ADDR_sys_lchown
        init_syscall(DETACH_ADDR_sys_lchown);
#endif
#ifdef DETACH_ADDR_sys_fchown
        init_syscall(DETACH_ADDR_sys_fchown);
#endif
#ifdef DETACH_ADDR_sys_open
        init_syscall(DETACH_ADDR_sys_open);
#endif
#ifdef DETACH_ADDR_sys_openat
        init_syscall(DETACH_ADDR_sys_openat);
#endif
#ifdef DETACH_ADDR_sys_creat
        init_syscall(DETACH_ADDR_sys_creat);
#endif
#ifdef DETACH_ADDR_sys_close
        init_syscall(DETACH_ADDR_sys_close);
#endif
#ifdef DETACH_ADDR_sys_vhangup
        init_syscall(DETACH_ADDR_sys_vhangup);
#endif
#ifdef DETACH_ADDR_sys_lseek
        init_syscall(DETACH_ADDR_sys_lseek);
#endif
#ifdef DETACH_ADDR_sys_llseek
        init_syscall(DETACH_ADDR_sys_llseek);
#endif
#ifdef DETACH_ADDR_sys_read
        init_syscall(DETACH_ADDR_sys_read);
#endif
#ifdef DETACH_ADDR_sys_write
        init_syscall(DETACH_ADDR_sys_write);
#endif
#ifdef DETACH_ADDR_sys_pread64
        init_syscall(DETACH_ADDR_sys_pread64);
#endif
#ifdef DETACH_ADDR_sys_pwrite64
        init_syscall(DETACH_ADDR_sys_pwrite64);
#endif
#ifdef DETACH_ADDR_sys_readv
        init_syscall(DETACH_ADDR_sys_readv);
#endif
#ifdef DETACH_ADDR_sys_writev
        init_syscall(DETACH_ADDR_sys_writev);
#endif
#ifdef DETACH_ADDR_sys_preadv
        init_syscall(DETACH_ADDR_sys_preadv);
#endif
#ifdef DETACH_ADDR_sys_pwritev
        init_syscall(DETACH_ADDR_sys_pwritev);
#endif
#ifdef DETACH_ADDR_sys_sendfile
        init_syscall(DETACH_ADDR_sys_sendfile);
#endif
#ifdef DETACH_ADDR_sys_sendfile64
        init_syscall(DETACH_ADDR_sys_sendfile64);
#endif
#ifdef DETACH_ADDR_sys_stat
        init_syscall(DETACH_ADDR_sys_stat);
#endif
#ifdef DETACH_ADDR_sys_lstat
        init_syscall(DETACH_ADDR_sys_lstat);
#endif
#ifdef DETACH_ADDR_sys_fstat
        init_syscall(DETACH_ADDR_sys_fstat);
#endif
#ifdef DETACH_ADDR_sys_newstat
        init_syscall(DETACH_ADDR_sys_newstat);
#endif
#ifdef DETACH_ADDR_sys_newlstat
        init_syscall(DETACH_ADDR_sys_newlstat);
#endif
#ifdef DETACH_ADDR_sys_newfstatat
        init_syscall(DETACH_ADDR_sys_newfstatat);
#endif
#ifdef DETACH_ADDR_sys_newfstat
        init_syscall(DETACH_ADDR_sys_newfstat);
#endif
#ifdef DETACH_ADDR_sys_readlinkat
        init_syscall(DETACH_ADDR_sys_readlinkat);
#endif
#ifdef DETACH_ADDR_sys_readlink
        init_syscall(DETACH_ADDR_sys_readlink);
#endif
#ifdef DETACH_ADDR_sys_uselib
        init_syscall(DETACH_ADDR_sys_uselib);
#endif
#ifdef DETACH_ADDR_sys_execve
        init_syscall(DETACH_ADDR_sys_execve);
#endif
#ifdef DETACH_ADDR_sys_pipe2
        init_syscall(DETACH_ADDR_sys_pipe2);
#endif
#ifdef DETACH_ADDR_sys_pipe
        init_syscall(DETACH_ADDR_sys_pipe);
#endif
#ifdef DETACH_ADDR_sys_mknodat
        init_syscall(DETACH_ADDR_sys_mknodat);
#endif
#ifdef DETACH_ADDR_sys_mknod
        init_syscall(DETACH_ADDR_sys_mknod);
#endif
#ifdef DETACH_ADDR_sys_mkdirat
        init_syscall(DETACH_ADDR_sys_mkdirat);
#endif
#ifdef DETACH_ADDR_sys_mkdir
        init_syscall(DETACH_ADDR_sys_mkdir);
#endif
#ifdef DETACH_ADDR_sys_rmdir
        init_syscall(DETACH_ADDR_sys_rmdir);
#endif
#ifdef DETACH_ADDR_sys_unlinkat
        init_syscall(DETACH_ADDR_sys_unlinkat);
#endif
#ifdef DETACH_ADDR_sys_unlink
        init_syscall(DETACH_ADDR_sys_unlink);
#endif
#ifdef DETACH_ADDR_sys_symlinkat
        init_syscall(DETACH_ADDR_sys_symlinkat);
#endif
#ifdef DETACH_ADDR_sys_symlink
        init_syscall(DETACH_ADDR_sys_symlink);
#endif
#ifdef DETACH_ADDR_sys_linkat
        init_syscall(DETACH_ADDR_sys_linkat);
#endif
#ifdef DETACH_ADDR_sys_link
        init_syscall(DETACH_ADDR_sys_link);
#endif
#ifdef DETACH_ADDR_sys_renameat
        init_syscall(DETACH_ADDR_sys_renameat);
#endif
#ifdef DETACH_ADDR_sys_rename
        init_syscall(DETACH_ADDR_sys_rename);
#endif
#ifdef DETACH_ADDR_sys_fcntl
        init_syscall(DETACH_ADDR_sys_fcntl);
#endif
#ifdef DETACH_ADDR_sys_ioctl
        init_syscall(DETACH_ADDR_sys_ioctl);
#endif
#ifdef DETACH_ADDR_sys_old_readdir
        init_syscall(DETACH_ADDR_sys_old_readdir);
#endif
#ifdef DETACH_ADDR_sys_getdents
        init_syscall(DETACH_ADDR_sys_getdents);
#endif
#ifdef DETACH_ADDR_sys_getdents64
        init_syscall(DETACH_ADDR_sys_getdents64);
#endif
#ifdef DETACH_ADDR_sys_select
        init_syscall(DETACH_ADDR_sys_select);
#endif
#ifdef DETACH_ADDR_sys_pselect6
        init_syscall(DETACH_ADDR_sys_pselect6);
#endif
#ifdef DETACH_ADDR_sys_poll
        init_syscall(DETACH_ADDR_sys_poll);
#endif
#ifdef DETACH_ADDR_sys_ppoll
        init_syscall(DETACH_ADDR_sys_ppoll);
#endif
#ifdef DETACH_ADDR_sys_getcwd
        init_syscall(DETACH_ADDR_sys_getcwd);
#endif
#ifdef DETACH_ADDR_sys_dup3
        init_syscall(DETACH_ADDR_sys_dup3);
#endif
#ifdef DETACH_ADDR_sys_dup2
        init_syscall(DETACH_ADDR_sys_dup2);
#endif
#ifdef DETACH_ADDR_sys_dup
        init_syscall(DETACH_ADDR_sys_dup);
#endif
#ifdef DETACH_ADDR_sys_sysfs
        init_syscall(DETACH_ADDR_sys_sysfs);
#endif
#ifdef DETACH_ADDR_sys_umount
        init_syscall(DETACH_ADDR_sys_umount);
#endif
#ifdef DETACH_ADDR_sys_oldumount
        init_syscall(DETACH_ADDR_sys_oldumount);
#endif
#ifdef DETACH_ADDR_sys_mount
        init_syscall(DETACH_ADDR_sys_mount);
#endif
#ifdef DETACH_ADDR_sys_pivot_root
        init_syscall(DETACH_ADDR_sys_pivot_root);
#endif
#ifdef DETACH_ADDR_sys_setxattr
        init_syscall(DETACH_ADDR_sys_setxattr);
#endif
#ifdef DETACH_ADDR_sys_lsetxattr
        init_syscall(DETACH_ADDR_sys_lsetxattr);
#endif
#ifdef DETACH_ADDR_sys_fsetxattr
        init_syscall(DETACH_ADDR_sys_fsetxattr);
#endif
#ifdef DETACH_ADDR_sys_getxattr
        init_syscall(DETACH_ADDR_sys_getxattr);
#endif
#ifdef DETACH_ADDR_sys_lgetxattr
        init_syscall(DETACH_ADDR_sys_lgetxattr);
#endif
#ifdef DETACH_ADDR_sys_fgetxattr
        init_syscall(DETACH_ADDR_sys_fgetxattr);
#endif
#ifdef DETACH_ADDR_sys_listxattr
        init_syscall(DETACH_ADDR_sys_listxattr);
#endif
#ifdef DETACH_ADDR_sys_llistxattr
        init_syscall(DETACH_ADDR_sys_llistxattr);
#endif
#ifdef DETACH_ADDR_sys_flistxattr
        init_syscall(DETACH_ADDR_sys_flistxattr);
#endif
#ifdef DETACH_ADDR_sys_removexattr
        init_syscall(DETACH_ADDR_sys_removexattr);
#endif
#ifdef DETACH_ADDR_sys_lremovexattr
        init_syscall(DETACH_ADDR_sys_lremovexattr);
#endif
#ifdef DETACH_ADDR_sys_fremovexattr
        init_syscall(DETACH_ADDR_sys_fremovexattr);
#endif
#ifdef DETACH_ADDR_sys_vmsplice
        init_syscall(DETACH_ADDR_sys_vmsplice);
#endif
#ifdef DETACH_ADDR_sys_splice
        init_syscall(DETACH_ADDR_sys_splice);
#endif
#ifdef DETACH_ADDR_sys_tee
        init_syscall(DETACH_ADDR_sys_tee);
#endif
#ifdef DETACH_ADDR_sys_sync
        init_syscall(DETACH_ADDR_sys_sync);
#endif
#ifdef DETACH_ADDR_sys_syncfs
        init_syscall(DETACH_ADDR_sys_syncfs);
#endif
#ifdef DETACH_ADDR_sys_fsync
        init_syscall(DETACH_ADDR_sys_fsync);
#endif
#ifdef DETACH_ADDR_sys_fdatasync
        init_syscall(DETACH_ADDR_sys_fdatasync);
#endif
#ifdef DETACH_ADDR_sys_sync_file_range
        init_syscall(DETACH_ADDR_sys_sync_file_range);
#endif
#ifdef DETACH_ADDR_sys_sync_file_range2
        init_syscall(DETACH_ADDR_sys_sync_file_range2);
#endif
#ifdef DETACH_ADDR_sys_utime
        init_syscall(DETACH_ADDR_sys_utime);
#endif
#ifdef DETACH_ADDR_sys_utimensat
        init_syscall(DETACH_ADDR_sys_utimensat);
#endif
#ifdef DETACH_ADDR_sys_futimesat
        init_syscall(DETACH_ADDR_sys_futimesat);
#endif
#ifdef DETACH_ADDR_sys_utimes
        init_syscall(DETACH_ADDR_sys_utimes);
#endif
#ifdef DETACH_ADDR_sys_statfs
        init_syscall(DETACH_ADDR_sys_statfs);
#endif
#ifdef DETACH_ADDR_sys_statfs64
        init_syscall(DETACH_ADDR_sys_statfs64);
#endif
#ifdef DETACH_ADDR_sys_fstatfs
        init_syscall(DETACH_ADDR_sys_fstatfs);
#endif
#ifdef DETACH_ADDR_sys_fstatfs64
        init_syscall(DETACH_ADDR_sys_fstatfs64);
#endif
#ifdef DETACH_ADDR_sys_ustat
        init_syscall(DETACH_ADDR_sys_ustat);
#endif
#ifdef DETACH_ADDR_sys_bdflush
        init_syscall(DETACH_ADDR_sys_bdflush);
#endif
#ifdef DETACH_ADDR_sys_ioprio_set
        init_syscall(DETACH_ADDR_sys_ioprio_set);
#endif
#ifdef DETACH_ADDR_sys_ioprio_get
        init_syscall(DETACH_ADDR_sys_ioprio_get);
#endif
#ifdef DETACH_ADDR_sys_inotify_init1
        init_syscall(DETACH_ADDR_sys_inotify_init1);
#endif
#ifdef DETACH_ADDR_sys_inotify_init
        init_syscall(DETACH_ADDR_sys_inotify_init);
#endif
#ifdef DETACH_ADDR_sys_inotify_add_watch
        init_syscall(DETACH_ADDR_sys_inotify_add_watch);
#endif
#ifdef DETACH_ADDR_sys_inotify_rm_watch
        init_syscall(DETACH_ADDR_sys_inotify_rm_watch);
#endif
#ifdef DETACH_ADDR_sys_fanotify_init
        init_syscall(DETACH_ADDR_sys_fanotify_init);
#endif
#ifdef DETACH_ADDR_sys_fanotify_mark
        init_syscall(DETACH_ADDR_sys_fanotify_mark);
#endif
#ifdef DETACH_ADDR_sys_epoll_create1
        init_syscall(DETACH_ADDR_sys_epoll_create1);
#endif
#ifdef DETACH_ADDR_sys_epoll_create
        init_syscall(DETACH_ADDR_sys_epoll_create);
#endif
#ifdef DETACH_ADDR_sys_epoll_ctl
        init_syscall(DETACH_ADDR_sys_epoll_ctl);
#endif
#ifdef DETACH_ADDR_sys_epoll_wait
        init_syscall(DETACH_ADDR_sys_epoll_wait);
#endif
#ifdef DETACH_ADDR_sys_epoll_pwait
        init_syscall(DETACH_ADDR_sys_epoll_pwait);
#endif
#ifdef DETACH_ADDR_sys_signalfd4
        init_syscall(DETACH_ADDR_sys_signalfd4);
#endif
#ifdef DETACH_ADDR_sys_signalfd
        init_syscall(DETACH_ADDR_sys_signalfd);
#endif
#ifdef DETACH_ADDR_sys_timerfd_create
        init_syscall(DETACH_ADDR_sys_timerfd_create);
#endif
#ifdef DETACH_ADDR_sys_timerfd_settime
        init_syscall(DETACH_ADDR_sys_timerfd_settime);
#endif
#ifdef DETACH_ADDR_sys_timerfd_gettime
        init_syscall(DETACH_ADDR_sys_timerfd_gettime);
#endif
#ifdef DETACH_ADDR_sys_eventfd2
        init_syscall(DETACH_ADDR_sys_eventfd2);
#endif
#ifdef DETACH_ADDR_sys_eventfd
        init_syscall(DETACH_ADDR_sys_eventfd);
#endif
#ifdef DETACH_ADDR_sys_io_setup
        init_syscall(DETACH_ADDR_sys_io_setup);
#endif
#ifdef DETACH_ADDR_sys_io_destroy
        init_syscall(DETACH_ADDR_sys_io_destroy);
#endif
#ifdef DETACH_ADDR_sys_io_submit
        init_syscall(DETACH_ADDR_sys_io_submit);
#endif
#ifdef DETACH_ADDR_sys_io_cancel
        init_syscall(DETACH_ADDR_sys_io_cancel);
#endif
#ifdef DETACH_ADDR_sys_io_getevents
        init_syscall(DETACH_ADDR_sys_io_getevents);
#endif
#ifdef DETACH_ADDR_sys_flock
        init_syscall(DETACH_ADDR_sys_flock);
#endif
#ifdef DETACH_ADDR_sys_name_to_handle_at
        init_syscall(DETACH_ADDR_sys_name_to_handle_at);
#endif
#ifdef DETACH_ADDR_sys_open_by_handle_at
        init_syscall(DETACH_ADDR_sys_open_by_handle_at);
#endif
#ifdef DETACH_ADDR_sys_quotactl
        init_syscall(DETACH_ADDR_sys_quotactl);
#endif
#ifdef DETACH_ADDR_sys_lookup_dcookie
        init_syscall(DETACH_ADDR_sys_lookup_dcookie);
#endif
#ifdef DETACH_ADDR_sys_msgget
        init_syscall(DETACH_ADDR_sys_msgget);
#endif
#ifdef DETACH_ADDR_sys_msgctl
        init_syscall(DETACH_ADDR_sys_msgctl);
#endif
#ifdef DETACH_ADDR_sys_msgsnd
        init_syscall(DETACH_ADDR_sys_msgsnd);
#endif
#ifdef DETACH_ADDR_sys_msgrcv
        init_syscall(DETACH_ADDR_sys_msgrcv);
#endif
#ifdef DETACH_ADDR_sys_semget
        init_syscall(DETACH_ADDR_sys_semget);
#endif
#ifdef DETACH_ADDR_sys_semctl
        init_syscall(DETACH_ADDR_sys_semctl);
#endif
#ifdef DETACH_ADDR_sys_semtimedop
        init_syscall(DETACH_ADDR_sys_semtimedop);
#endif
#ifdef DETACH_ADDR_sys_semop
        init_syscall(DETACH_ADDR_sys_semop);
#endif
#ifdef DETACH_ADDR_sys_shmget
        init_syscall(DETACH_ADDR_sys_shmget);
#endif
#ifdef DETACH_ADDR_sys_shmctl
        init_syscall(DETACH_ADDR_sys_shmctl);
#endif
#ifdef DETACH_ADDR_sys_shmat
        init_syscall(DETACH_ADDR_sys_shmat);
#endif
#ifdef DETACH_ADDR_sys_shmdt
        init_syscall(DETACH_ADDR_sys_shmdt);
#endif
#ifdef DETACH_ADDR_sys_mq_open
        init_syscall(DETACH_ADDR_sys_mq_open);
#endif
#ifdef DETACH_ADDR_sys_mq_unlink
        init_syscall(DETACH_ADDR_sys_mq_unlink);
#endif
#ifdef DETACH_ADDR_sys_mq_timedsend
        init_syscall(DETACH_ADDR_sys_mq_timedsend);
#endif
#ifdef DETACH_ADDR_sys_mq_timedreceive
        init_syscall(DETACH_ADDR_sys_mq_timedreceive);
#endif
#ifdef DETACH_ADDR_sys_mq_notify
        init_syscall(DETACH_ADDR_sys_mq_notify);
#endif
#ifdef DETACH_ADDR_sys_mq_getsetattr
        init_syscall(DETACH_ADDR_sys_mq_getsetattr);
#endif
#ifdef DETACH_ADDR_sys_add_key
        init_syscall(DETACH_ADDR_sys_add_key);
#endif
#ifdef DETACH_ADDR_sys_request_key
        init_syscall(DETACH_ADDR_sys_request_key);
#endif
#ifdef DETACH_ADDR_sys_keyctl
        init_syscall(DETACH_ADDR_sys_keyctl);
#endif
#ifdef DETACH_ADDR_sys_dmi_field_show
        init_syscall(DETACH_ADDR_sys_dmi_field_show);
#endif
#ifdef DETACH_ADDR_sys_dmi_modalias_show
        init_syscall(DETACH_ADDR_sys_dmi_modalias_show);
#endif
#ifdef DETACH_ADDR_sys_socket
        init_syscall(DETACH_ADDR_sys_socket);
#endif
#ifdef DETACH_ADDR_sys_socketpair
        init_syscall(DETACH_ADDR_sys_socketpair);
#endif
#ifdef DETACH_ADDR_sys_bind
        init_syscall(DETACH_ADDR_sys_bind);
#endif
#ifdef DETACH_ADDR_sys_listen
        init_syscall(DETACH_ADDR_sys_listen);
#endif
#ifdef DETACH_ADDR_sys_accept4
        init_syscall(DETACH_ADDR_sys_accept4);
#endif
#ifdef DETACH_ADDR_sys_accept
        init_syscall(DETACH_ADDR_sys_accept);
#endif
#ifdef DETACH_ADDR_sys_connect
        init_syscall(DETACH_ADDR_sys_connect);
#endif
#ifdef DETACH_ADDR_sys_getsockname
        init_syscall(DETACH_ADDR_sys_getsockname);
#endif
#ifdef DETACH_ADDR_sys_getpeername
        init_syscall(DETACH_ADDR_sys_getpeername);
#endif
#ifdef DETACH_ADDR_sys_sendto
        init_syscall(DETACH_ADDR_sys_sendto);
#endif
#ifdef DETACH_ADDR_sys_send
        init_syscall(DETACH_ADDR_sys_send);
#endif
#ifdef DETACH_ADDR_sys_recvfrom
        init_syscall(DETACH_ADDR_sys_recvfrom);
#endif
#ifdef DETACH_ADDR_sys_recv
        init_syscall(DETACH_ADDR_sys_recv);
#endif
#ifdef DETACH_ADDR_sys_setsockopt
        init_syscall(DETACH_ADDR_sys_setsockopt);
#endif
#ifdef DETACH_ADDR_sys_getsockopt
        init_syscall(DETACH_ADDR_sys_getsockopt);
#endif
#ifdef DETACH_ADDR_sys_shutdown
        init_syscall(DETACH_ADDR_sys_shutdown);
#endif
#ifdef DETACH_ADDR_sys_sendmsg
        init_syscall(DETACH_ADDR_sys_sendmsg);
#endif
#ifdef DETACH_ADDR_sys_sendmmsg
        init_syscall(DETACH_ADDR_sys_sendmmsg);
#endif
#ifdef DETACH_ADDR_sys_recvmsg
        init_syscall(DETACH_ADDR_sys_recvmsg);
#endif
#ifdef DETACH_ADDR_sys_recvmmsg
        init_syscall(DETACH_ADDR_sys_recvmmsg);
#endif
#ifdef DETACH_ADDR_sys_socketcall
        init_syscall(DETACH_ADDR_sys_socketcall);
#endif
    })
}

#endif

