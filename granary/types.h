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

#if defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wshadow"
#elif defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wshadow"
#else
#   error "Can't disable `-Wshadow` around `(user/kernel)_types.h` include."
#endif

#define restrict
#define __restrict
#define _Bool bool

#   if GRANARY_IN_KERNEL
#      include "granary/gen/kernel_types.h"
#   else
// OS X-specific hack.
#      ifdef __APPLE__
typedef wchar_t __darwin_wchar_t;
#      endif
#      include "granary/gen/user_types.h"
#   endif
#   undef restrict
#   undef wchar_t

#if defined(GCC_VERSION)
#   pragma GCC diagnostic pop
#elif defined(__clang__)
#   pragma clang diagnostic pop
#endif

}

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
}}
#endif

#endif /* granary_TYPES_H_ */
