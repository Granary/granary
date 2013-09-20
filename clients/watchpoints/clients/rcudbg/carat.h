/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * carat.h
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#ifndef RCUDBG_CARAT_H_
#define RCUDBG_CARAT_H_

namespace client {

    /// Returns a unique ID for a string carat location. This unique ID will fit
    /// into 14 bits, and so it's suitable for use in watched addresses.
    unsigned get_location_id(const char *assign_carat_) throw();


    /// Gets a location carat given a location id.
    const char *get_location_carat(unsigned location_id) throw();


    /// Gets a section ID for a given carat and thread.
    unsigned allocate_section_id(
        const char *carat,
        const void *thread,
        const unsigned conflict_id
    ) throw();

    enum section_carat_kind {
        SECTION_LOCK_CARAT,
        SECTION_UNLOCK_CARAT,
        SECTION_DEREF_CARAT
    };

    /// Gets the last carat assigned to a particular section id.
    const char *get_section_carat(
        unsigned section_id,
        section_carat_kind kind
    ) throw();


    /// Gets the last carat assigned to a particular section id. This won't
    /// necessarily get the right carat, but it will usually get one that's
    /// close enough.
    void set_section_carat(
        unsigned section_id,
        section_carat_kind kind,
        const char *carat
    ) throw();
}


#endif /* RCUDBG_CARAT_H_ */
