/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * watched_policy.cc
 *
 *  Created on: 2013-05-15
 *      Author: Peter Goodman
 */

#include "granary/globals.h"

#include "clients/watchpoints/policies/watched_policy.h"

using namespace granary;

extern "C" {
    extern int __get_user_1 ( void ) ;
    extern int __get_user_2 ( void ) ;
    extern int __get_user_4 ( void ) ;
    extern int __get_user_8 ( void ) ;
    extern void __put_user_1 ( void ) ;
    extern void __put_user_2 ( void ) ;
    extern void __put_user_4 ( void ) ;
    extern void __put_user_8 ( void ) ;
}

GRANARY_DETACH_POINT(__get_user_1)
GRANARY_DETACH_POINT(__get_user_2)
GRANARY_DETACH_POINT(__get_user_4)
GRANARY_DETACH_POINT(__get_user_8)

GRANARY_DETACH_POINT(__put_user_1)
GRANARY_DETACH_POINT(__put_user_2)
GRANARY_DETACH_POINT(__put_user_4)
GRANARY_DETACH_POINT(__put_user_8)

