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


    /// Gets a section ID for a given carat and task.
    unsigned get_section_id(const char *lock_carat_, void *task) throw();


    /// Gets the last carat assigned to a particular section id.
    const char *get_section_carat(unsigned section_id) throw();
}


#endif /* RCUDBG_CARAT_H_ */
