/*
 * types.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_TYPES_H_
#define granary_TYPES_H_


/// Types are placed within a namespace so that they don't clobber any existing
/// global types, thus causing compiler warnings/errors.
#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
namespace granary { namespace types {
#endif

extern "C" {

#define restrict __restrict__

#if GRANARY_IN_KERNEL
#   include "granary/gen/kernel_types.h"
#else
// OS X-specific hack.
#   ifdef __APPLE__
typedef wchar_t __darwin_wchar_t;
#   endif
#   define _Bool bool
#   include "granary/gen/user_types.h"
#endif
#undef restrict
#undef wchar_t
}

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
}}
#endif

#endif /* granary_TYPES_H_ */
