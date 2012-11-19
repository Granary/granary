/*
 * types.h
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */

#ifndef GR_KERNEL_TYPES_H_
#define GR_KERNEL_TYPES_H_

#include <linux/version.h>
#define LINUX_MAJOR_VERSION ((LINUX_VERSION_CODE >> 16) & 0xFF)
#define LINUX_MINOR_VERSION ((LINUX_VERSION_CODE >> 8)  & 0xFF)
#define LINUX_PATCH_VERSION ((LINUX_VERSION_CODE >> 0)  & 0xFF)

#ifdef __cplusplus
namespace kernel {
extern "C" {
#endif

#if LINUX_MAJOR_VERSION == 3 && LINUX_MINOR_VERSION == 5
#	include "granary/kernel/types/3.5.0/kernel.h"
#elif LINUX_MAJOR_VERSION == 2 && LINUX_MINOR_VERSION == 6 && LINUX_PATCH_VERSION == 33
#	include "granary/kernel/types/2.6.33/kernel.h"
#endif

#ifdef __cplusplus
} /* extern */
} /* kernel namespace */
#endif

#endif /* GR_KERNEL_TYPES_H_ */
