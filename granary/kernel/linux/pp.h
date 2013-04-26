/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * pp.h
 *
 *  Created on: 2013-04-10
 *      Author: pag
 */

#ifndef PP_H_
#define PP_H_

#ifndef __KERNEL__
#   define __KERNEL__
#endif

#include <linux/version.h>

#ifndef LINUX_MAJOR_VERSION
#   define LINUX_MAJOR_VERSION ((LINUX_VERSION_CODE >> 16) & 0xFF)
#   define LINUX_MINOR_VERSION ((LINUX_VERSION_CODE >> 8)  & 0xFF)
#   define LINUX_PATCH_VERSION ((LINUX_VERSION_CODE >> 0)  & 0xFF)
#endif

#define LINUX_VERSION_CODE_FOR(major, minor, patch) KERNEL_VERSION(major, minor, patch)

#endif /* PP_H_ */
