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
#   pragma GCC diagnostic ignored "-fpermissive"
#   pragma GCC diagnostic ignored "-Wunused-variable"
#elif defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wshadow"
#   pragma clang diagnostic ignored "-fpermissive"
#   pragma clang diagnostic ignored "-Wunused-variable"
#else
#   error "Can't disable `-Wshadow` around `(user/kernel)_types.h` include."
#endif


#if GRANARY_IN_KERNEL
    typedef bool K_Bool;
#   include "granary/gen/kernel_types.h"
#else
#   define _Bool bool
// OS X-specific hack.
#   ifdef __APPLE__
        typedef wchar_t __darwin_wchar_t;
#   endif
#   define restrict
#   define __restrict
#   include "granary/gen/user_types.h"
#   undef restrict
#   undef __restrict
#   endif


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
