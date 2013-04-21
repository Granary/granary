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
#define WRAPPER_FOR_struct_list_head
#define WRAPPER_FOR_struct_dentry
#define WRAPPER_FOR_struct_inode


/// Custom wrapping for super blocks.
#ifndef WRAPPER_FOR_struct_super_block
#   define WRAPPER_FOR_struct_super_block
    struct granary_super_block : public super_block { };
    TYPE_WRAPPER(granary_super_block, {
        NO_POST
        PRE_INOUT {
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
#endif


#ifndef WRAPPER_FOR_struct_address_space
#   define WRAPPER_FOR_struct_address_space
    TYPE_WRAPPER(struct address_space, {
        NO_PRE_IN
        PRE_OUT {
            ABORT_IF_SUB_FUNCTION_IS_WRAPPED(arg.a_ops, writepage);

            PRE_OUT_WRAP(arg.a_ops);
        }
        NO_POST
        NO_RETURN
    })
#endif


#ifndef WRAPPER_FOR_struct_file_system_type
#   define WRAPPER_FOR_struct_file_system_type
    TYPE_WRAPPER(struct file_system_type, {
        NO_PRE_IN
        PRE_OUT {
            WRAP_FUNCTION(arg.mount);
            WRAP_FUNCTION(arg.kill_sb);
        }
        NO_POST
        NO_RETURN
    })
#endif


#if defined(CAN_WRAP_iget_locked) && CAN_WRAP_iget_locked && !defined(WRAPPER_FOR_iget_locked)
#   define WRAPPER_FOR_iget_locked
    FUNCTION_WRAPPER(iget_locked, (struct inode *), (struct super_block *sb, unsigned long ino), {
        granary_super_block *sb_((granary_super_block *) sb);
        PRE_OUT_WRAP(sb_);
        inode *inode(iget_locked(sb, ino));
        PRE_OUT_WRAP(inode->i_mapping);
        return inode;
    })
#endif


#if defined(CAN_WRAP_unlock_new_inode) && CAN_WRAP_unlock_new_inode && !defined(WRAPPER_FOR_unlock_new_inode)
#   define WRAPPER_FOR_unlock_new_inode
    FUNCTION_WRAPPER(unlock_new_inode, (void), (struct inode *inode), {
        PRE_OUT_WRAP(inode->i_op);
        PRE_OUT_WRAP(inode->i_fop);
        PRE_OUT_WRAP(inode->i_mapping);
        unlock_new_inode(inode);
    })
#endif


/// Mount a block device. Make sure that the super block is wrapped when the
/// dynamic wrapper for `fill_super`
#if defined(CAN_WRAP_mount_bdev) && CAN_WRAP_mount_bdev && !defined(WRAPPER_FOR_mount_bdev)
#   define WRAPPER_FOR_mount_bdev
    FUNCTION_WRAPPER(mount_bdev, (struct dentry *), (
        struct file_system_type * fs_type ,
        int flags ,
        const char *dev_name ,
        void *data ,
        int (*fill_super)(struct super_block *, void *, int)
    ), {
        PRE_OUT_WRAP(fs_type);

        typedef int (granary_fill_super_t)(granary_super_block *, void *, int);

        granary_fill_super_t *&fill_super_((granary_fill_super_t *&) fill_super);
        WRAP_FUNCTION(fill_super);

        struct dentry *ret(mount_bdev(fs_type, flags, dev_name, data, fill_super));
        RETURN_IN_WRAP(ret);
        return ret;
    })
#endif


#endif /* granary_KERNEL_OVERRIDE_WRAPPERS_H_ */
