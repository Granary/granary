/*
 * scanner.h
 *
 *  Created on: 2013-08-06
 *      Author: akshayk
 */

#ifndef _SHADOW_POLICY_SCANNER_H_
#define _SHADOW_POLICY_SCANNER_H_



template<typename T>
class type_class {
public:

    static unsigned get_size(void) throw () {
        unsigned size_ = sizeof(T);
        return size_;
    }
};

/// dummy for returning the same type; guarantees typdef syntax correctness
template <typename T>
class identity_type {
public:
    typedef T type;
};


#define TYPE_SCAN_WRAPPER(type_name, body) \
    template <> \
    struct scan_function<type_name> { \
    public: \
        enum { \
            IS_DEFINED = 1 \
        }; \
        typedef identity_type<type_name>::type ArgT__; \
        static void scan(ArgT__ &arg, const int depth__) \
        body \
    };

#define TYPE_SCANNER_CLASS(type_name) \
    template <> \
    struct scan_function<type_name> { \
    public: \
        enum { \
            IS_DEFINED = 1 \
        }; \
        typedef identity_type<type_name>::type ArgT__; \
        static void scan(ArgT__ &arg, const int depth__); \
    };

#define TYPE_SCANNER_BODY(type_name, body)  \
    void scan_function<type_name>:: \
    scan(identity_type<type_name>::type &arg, const int depth__) \
    body


#define SCAN_FUNCTION(lval) \
    type_scanner_field(lval); \


#define SCAN_RECURSIVE(lval)    \
    if(scan_function<decltype(lval)>::IS_DEFINED) {    \
        scan_function<decltype(lval)>::scan(lval, depth__ + 1);  \
    }


#define SCAN_RECURSIVE_PTR(lval)    \
    if(scan_function<decltype(lval)>::IS_DEFINED) {    \
        scan_function<decltype(lval)>::scan(lval, depth__ + 1);  \
    }


#define MAX_DEPTH_SCANNER 5


#define SCAN_HEAD_FUNC(type_name) \
    scan_function<type_name>::scan


template <typename T>
struct scan_function{
public:
    enum {
        IS_DEFINED = 0
    };

    static void scan(T & , const int) { }
};


template <>
struct scan_function<void *> {
public:
    enum {
        IS_DEFINED = 1
    };
    static void scan(void *, const int depth__) { }
};


template <typename T>
struct scan_function<T &> {
public:
    enum {
        IS_DEFINED = scan_function<T>::IS_DEFINED
    };

    static void scan(T &val, const int depth) {
        if(IS_DEFINED) {
            scan_function<T>::scan(val, depth);
        }
    }

    static inline void pre_scan(T &, const int) { }
};


template <typename T>
struct scan_function<T *> {
public:
    enum {
        IS_DEFINED = scan_function<T>::IS_DEFINED
    };

    static void scan(T *ptr, const int depth) {
        if(IS_DEFINED && granary::is_valid_address(ptr)) {
            scan_function<T>::scan(*ptr, depth);
        }
    }

};


template <typename T>
struct scan_function<const T> {
public:
    enum {
        IS_DEFINED = scan_function<T>::IS_DEFINED
    };

    static void scan(const T &val, const int depth) {
        if(IS_DEFINED) {
            scan_function<T>::scan(const_cast<T &>(val), depth);
        }
    }
};


template <typename T>
struct scan_function<volatile T> {
public:
    enum {
        IS_DEFINED = scan_function<T>::IS_DEFINED
    };

    static void scan(const T &val, const int depth) {
        if(IS_DEFINED) {
            scan_function<T>::scan(const_cast<T &>(val), depth);
        }
    }
};


template <typename T>
struct scan_function<const volatile T> {
public:
    enum {
        IS_DEFINED = scan_function<T>::IS_DEFINED
    };

    static void scan(const T &val, const int depth) {
        if(IS_DEFINED) {
            scan_function<T>::scan(const_cast<T &>(val), depth);
        }
    }

};


template <typename T>
bool type_scanner_field(T* ptr) {
    if(client::wp::is_watched_address(ptr)){
        granary::printf("watchpoint : %llx\n", ptr);
        return true;
    }
    return false;
}


template <typename T>
bool type_scanner_field(T addr) {
    if(client::wp::is_watched_address(&addr)){
        granary::printf("watchpoint : %llx\n", addr);
        return true;
    }
    return false;
}

#define SCANNER_FOR_struct_kobject
#define SCANNER_FOR_struct_file
#define SCANNER_FOR_struct_ts_ops
#define SCANNER_FOR_struct_dma_chan
#define SCANNER_FOR_struct_bus_type
#define SCANNER_FOR_struct_input_handle
#define SCANNER_FOR_struct_transaction_s
#define SCANNER_FOR_struct_device
#define SCANNER_FOR_struct_io_context
#define SCANNER_FOR_struct_backing_dev_info
#define SCANNER_FOR_struct_dst_ops
#define SCANNER_FOR_struct_bio
#define SCANNER_FOR_struct_request_queue
#define SCANNER_FOR_struct_sock
#define SCANNER_FOR_struct_gendisk
#define SCANNER_FOR_struct_block_device
#define SCANNER_FOR_struct_page
#define SCANNER_FOR_struct_crypto_tfm
#define SCANNER_FOR_struct_kthread_work
#define SCANNER_FOR_struct_mm_struct
#define SCANNER_FOR_struct_sk_buff_head
#define SCANNER_FOR_struct_module
#define SCANNER_FOR_struct_super_block
#define SCANNER_FOR_struct_ata_port_operations
#define SCANNER_FOR_struct_pci_dev
#define SCANNER_FOR_struct_ctl_table_header
#define SCANNER_FOR_struct_ctl_table_set
#define SCANNER_FOR_struct_ata_device
#define SCANNER_FOR_struct_sk_buff
#define SCANNER_FOR_struct_net
#define SCANNER_FOR_struct_neigh_parms
#define SCANNER_FOR_struct_mii_bus
#define SCANNER_FOR_struct_net_device
#define SCANNER_FOR_struct_dsa_switch_tree
#define SCANNER_FOR_struct_Qdisc
#define SCANNER_FOR_struct_request_sock_ops
#define SCANNER_FOR_struct_ata_port

#if 0
#define SCAN_OBJECT(arg)    \
    unsigned long size = type_class<decltype(arg)>::get_size();  \
    granary::printf("size : %llx\n", size);    \
    if(!granary::is_valid_address(&arg)){ \
        return; \
    }   \
    unsigned long i = 0;    \
    uint64_t *ptr = (uint64_t*)(&arg);  \
    while(i < size){    \
        uint64_t value = (uint64_t)(*ptr);  \
        if(client::wp::is_watched_address(value)) {   \
            granary::printf("watchpoint like object %llx,\n",(void*)value);   \
            if(client::wp::is_active_watchpoint(granary::unsafe_cast<void*>(value))){   \
                granary::printf("%llx, %llx,\n", ptr, (void*)value);   \
            }\
        }   \
        ptr++;  \
        i = i + sizeof(void*);  \
    }


#ifndef SCANNER_FOR_struct_inode
#define SCANNER_FOR_struct_inode
TYPE_SCANNER_CLASS(struct inode);
#endif

#ifndef SCANNER_FOR_struct_dentry
#define SCANNER_FOR_struct_dentry
TYPE_SCANNER_CLASS(struct dentry);
#endif

#ifndef SCANNER_FOR_struct_task_struct
#define SCANNER_FOR_struct_task_struct
TYPE_SCANNER_CLASS(struct task_struct);
#endif


#include "clients/gen/kernel_type_scanners.h"


TYPE_SCANNER_BODY(struct inode, {
    granary::printf( "struct inode\n");
    SCAN_OBJECT(arg);
    SCAN_FUNCTION(arg.i_mode);
    SCAN_FUNCTION(arg.i_opflags);
    SCAN_FUNCTION(arg.i_uid);
    SCAN_FUNCTION(arg.i_gid);
    SCAN_FUNCTION(arg.i_flags);
    SCAN_RECURSIVE_PTR(arg.i_acl);
    SCAN_RECURSIVE_PTR(arg.i_default_acl);
    SCAN_RECURSIVE_PTR(arg.i_op);
    SCAN_RECURSIVE_PTR(arg.i_sb);
    SCAN_RECURSIVE_PTR(arg.i_mapping);
    SCAN_FUNCTION(arg.i_ino);
//  Union(union anon_union_120) None
    SCAN_FUNCTION(arg.i_rdev);
    SCAN_FUNCTION(arg.i_size);
    SCAN_RECURSIVE(arg.i_atime);
    SCAN_RECURSIVE(arg.i_mtime);
    SCAN_RECURSIVE(arg.i_ctime);
    SCAN_RECURSIVE(arg.i_lock);
    SCAN_FUNCTION(arg.i_bytes);
    SCAN_FUNCTION(arg.i_blkbits);
    SCAN_FUNCTION(arg.i_blocks);
    SCAN_FUNCTION(arg.i_state);
    SCAN_RECURSIVE(arg.i_mutex);
    SCAN_FUNCTION(arg.dirtied_when);
    SCAN_RECURSIVE(arg.i_hash);
    SCAN_RECURSIVE(arg.i_wb_list);
    SCAN_RECURSIVE(arg.i_lru);
    SCAN_RECURSIVE(arg.i_sb_list);
//  Union(union anon_union_121) None
    SCAN_FUNCTION(arg.i_version);
    SCAN_RECURSIVE(arg.i_count);
    SCAN_RECURSIVE(arg.i_dio_count);
    SCAN_RECURSIVE(arg.i_writecount);
    SCAN_RECURSIVE_PTR(arg.i_fop);
    SCAN_RECURSIVE_PTR(arg.i_flock);
    SCAN_RECURSIVE(arg.i_data);
//  Array(Pointer(Use(Struct(struct dquot)))) arg.i_dquot
    SCAN_RECURSIVE(arg.i_devices);
//  Union(union anon_union_122) None
    SCAN_FUNCTION(arg.i_generation);
    SCAN_FUNCTION(arg.i_fsnotify_mask);
    SCAN_RECURSIVE(arg.i_fsnotify_marks);
    SCAN_FUNCTION(arg.i_private);
})


TYPE_SCANNER_BODY(struct dentry, {
    granary::printf( "struct dentry\n");
    SCAN_OBJECT(arg);
    SCAN_FUNCTION(arg.d_flags);
    SCAN_RECURSIVE(arg.d_seq);
    SCAN_RECURSIVE(arg.d_hash);
    SCAN_RECURSIVE_PTR(arg.d_parent);
    SCAN_RECURSIVE(arg.d_name);
    SCAN_RECURSIVE_PTR(arg.d_inode);
//  Array(Attributed(unsigned , BuiltIn(char))) arg.d_iname
    SCAN_FUNCTION(arg.d_count);
    SCAN_RECURSIVE(arg.d_lock);
    SCAN_RECURSIVE_PTR(arg.d_op);
    SCAN_RECURSIVE_PTR(arg.d_sb);
    SCAN_FUNCTION(arg.d_time);
    SCAN_FUNCTION(arg.d_fsdata);
    SCAN_RECURSIVE(arg.d_lru);
//  Union(union anon_union_113) arg.d_u
    SCAN_RECURSIVE(arg.d_subdirs);
    SCAN_RECURSIVE(arg.d_alias);
})


TYPE_SCANNER_BODY(struct task_struct, {
    granary::printf( "struct task_struct\n");
    SCAN_OBJECT(arg);
    SCAN_FUNCTION(arg.state);
    SCAN_FUNCTION(arg.stack);
    SCAN_RECURSIVE(arg.usage);
    SCAN_FUNCTION(arg.flags);
    SCAN_FUNCTION(arg.ptrace);
    SCAN_RECURSIVE(arg.wake_entry);
    SCAN_FUNCTION(arg.on_cpu);
    SCAN_FUNCTION(arg.on_rq);
    SCAN_FUNCTION(arg.prio);
    SCAN_FUNCTION(arg.static_prio);
    SCAN_FUNCTION(arg.normal_prio);
    SCAN_FUNCTION(arg.rt_priority);
    SCAN_RECURSIVE_PTR(arg.sched_class);
    SCAN_RECURSIVE(arg.se);
    SCAN_RECURSIVE(arg.rt);
    SCAN_RECURSIVE_PTR(arg.sched_task_group);
    SCAN_RECURSIVE(arg.preempt_notifiers);
    SCAN_FUNCTION(arg.fpu_counter);
    SCAN_FUNCTION(arg.policy);
    SCAN_FUNCTION(arg.nr_cpus_allowed);
    SCAN_RECURSIVE(arg.cpus_allowed);
    SCAN_RECURSIVE(arg.sched_info);
    SCAN_RECURSIVE(arg.tasks);
    SCAN_RECURSIVE(arg.pushable_tasks);
    SCAN_RECURSIVE_PTR(arg.mm);
    SCAN_RECURSIVE_PTR(arg.active_mm);
    SCAN_RECURSIVE(arg.rss_stat);
    SCAN_FUNCTION(arg.exit_state);
    SCAN_FUNCTION(arg.exit_code);
    SCAN_FUNCTION(arg.exit_signal);
    SCAN_FUNCTION(arg.pdeath_signal);
    SCAN_FUNCTION(arg.jobctl);
    SCAN_FUNCTION(arg.personality);
//  Bitfield(Attributed(unsigned , BuiltIn(int))) arg.did_exec
//  Bitfield(Attributed(unsigned , BuiltIn(int))) arg.in_execve
//  Bitfield(Attributed(unsigned , BuiltIn(int))) arg.in_iowait
//  Bitfield(Attributed(unsigned , BuiltIn(int))) arg.no_new_privs
//  Bitfield(Attributed(unsigned , BuiltIn(int))) arg.sched_reset_on_fork
//  Bitfield(Attributed(unsigned , BuiltIn(int))) arg.sched_contributes_to_load
    SCAN_FUNCTION(arg.pid);
    SCAN_FUNCTION(arg.tgid);
    SCAN_FUNCTION(arg.stack_canary);
    SCAN_RECURSIVE_PTR(arg.real_parent);
    SCAN_RECURSIVE_PTR(arg.parent);
    SCAN_RECURSIVE(arg.children);
    SCAN_RECURSIVE(arg.sibling);
    SCAN_RECURSIVE_PTR(arg.group_leader);
    SCAN_RECURSIVE(arg.ptraced);
    SCAN_RECURSIVE(arg.ptrace_entry);
//  Array(Use(Struct(struct pid_link))) arg.pids
    SCAN_RECURSIVE(arg.thread_group);
    SCAN_RECURSIVE_PTR(arg.vfork_done);
    SCAN_FUNCTION(arg.set_child_tid);
    SCAN_FUNCTION(arg.clear_child_tid);
    SCAN_FUNCTION(arg.utime);
    SCAN_FUNCTION(arg.stime);
    SCAN_FUNCTION(arg.utimescaled);
    SCAN_FUNCTION(arg.stimescaled);
    SCAN_FUNCTION(arg.gtime);
    SCAN_RECURSIVE(arg.prev_cputime);
    SCAN_FUNCTION(arg.nvcsw);
    SCAN_FUNCTION(arg.nivcsw);
    SCAN_RECURSIVE(arg.start_time);
    SCAN_RECURSIVE(arg.real_start_time);
    SCAN_FUNCTION(arg.min_flt);
    SCAN_FUNCTION(arg.maj_flt);
    SCAN_RECURSIVE(arg.cputime_expires);
//  Array(Use(Struct(struct list_head))) arg.cpu_timers
    SCAN_RECURSIVE_PTR(arg.real_cred);
    SCAN_RECURSIVE_PTR(arg.cred);
//  Array(BuiltIn(char)) arg.comm
    SCAN_FUNCTION(arg.link_count);
    SCAN_FUNCTION(arg.total_link_count);
    SCAN_RECURSIVE(arg.sysvsem);
    SCAN_FUNCTION(arg.last_switch_count);
    SCAN_RECURSIVE(arg.thread);
    SCAN_RECURSIVE_PTR(arg.fs);
    SCAN_RECURSIVE_PTR(arg.files);
    SCAN_RECURSIVE_PTR(arg.nsproxy);
    SCAN_RECURSIVE_PTR(arg.signal);
    SCAN_RECURSIVE_PTR(arg.sighand);
    SCAN_RECURSIVE(arg.blocked);
    SCAN_RECURSIVE(arg.real_blocked);
    SCAN_RECURSIVE(arg.saved_sigmask);
    SCAN_RECURSIVE(arg.pending);
    SCAN_FUNCTION(arg.sas_ss_sp);
    SCAN_FUNCTION(arg.sas_ss_size);
    SCAN_FUNCTION(arg.notifier);
    SCAN_FUNCTION(arg.notifier_data);
    SCAN_RECURSIVE_PTR(arg.notifier_mask);
    SCAN_RECURSIVE_PTR(arg.task_works);
    SCAN_RECURSIVE_PTR(arg.audit_context);
    SCAN_FUNCTION(arg.loginuid);
    SCAN_FUNCTION(arg.sessionid);
    SCAN_RECURSIVE(arg.seccomp);
    SCAN_FUNCTION(arg.parent_exec_id);
    SCAN_FUNCTION(arg.self_exec_id);
    SCAN_RECURSIVE(arg.alloc_lock);
    SCAN_RECURSIVE(arg.pi_lock);
    SCAN_RECURSIVE(arg.pi_waiters);
    SCAN_RECURSIVE_PTR(arg.pi_blocked_on);
    SCAN_FUNCTION(arg.journal_info);
    SCAN_RECURSIVE_PTR(arg.bio_list);
    SCAN_RECURSIVE_PTR(arg.plug);
    SCAN_RECURSIVE_PTR(arg.reclaim_state);
    SCAN_RECURSIVE_PTR(arg.backing_dev_info);
    SCAN_RECURSIVE_PTR(arg.io_context);
    SCAN_FUNCTION(arg.ptrace_message);
    SCAN_RECURSIVE_PTR(arg.last_siginfo);
    SCAN_RECURSIVE(arg.ioac);
    SCAN_FUNCTION(arg.acct_rss_mem1);
    SCAN_FUNCTION(arg.acct_vm_mem1);
    SCAN_FUNCTION(arg.acct_timexpd);
    SCAN_RECURSIVE(arg.mems_allowed);
    SCAN_RECURSIVE(arg.mems_allowed_seq);
    SCAN_FUNCTION(arg.cpuset_mem_spread_rotor);
    SCAN_FUNCTION(arg.cpuset_slab_spread_rotor);
    SCAN_RECURSIVE_PTR(arg.cgroups);
    SCAN_RECURSIVE(arg.cg_list);
    SCAN_RECURSIVE_PTR(arg.robust_list);
    SCAN_RECURSIVE_PTR(arg.compat_robust_list);
    SCAN_RECURSIVE(arg.pi_state_list);
    SCAN_RECURSIVE_PTR(arg.pi_state_cache);
//  Array(Pointer(Use(Struct(struct perf_event_context)))) arg.perf_event_ctxp
    SCAN_RECURSIVE(arg.perf_event_mutex);
    SCAN_RECURSIVE(arg.perf_event_list);
    SCAN_RECURSIVE_PTR(arg.mempolicy);
    SCAN_FUNCTION(arg.il_next);
    SCAN_FUNCTION(arg.pref_node_fork);
    SCAN_RECURSIVE(arg.rcu);
    SCAN_RECURSIVE_PTR(arg.splice_pipe);
    SCAN_RECURSIVE(arg.task_frag);
    SCAN_RECURSIVE_PTR(arg.delays);
    SCAN_FUNCTION(arg.nr_dirtied);
    SCAN_FUNCTION(arg.nr_dirtied_pause);
    SCAN_FUNCTION(arg.dirty_paused_when);
    SCAN_FUNCTION(arg.latency_record_count);
//  Array(Use(Struct(struct latency_record))) arg.latency_record
    SCAN_FUNCTION(arg.timer_slack_ns);
    SCAN_FUNCTION(arg.default_timer_slack_ns);
    SCAN_RECURSIVE(arg.ptrace_bp_refcnt);
})
#endif


#endif /* SCANNER_H_ */
