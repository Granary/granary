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


/// Size (in bits) of the counter index. This should either be 16.
///
/// Note: The lowest order bit of the counter index is reserved for detecting
///       if an address is watched or not.
#define WP_COUNTER_INDEX_WIDTH 16


/// Enable if partial indexes should be used.
#define WP_USE_PARTIAL_INDEX 0


/// Size (in bits) of the partial index. This should be a small, non-negative
/// number that complement the counter index with (minus 1), so as to get the
/// desired space of objects.
#define WP_PARTIAL_INDEX_WIDTH 5


/// The granularity of the partial index. This depends on the expected size of
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
///       partial indexes make the descriptor table (if used) more sparse.
#define WP_PARTIAL_INDEX_GRANULARITY 14


/// Backup for if the partial index width is set to 0 but the partial indexes
/// are left enabled. Don't change.
#if !WP_PARTIAL_INDEX_WIDTH
#   undef WP_USE_PARTIAL_INDEX
#   define WP_USE_PARTIAL_INDEX 0
#endif


/// Backup for if the partial index is disabled, but the partial index width is
/// left as non-zero. Don't change.
#if !WP_USE_PARTIAL_INDEX
#   undef WP_PARTIAL_INDEX_WIDTH
#   define WP_PARTIAL_INDEX_WIDTH 0
#endif


/// Feature toggler. Don't change.
#if WP_USE_PARTIAL_INDEX
#   define IF_PARTIAL_INDEX(...) __VA_ARGS__
#else
#   define IF_PARTIAL_INDEX(...)
#endif


/// Double check the settings.
#if 16 != WP_COUNTER_INDEX_WIDTH
#   error "The counter index width must be 16 for kernel-space compatibility."
#endif

#endif /* WATCHPOINT_CONFIG_H_ */
