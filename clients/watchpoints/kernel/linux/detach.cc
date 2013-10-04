/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * watched_policy.cc
 *
 *  Created on: 2013-05-15
 *      Author: Peter Goodman
 */

#include "granary/client.h"

using namespace granary;

#ifdef DETACH_ADDR___get_user_1
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR___get_user_1)
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR___get_user_2)
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR___get_user_4)
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR___get_user_8)

    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR___put_user_1)
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR___put_user_2)
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR___put_user_4)
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR___put_user_8)
#endif

#ifdef DETACH_ADDR___clear_user
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR___clear_user)
#endif
