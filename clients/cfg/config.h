/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * config.h
 *
 *  Created on: 2013-11-21
 *      Author: Peter Goodman
 */

#ifndef CFG_CONFIG_H_
#define CFG_CONFIG_H_


/// Should we record basic block execution counts?
#define CFG_RECORD_EXEC_COUNT 1


/// If we're recording basic block execution counts, then should we also count
/// the number of times that a conditional branch is not taken (i.e. we fall
/// through to the next basic block).
#define CFG_RECORD_FALL_THROUGH_COUNT 1


/// Should we record indirect branch targets?
#define CFG_RECORD_INDIRECT_TARGETS 0

#endif /* CFG_CONFIG_H_ */
