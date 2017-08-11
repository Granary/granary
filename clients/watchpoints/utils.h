/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * utils.h
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#ifndef CLIENT_WP_UTILS_H_
#define CLIENT_WP_UTILS_H_

#include "clients/watchpoints/instrument.h"

namespace client { namespace wp {

    /// Convert a register into an index between [0, 14].
    unsigned register_to_index(dynamorio::reg_id_t) ;


    /// Convert an operand size into an integer `i` such that `2^i` is the
    /// number of bytes required to represent an operand of that size.
    unsigned operand_size_order(operand_size) ;
}}

#endif /* CLIENT_WP_UTILS_H_ */
