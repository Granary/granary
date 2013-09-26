/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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


#if defined(CAN_WRAP_kthread_create_on_node) && CAN_WRAP_kthread_create_on_node && !defined(APP_WRAPPER_FOR_kthread_create_on_node)
#   define APP_WRAPPER_FOR_kthread_create_on_node
    FUNCTION_WRAPPER(APP, kthread_create_on_node, (struct task_struct *), (
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


#if defined(CAN_WRAP_kthread_create) && CAN_WRAP_kthread_create && !defined(APP_WRAPPER_FOR_kthread_create)
#   define APP_WRAPPER_FOR_kthread_create
    FUNCTION_WRAPPER(APP, kthread_create, (struct task_struct *), (
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
#define APP_WRAPPER_FOR_struct_task_struct
#define APP_WRAPPER_FOR_struct_tracepoint
#define APP_WRAPPER_FOR_struct_module
#define APP_WRAPPER_FOR_struct_gendisk
#define APP_WRAPPER_FOR_struct_address
#define APP_WRAPPER_FOR_struct_sk_buff
#define APP_WRAPPER_FOR_struct_sk_buff_head
#define APP_WRAPPER_FOR_struct_sched_class
#define APP_WRAPPER_FOR_struct_nf_bridge_info
#define APP_WRAPPER_FOR_struct_sock_common
#define APP_WRAPPER_FOR_struct_sock
#define APP_WRAPPER_FOR_struct_ctl_table_set
#define APP_WRAPPER_FOR_struct_net
#define APP_WRAPPER_FOR_struct_dentry
#define APP_WRAPPER_FOR_struct_inode

#define APP_WRAPPER_FOR_struct_rb_node
#define APP_WRAPPER_FOR_struct_rb_root

#define APP_WRAPPER_FOR_struct_hlist_bl_head
#define APP_WRAPPER_FOR_struct_hlist_bl_node

#define APP_WRAPPER_FOR_struct_list_head
#define APP_WRAPPER_FOR_struct_list_node

#define APP_WRAPPER_FOR_struct_hlist_head
#define APP_WRAPPER_FOR_struct_hlist_node

#define APP_WRAPPER_FOR_struct_plist_head
#define APP_WRAPPER_FOR_struct_plist_node

#define APP_WRAPPER_FOR_struct_llist_head
#define APP_WRAPPER_FOR_struct_llist_node


/// Disable wrapping of sk buffs as they are used so frequently with network
/// drivers.
#define APP_WRAPPER_FOR_struct_sk_buff
#define APP_WRAPPER_FOR_struct_sk_buff_head

#if 1 == CONFIG_MAX_PRE_WRAP_DEPTH
#   define IF_WRAP_DEPTH_1(...) __VA_ARGS__
#else
#   define IF_WRAP_DEPTH_1(...)
#endif

/// Custom wrapping for net devices.
#ifndef APP_WRAPPER_FOR_struct_net_device
#   define APP_WRAPPER_FOR_struct_net_device
    struct granary_net_device : public net_device { };
    TYPE_WRAPPER(struct granary_net_device, {
        NO_PRE_IN
        PRE_OUT {

            // Go deep when we register the device.
            IF_WRAP_DEPTH_1( RELAX_WRAP_DEPTH; )
            RELAX_WRAP_DEPTH; RELAX_WRAP_DEPTH;

            PRE_OUT_WRAP(arg.dsa_ptr);
            PRE_OUT_WRAP(arg.ip_ptr);
            PRE_OUT_WRAP(arg.dn_ptr);
            PRE_OUT_WRAP(arg.ip6_ptr);
            PRE_OUT_WRAP(arg.ieee80211_ptr);
            PRE_OUT_WRAP(arg.queues_kset);
            PRE_OUT_WRAP(arg._rx);
            WRAP_FUNCTION(arg.rx_handler);
            PRE_OUT_WRAP(arg.ingress_queue);
            PRE_OUT_WRAP(arg._tx);
            PRE_OUT_WRAP(arg.qdisc);
            PRE_OUT_WRAP(arg.xps_maps);
            PRE_OUT_WRAP(arg.watchdog_timer);
            WRAP_FUNCTION(arg.destructor);
            PRE_OUT_WRAP(arg.npinfo);
            PRE_OUT_WRAP(arg.nd_net);
            PRE_OUT_WRAP(arg.dev);
            PRE_OUT_WRAP(arg.priomap);
            PRE_OUT_WRAP(arg.phydev);
            PRE_OUT_WRAP(arg.pm_qos_req);
        }
        NO_POST_IN
        POST_OUT {
            // Go deep when we register the device.
            RELAX_WRAP_DEPTH; RELAX_WRAP_DEPTH; RELAX_WRAP_DEPTH;

            PRE_OUT_WRAP(arg.wireless_handlers);
            PRE_OUT_WRAP(arg.netdev_ops);
            PRE_OUT_WRAP(arg.ethtool_ops);
            PRE_OUT_WRAP(arg.header_ops);
            PRE_OUT_WRAP(arg.rtnl_link_ops);
            PRE_OUT_WRAP(arg.dcbnl_ops);
        }
        NO_RETURN
    })
#endif


#if defined(CAN_WRAP_netif_napi_add) && CAN_WRAP_netif_napi_add && !defined(APP_WRAPPER_FOR_netif_napi_add)
#   define APP_WRAPPER_FOR_netif_napi_add
    FUNCTION_WRAPPER_VOID(APP, netif_napi_add, \
        (struct granary_net_device *dev, struct napi_struct *napi,
        int (*poll)(struct napi_struct *, int), int weight \
    ), {
        // Double-up on the wrapping of the device (pre & post).
        PRE_OUT_WRAP(dev);
        POST_OUT_WRAP(dev);

        WRAP_FUNCTION(poll);
        PRE_OUT_WRAP(napi);
        netif_napi_add(dev, napi, poll, weight);
        POST_OUT_WRAP(napi);
        POST_OUT_WRAP(dev);
    })
#endif


#if defined(CAN_WRAP_register_netdev) && CAN_WRAP_register_netdev && !defined(APP_WRAPPER_FOR_register_netdev)
#   define APP_WRAPPER_FOR_register_netdev
    FUNCTION_WRAPPER(APP, register_netdev, (int), (struct granary_net_device * dev), {
        PRE_OUT_WRAP(dev);
        int ret(register_netdev(dev));
        POST_OUT_WRAP(dev);
        return ret;
    })
#endif


#if defined(CAN_WRAP_register_netdevice) && CAN_WRAP_register_netdevice && !defined(APP_WRAPPER_FOR_register_netdevice)
#   define APP_WRAPPER_FOR_register_netdevice
    FUNCTION_WRAPPER(APP, register_netdevice, (int), (struct granary_net_device * dev), {
        PRE_OUT_WRAP(dev);
        int ret(register_netdev(dev));
        POST_OUT_WRAP(dev);
        return ret;
    })
#endif


/// Custom wrapping for super blocks.
#ifndef APP_WRAPPER_FOR_struct_super_block
#   define APP_WRAPPER_FOR_struct_super_block
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


#ifndef APP_WRAPPER_FOR_struct_address_space
#   define APP_WRAPPER_FOR_struct_address_space
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


#ifndef APP_WRAPPER_FOR_struct_file_system_type
#   define APP_WRAPPER_FOR_struct_file_system_type
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


#if defined(CAN_WRAP_iget_locked) && CAN_WRAP_iget_locked && !defined(APP_WRAPPER_FOR_iget_locked)
#   define APP_WRAPPER_FOR_iget_locked
    FUNCTION_WRAPPER(APP, iget_locked, (struct inode *), (struct granary_super_block *sb, unsigned long ino), {
        IF_WRAP_DEPTH_1( RELAX_WRAP_DEPTH; )
        PRE_OUT_WRAP(sb);
        inode *inode(iget_locked(sb, ino));
        PRE_OUT_WRAP(inode->i_mapping);
        return inode;
    })
#endif


#if defined(CAN_WRAP_unlock_new_inode) && CAN_WRAP_unlock_new_inode && !defined(APP_WRAPPER_FOR_unlock_new_inode)
#   define APP_WRAPPER_FOR_unlock_new_inode
    FUNCTION_WRAPPER(APP, unlock_new_inode, (void), (struct inode *inode), {
        IF_WRAP_DEPTH_1( RELAX_WRAP_DEPTH; )
        PRE_OUT_WRAP(inode->i_op);
        PRE_OUT_WRAP(inode->i_fop);
        PRE_OUT_WRAP(inode->i_mapping);
        unlock_new_inode(inode);
    })
#endif


/// Mount a block device. Make sure that the super block is wrapped when the
/// dynamic wrapper for `fill_super`
#if defined(CAN_WRAP_mount_bdev) && CAN_WRAP_mount_bdev && !defined(APP_WRAPPER_FOR_mount_bdev)
#   define APP_WRAPPER_FOR_mount_bdev
    FUNCTION_WRAPPER(APP, mount_bdev, (struct dentry *), (
        struct file_system_type * fs_type ,
        int flags ,
        const char *dev_name ,
        void *data ,
        int (*fill_super)(struct super_block *, void *, int)
    ), {
        IF_WRAP_DEPTH_1( RELAX_WRAP_DEPTH; )
        PRE_OUT_WRAP(fs_type);

        typedef int (granary_fill_super_t)(granary_super_block *, void *, int);

        granary_fill_super_t *&fill_super_((granary_fill_super_t *&) fill_super);
        WRAP_FUNCTION(fill_super);

        struct dentry *ret(mount_bdev(fs_type, flags, dev_name, data, fill_super));
        RETURN_IN_WRAP(ret);
        return ret;
    })
#endif


/// Hot-patch the `process_one_work` function to wrap `work_struct`s before
/// their functions are executed. This is here because there are ways to get
/// work structs registered through macros/inline functions, thus bypassing
/// wrapping.
#if defined(DETACH_ADDR_process_one_work)
    PATCH_WRAPPER_VOID(process_one_work, (struct worker *worker, struct work_struct *work), {
        PRE_OUT_WRAP(work);
        process_one_work(worker, work);
    })
#endif


/// Appears to be used for swapping the user-space GS register value instead of
/// using WRMSR. The benefit of the approach appears to be that there is very
/// little possibility for exceptions using a SWAPGS in this function, whereas
/// WRMSR has a few more exception possibilities.
#if defined(DETACH_ADDR_native_load_gs_index)
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR_native_load_gs_index);
#endif


#endif /* granary_KERNEL_OVERRIDE_WRAPPERS_H_ */
