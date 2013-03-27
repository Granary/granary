/*
 * wrappers.h
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_KERNEL_OVERRIDE_WRAPPERS_H_
#define granary_KERNEL_OVERRIDE_WRAPPERS_H_

#ifndef D
#   define D(...) __VA_ARGS__
#endif


#if defined(CAN_WRAP_kthread_create_on_node) && CAN_WRAP_kthread_create_on_node
#   define WRAPPER_FOR_kthread_create_on_node 1
    FUNCTION_WRAPPER(kthread_create_on_node, (struct task_struct *), (
        int (*threadfn)(void *),
        void *data,
        int node,
        const char namefmt[],
        ...
    ), {
        va_list args__;
        va_start(args__, namefmt);

        WRAP_FUNCTION(threadfn);
        char name_buff[sizeof(task_struct().comm)];
        vsnprintf(&(name_buff[0]), sizeof(name_buff), namefmt, args__);
        struct task_struct *ret = kthread_create_on_node(threadfn, data, node, name_buff);
        va_end(args__);
        RETURN_OUT_WRAP(ret);
        return ret;
    })
#endif


#if defined(CAN_WRAP_kthread_create) && CAN_WRAP_kthread_create
#   define WRAPPER_FOR_kthread_create 1
    FUNCTION_WRAPPER(kthread_create, (struct task_struct *), (
        int (*threadfn)(void *),
        void *data,
        const char namefmt[],
        ...
    ), {
        va_list args__;
        va_start(args__, namefmt);

        WRAP_FUNCTION(threadfn);
        char name_buff[sizeof(task_struct().comm)];
        vsnprintf(&(name_buff[0]), sizeof(name_buff), namefmt, args__);
        struct task_struct *ret = kthread_create(threadfn, data, name_buff);
        va_end(args__);
        RETURN_OUT_WRAP(ret);
        return ret;
    })
#endif


/// Disable wrapping of certain types.
#define WRAPPER_FOR_struct_tracepoint
#define WRAPPER_FOR_struct_module
#define WRAPPER_FOR_struct_gendisk
#define WRAPPER_FOR_struct_address
#define WRAPPER_FOR_struct_sk_buff
#define WRAPPER_FOR_struct_sk_buff_head
#define WRAPPER_FOR_struct_sched_class
#define WRAPPER_FOR_struct_nf_bridge_info
#define WRAPPER_FOR_struct_sock_common
#define WRAPPER_FOR_struct_sock
#define WRAPPER_FOR_struct_ctl_table_set
#define WRAPPER_FOR_struct_net
#define WRAPPER_FOR_struct_task_struct


/// Custom wrapped.
#define WRAPPER_FOR_struct_inode
TYPE_WRAPPER(struct inode, {
    NO_PRE_IN
    NO_POST_IN

    PRE_OUT {
        ABORT_IF_SUB_FUNCTION_IS_WRAPPED(arg.i_fop, llseek);
        PRE_OUT_WRAP(arg.i_fop);
        PRE_OUT_WRAP(arg.i_sb);
    }

    POST_OUT {
        ABORT_IF_SUB_FUNCTION_IS_WRAPPED(arg.i_op, lookup);

        PRE_OUT_WRAP(arg.i_op);
        PRE_OUT_WRAP(arg.i_mapping);

        // at a post-wrap depth of 2, we can generally get away with file
        // systems working with the following special case.
        if(2 == CONFIG_MAX_POST_WRAP_DEPTH
        && 1 == depth__
        && is_valid_address(arg.i_mapping)) {
            ++depth__;
            PRE_OUT_WRAP(arg.i_mapping->a_ops);
        }

    }

    NO_RETURN
})


#define WRAPPER_FOR_struct_super_block
TYPE_WRAPPER(struct super_block, {
    NO_POST
    NO_PRE_IN
    PRE_OUT {
        ABORT_IF_SUB_FUNCTION_IS_WRAPPED(arg.s_op, alloc_inode);

        PRE_OUT_WRAP(arg.s_op);
        PRE_OUT_WRAP(arg.dq_op);
        PRE_OUT_WRAP(arg.s_qcop);
        PRE_OUT_WRAP(arg.s_export_op);
        PRE_OUT_WRAP(arg.s_d_op);

        if(is_valid_address(arg.s_xattr)) {
            const struct xattr_handler **handlers(arg.s_xattr);
            const struct xattr_handler *handler(nullptr);
            for((handler) = *(handlers)++;
                handler;
                (handler) = *(handlers)++) {
                PRE_OUT_WRAP(handler);
            }
        }
    }
    NO_RETURN
})


#define WRAPPER_FOR_struct_address_space
TYPE_WRAPPER(struct address_space, {
    NO_PRE_IN
    PRE_OUT {
        ABORT_IF_SUB_FUNCTION_IS_WRAPPED(arg.a_ops, writepage);

        PRE_OUT_WRAP(arg.a_ops);
    }
    NO_POST
    NO_RETURN
})


#endif /* granary_KERNEL_OVERRIDE_WRAPPERS_H_ */
