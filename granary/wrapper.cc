/*
 * wrapper.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */


/// Prevent globals.h and others from including (parts of) the C standard
/// library.
#define GRANARY_DONT_INCLUDE_CSTDLIB


/// The generated types. This is either the functions/types of libc or the
/// functions/types of the kernel. This is included *before* detach.h (which
/// includes globals.h) so that any prototypes depending on things like
/// stdint.h will still be resolved.
#include "granary/types.h"


/// Prototypes of exported symbols.
#include "granary/detach.h"


/// Auto-generated file that assigns unique IDs to each function.
#include "granary/gen/detach.h"


/// Wrapper templates.
#include "granary/wrapper.h"


/// Wrappers (potentially auto-generated) that are specialized to specific
/// functions (by means of IDs) or to types, all contained in types.h.
#if GRANARY_IN_KERNEL
#   include "granary/gen/kernel_wrappers.h"
#else
#   include "granary/gen/user_wrappers.h"
#endif


/// Auto-generated table of all detachable functions and their wrapper
/// instantiations. These depend on the partial specializations from
/// user/kernel_wrappers.h.
#include "granary/gen/detach.cc"

