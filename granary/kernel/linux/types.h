/*
 * types.h
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */

#ifndef GR_KERNEL_TYPES_H_
#define GR_KERNEL_TYPES_H_

#ifdef GRANARY
#   error "This file should not be included directly."
#endif

#include "granary/gen/kernel_macros.h"

#define new new_
#define true true_
#define false false_
#define private private_
#define namespace namespace_
#define template template_
#define class class_
#define delete delete_
#define export export_
#define typeof decltype
#define this this_

#define int8_t K_int8_t
#define int16_t K_int16_t
#define int32_t K_int32_t
#define int64_t K_int64_t

#define uint8_t K_uint8_t
#define uint16_t K_uint16_t
#define uint32_t K_uint32_t
#define uint64_t K_uint64_t

#define bool K_bool
#define _Bool K_Bool

/* Big hack: clang complains when a (named) struct is declared inside of an
 * anonymous union. There is one such case: __raw_tickets, and it's not
 * referenced by other types, so we will clobber it.
 */
#define __raw_tickets

#include <linux/version.h>
#define LINUX_MAJOR_VERSION ((LINUX_VERSION_CODE >> 16) & 0xFF)
#define LINUX_MINOR_VERSION ((LINUX_VERSION_CODE >> 8)  & 0xFF)
#define LINUX_PATCH_VERSION ((LINUX_VERSION_CODE >> 0)  & 0xFF)


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/tick.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <asm/pvclock.h>

/* Taken from e1000 */
#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/if_vlan.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/string.h>
#include <linux/firmware.h>
#include <linux/rtnetlink.h>
#include <asm/unaligned.h>

/* Also taken from e1000 */

#include <linux/stddef.h>
#include <linux/module.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/pagemap.h>
#include <linux/dma-mapping.h>
#include <linux/bitops.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/capability.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/pkt_sched.h>
#include <linux/list.h>
#include <linux/reboot.h>
#include <net/checksum.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>

/* Taken from ext4 */
#include <linux/types.h>
#include <linux/blkdev.h>
#include <linux/magic.h>
#include <linux/jbd2.h>
#include <linux/quota.h>
#include <linux/rwsem.h>
#include <linux/rbtree.h>
#include <linux/seqlock.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/blockgroup_lock.h>
#include <linux/percpu_counter.h>
#include <crypto/hash.h>
#ifdef __KERNEL__
#include <linux/compat.h>
#endif

/* Taken from btrfs */
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/mount.h>
#include <linux/mpage.h>
#include <linux/swap.h>
#include <linux/writeback.h>
#include <linux/statfs.h>
#include <linux/compat.h>
#include <linux/parser.h>
#include <linux/ctype.h>
#include <linux/namei.h>
#include <linux/miscdevice.h>
#include <linux/magic.h>
#include <linux/slab.h>
#if LINUX_MAJOR_VERSION >= 3
#   include <linux/cleancache.h>
#endif
#include <linux/ratelimit.h>

/* Taken from ramfs */
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/ramfs.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

/* for __fswab32 */
#include <linux/swab.h>


#endif /* GR_KERNEL_TYPES_H_ */
