
SYSCALL_NAMES = set([
	"sys_arch_prctl",
	"sys_rt_sigreturn",
	"sys_ioperm",
	"sys_iopl",
	"sys_modify_ldt",
	"sys_mmap",
	"sys_set_thread_area",
	"sys_get_thread_area",
	"sys_set_tid_address",
	"sys_fork",
	"sys_vfork",
	"sys_clone",
	"sys_unshare",
	"sys_personality",
	"sys_exit",
	"sys_exit_group",
	"sys_waitid",
	"sys_wait4",
	"sys_waitpid",
	"sys_getitimer",
	"sys_setitimer",
	"sys_time",
	"sys_stime",
	"sys_gettimeofday",
	"sys_settimeofday",
	"sys_adjtimex",
	"sys_sysctl",
	"sys_capget",
	"sys_capset",
	"sys_ptrace",
	"sys_alarm",
	"sys_restart_syscall",
	"sys_rt_sigprocmask",
	"sys_rt_sigpending",
	"sys_rt_sigtimedwait",
	"sys_kill",
	"sys_tgkill",
	"sys_tkill",
	"sys_rt_sigqueueinfo",
	"sys_rt_tgsigqueueinfo",
	"sys_sigaltstack",
	"sys_sigpending",
	"sys_sigprocmask",
	"sys_rt_sigaction",
	"sys_sgetmask",
	"sys_ssetmask",
	"sys_signal",
	"sys_pause",
	"sys_rt_sigsuspend",
	"sys_sigsuspend",
	"sys_setpriority",
	"sys_getpriority",
	"sys_setregid",
	"sys_setgid",
	"sys_setreuid",
	"sys_setuid",
	"sys_setresuid",
	"sys_getresuid",
	"sys_setresgid",
	"sys_getresgid",
	"sys_setfsuid",
	"sys_setfsgid",
	"sys_getpid",
	"sys_gettid",
	"sys_getppid",
	"sys_getuid",
	"sys_geteuid",
	"sys_getgid",
	"sys_getegid",
	"sys_times",
	"sys_setpgid",
	"sys_getpgid",
	"sys_getpgrp",
	"sys_getsid",
	"sys_setsid",
	"sys_newuname",
	"sys_uname",
	"sys_olduname",
	"sys_sethostname",
	"sys_gethostname",
	"sys_setdomainname",
	"sys_old_getrlimit",
	"sys_getrlimit",
	"sys_prlimit64",
	"sys_setrlimit",
	"sys_getrusage",
	"sys_umask",
	"sys_prctl",
	"sys_getcpu",
	"sys_sysinfo",
	"sys_timer_create",
	"sys_timer_gettime",
	"sys_timer_getoverrun",
	"sys_timer_settime",
	"sys_timer_delete",
	"sys_clock_settime",
	"sys_clock_gettime",
	"sys_clock_adjtime",
	"sys_clock_getres",
	"sys_clock_nanosleep",
	"sys_ni_syscall",
	"sys_ipc",
	"sys_pciconfig_iobase",
	"sys_pciconfig_read",
	"sys_pciconfig_write",
	"sys_spu_create",
	"sys_spu_run",
	"sys_subpage_prot",
	"sys_vm86",
	"sys_vm86old",
	"sys_nanosleep",
	"sys_setns",
	"sys_reboot",
	"sys_getgroups",
	"sys_setgroups",
	"sys_nice",
	"sys_sched_setscheduler",
	"sys_sched_setparam",
	"sys_sched_getscheduler",
	"sys_sched_getparam",
	"sys_sched_getaffinity",
	"sys_sched_yield",
	"sys_sched_get_priority_max",
	"sys_sched_get_priority_min",
	"sys_sched_rr_get_interval",
	"sys_sched_setaffinity",
	"sys_syslog",
	"sys_kcmp",
	"sys_set_robust_list",
	"sys_get_robust_list",
	"sys_futex",
	"sys_chown16",
	"sys_lchown16",
	"sys_fchown16",
	"sys_setregid16",
	"sys_setgid16",
	"sys_setreuid16",
	"sys_setuid16",
	"sys_setresuid16",
	"sys_getresuid16",
	"sys_setresgid16",
	"sys_getresgid16",
	"sys_setfsuid16",
	"sys_setfsgid16",
	"sys_getgroups16",
	"sys_setgroups16",
	"sys_getuid16",
	"sys_geteuid16",
	"sys_getgid16",
	"sys_getegid16",
	"sys_delete_module",
	"sys_init_module",
	"sys_finit_module",
	"sys_acct",
	"sys_kexec_load",
	"sys_perf_event_open",
	"sys_fadvise64_64",
	"sys_fadvise64",
	"sys_readahead",
	"sys_remap_file_pages",
	"sys_madvise",
	"sys_mincore",
	"sys_mlock",
	"sys_munlock",
	"sys_mlockall",
	"sys_munlockall",
	"sys_mmap_pgoff",
	"sys_brk",
	"sys_munmap",
	"sys_mprotect",
	"sys_mremap",
	"sys_msync",
	"sys_process_vm_readv",
	"sys_process_vm_writev",
	"sys_swapoff",
	"sys_swapon",
	"sys_set_mempolicy",
	"sys_migrate_pages",
	"sys_get_mempolicy",
	"sys_mbind",
	"sys_move_pages",
	"sys_truncate",
	"sys_ftruncate",
	"sys_fallocate",
	"sys_faccessat",
	"sys_access",
	"sys_chdir",
	"sys_fchdir",
	"sys_chroot",
	"sys_fchmod",
	"sys_fchmodat",
	"sys_chmod",
	"sys_fchownat",
	"sys_chown",
	"sys_lchown",
	"sys_fchown",
	"sys_open",
	"sys_openat",
	"sys_creat",
	"sys_close",
	"sys_vhangup",
	"sys_lseek",
	"sys_llseek",
	"sys_read",
	"sys_write",
	"sys_pread64",
	"sys_pwrite64",
	"sys_readv",
	"sys_writev",
	"sys_preadv",
	"sys_pwritev",
	"sys_sendfile",
	"sys_sendfile64",
	"sys_stat",
	"sys_lstat",
	"sys_fstat",
	"sys_newstat",
	"sys_newlstat",
	"sys_newfstatat",
	"sys_newfstat",
	"sys_readlinkat",
	"sys_readlink",
	"sys_uselib",
	"sys_execve",
	"sys_pipe2",
	"sys_pipe",
	"sys_mknodat",
	"sys_mknod",
	"sys_mkdirat",
	"sys_mkdir",
	"sys_rmdir",
	"sys_unlinkat",
	"sys_unlink",
	"sys_symlinkat",
	"sys_symlink",
	"sys_linkat",
	"sys_link",
	"sys_renameat",
	"sys_rename",
	"sys_fcntl",
	"sys_ioctl",
	"sys_old_readdir",
	"sys_getdents",
	"sys_getdents64",
	"sys_select",
	"sys_pselect6",
	"sys_poll",
	"sys_ppoll",
	"sys_getcwd",
	"sys_dup3",
	"sys_dup2",
	"sys_dup",
	"sys_sysfs",
	"sys_umount",
	"sys_oldumount",
	"sys_mount",
	"sys_pivot_root",
	"sys_setxattr",
	"sys_lsetxattr",
	"sys_fsetxattr",
	"sys_getxattr",
	"sys_lgetxattr",
	"sys_fgetxattr",
	"sys_listxattr",
	"sys_llistxattr",
	"sys_flistxattr",
	"sys_removexattr",
	"sys_lremovexattr",
	"sys_fremovexattr",
	"sys_vmsplice",
	"sys_splice",
	"sys_tee",
	"sys_sync",
	"sys_syncfs",
	"sys_fsync",
	"sys_fdatasync",
	"sys_sync_file_range",
	"sys_sync_file_range2",
	"sys_utime",
	"sys_utimensat",
	"sys_futimesat",
	"sys_utimes",
	"sys_statfs",
	"sys_statfs64",
	"sys_fstatfs",
	"sys_fstatfs64",
	"sys_ustat",
	"sys_bdflush",
	"sys_ioprio_set",
	"sys_ioprio_get",
	"sys_inotify_init1",
	"sys_inotify_init",
	"sys_inotify_add_watch",
	"sys_inotify_rm_watch",
	"sys_fanotify_init",
	"sys_fanotify_mark",
	"sys_epoll_create1",
	"sys_epoll_create",
	"sys_epoll_ctl",
	"sys_epoll_wait",
	"sys_epoll_pwait",
	"sys_signalfd4",
	"sys_signalfd",
	"sys_timerfd_create",
	"sys_timerfd_settime",
	"sys_timerfd_gettime",
	"sys_eventfd2",
	"sys_eventfd",
	"sys_io_setup",
	"sys_io_destroy",
	"sys_io_submit",
	"sys_io_cancel",
	"sys_io_getevents",
	"sys_flock",
	"sys_name_to_handle_at",
	"sys_open_by_handle_at",
	"sys_quotactl",
	"sys_lookup_dcookie",
	"sys_msgget",
	"sys_msgctl",
	"sys_msgsnd",
	"sys_msgrcv",
	"sys_semget",
	"sys_semctl",
	"sys_semtimedop",
	"sys_semop",
	"sys_shmget",
	"sys_shmctl",
	"sys_shmat",
	"sys_shmdt",
	"sys_mq_open",
	"sys_mq_unlink",
	"sys_mq_timedsend",
	"sys_mq_timedreceive",
	"sys_mq_notify",
	"sys_mq_getsetattr",
	"sys_add_key",
	"sys_request_key",
	"sys_keyctl",
	"sys_dmi_field_show",
	"sys_dmi_modalias_show",
	"sys_socket",
	"sys_socketpair",
	"sys_bind",
	"sys_listen",
	"sys_accept4",
	"sys_accept",
	"sys_connect",
	"sys_getsockname",
	"sys_getpeername",
	"sys_sendto",
	"sys_send",
	"sys_recvfrom",
	"sys_recv",
	"sys_setsockopt",
	"sys_getsockopt",
	"sys_shutdown",
	"sys_sendmsg",
	"sys_sendmmsg",
	"sys_recvmsg",
	"sys_recvmmsg",
	"sys_socketcall",
])