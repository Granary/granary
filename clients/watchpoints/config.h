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
/// watched address.
#define WP_IGNORE_FRAME_POINTER 1


/// Enable if partial indexes should be used (bit [15, 20]).
#define WP_USE_PARTIAL_INDEX 0


#endif /* WATCHPOINT_CONFIG_H_ */
