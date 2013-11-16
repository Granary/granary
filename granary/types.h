/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * types.h
 *
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

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wshadow"
#   pragma clang diagnostic ignored "-Wunused-variable"
#elif defined(GCC_VERSION) || defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wshadow"
#   pragma GCC diagnostic ignored "-Wunused-variable"
#   pragma GCC diagnostic ignored "-Wunused-function"
#   pragma GCC diagnostic ignored "-fpermissive"
#else
#   error "Can't disable compiler warnings around `(user/kernel)_types.h` include."
#endif


#if CONFIG_ENV_KERNEL

    /* Big hack: the kernel has it's own definition for bool, which we macro'd
     * out in kernel/linux/types.h.
     */
    typedef bool K_Bool;

    /* Big hack: we remove inline functions, and these two are called by some
     * enumerator constants. The constants don't appear to be used in any
     * significant way within the kernel types though.
     */
#   define __fswab32(...) __VA_ARGS__
#   define __fswab64(...) __VA_ARGS__
#   define __fswab16(...) __VA_ARGS__

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
#endif

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(GCC_VERSION) || defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

}

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
}}
#endif

#endif /* granary_TYPES_H_ */
