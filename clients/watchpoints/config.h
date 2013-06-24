/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * config.h
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_CONFIG_H_
#define WATCHPOINT_CONFIG_H_


/// Enable if %RBP should be treated as a frame pointer and not as a potential
/// watched address. The upside of this is that it's likely that more code
/// (especially leaf functions) will behave correctly. The downside is that
/// a lot of code will have extra, potentially unneeded instrumentation,
/// especially where frame pointers are concerned.
#define WP_IGNORE_FRAME_POINTER 0


/// Check for user space addresses. This adds an extra two instructions to
/// kernel-mode instrumentation and can be a useful debugging aid when trying
/// to see if an instrumentation error might be caused by the presence of a
/// user space address.
#define WP_CHECK_FOR_USER_ADDRESS 1


/// Size (in bits) of the counter index. This should either be 16.
///
/// Note: The lowest order bit of the counter index is reserved for detecting
///       if an address is watched or not.
#define WP_COUNTER_INDEX_WIDTH 16


/// Enable if inherited indexes should be used.
#define WP_USE_INHERITED_INDEX 0


/// Size (in bits) of the inherited index. This should be a small, non-negative
/// number that complement the counter index with (minus 1), so as to get the
/// desired space of objects.
#define WP_INHERITED_INDEX_WIDTH 5


/// The granularity of the inherited index. This depends on the expected size of
/// watched objects. A lower number implicitly means that objects are expected
/// to be smaller. A higher number (in the range of 14 or more) says that
/// objects are expected to be bigger.
///
/// Note: A key concern is that objects shouldn't spill across two granularity-
///       defined areas. E.g. if the granularity is 8, then each zone is 256
///       bytes wide. If an object spaces bytes [250, 260], then it will be
///       across two zones and resolve to the incorrect descriptor.
///
/// Note: One approach that deals with the above issues is duplicating a
///       descriptor entry across adjacent zones. This is possible because
///       inherited indexes make the descriptor table (if used) more sparse.
#define WP_INHERITED_INDEX_GRANULARITY 14


/// Backup for if the inherited index width is set to 0 but the inherited indexes
/// are left enabled. Don't change.
#if !WP_INHERITED_INDEX_WIDTH
#   undef WP_USE_INHERITED_INDEX
#   define WP_USE_INHERITED_INDEX 0
#endif


/// Backup for if the inherited index is disabled, but the inherited index width is
/// left as non-zero. Don't change.
#if !WP_USE_INHERITED_INDEX
#   undef WP_INHERITED_INDEX_WIDTH
#   define WP_INHERITED_INDEX_WIDTH 0
#endif


/// Feature toggler. Don't change.
#if WP_USE_INHERITED_INDEX
#   define IF_INHERITED_INDEX(...) __VA_ARGS__
#else
#   define IF_INHERITED_INDEX(...)
#endif


/// Double check the settings.
#if 16 != WP_COUNTER_INDEX_WIDTH
#   error "The counter index width must be 16 for kernel-space compatibility."
#endif


/// Left shift for inherited indexes to get the inherited index into the high
/// position.
#define WP_INHERITED_INDEX_LSH \
    (64 - (WP_INHERITED_INDEX_WIDTH + WP_INHERITED_INDEX_GRANULARITY))


/// Right shift for inherited indexes to get it into the low position.
#define WP_INHERITED_INDEX_RSH (64 - WP_INHERITED_INDEX_WIDTH)


/// Right shift for counter indexes to get it into the low position.
#define WP_COUNTER_INDEX_RSH (64 - (WP_COUNTER_INDEX_WIDTH - 1))


#endif /* WATCHPOINT_CONFIG_H_ */
