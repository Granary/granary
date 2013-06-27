/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * init.h
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#ifndef GR_INIT_H_
#define GR_INIT_H_

namespace granary {

    /// A list of static initialisers.
    struct static_init_list {
        static_init_list *next;
        void (*exec)(void);

        static void append(static_init_list &) throw();
        IF_KERNEL( static void append_sync(static_init_list &) throw(); )
    };


    void init(void) throw();

    IF_KERNEL( bool should_init_sync(void) throw(); )
    IF_KERNEL( void init_sync(void) throw(); )
}

#endif /* GR_INIT_H_ */
