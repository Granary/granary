/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * location.cc
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */


#include "granary/client.h"
#include "clients/watchpoints/clients/rcudbg/carat.h"

namespace client {


    enum {
        MAX_NUM_LOCATION_IDS = 1ULL << 14,
        MAX_NUM_SECTION_IDS = MAX_NUM_LOCATION_IDS
    };


    static granary::static_data<
        granary::atomic_id_set<MAX_NUM_LOCATION_IDS, const char *>
    > LOCATION_CARATS;


    STATIC_INITIALISE_ID(rcudbg_location_carats, {
        LOCATION_CARATS.construct(reinterpret_cast<const char *>(~0ULL));
    });


    /// Returns a unique ID for a string carat location. This unique ID will fit
    /// into 14 bits, and so it's suitable for use in watched addresses.
    unsigned get_location_id(const char *carat_) throw() {
        return LOCATION_CARATS->add(carat_);
    }


    /// Gets a location carat given a location id.
    const char *get_location_carat(unsigned location_id) throw() {
        return LOCATION_CARATS->get(location_id);
    }


    /// Gets a section ID for a given carat and task.
    uintptr_t get_section_id(const char *lock_carat_, void *task) throw() {
        return 0; // TODO
    }


    /// Gets the last carat assigned to a particular section id.
    const char *get_section_carat(uintptr_t section_id) throw() {
        return nullptr; // TODO
    }
}

