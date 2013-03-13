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
#include <asm/pvclock.h>


#endif /* GR_KERNEL_TYPES_H_ */
