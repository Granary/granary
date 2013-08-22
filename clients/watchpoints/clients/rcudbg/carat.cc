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


    static std::atomic<unsigned> NEXT_SECTION_ID = ATOMIC_VAR_INIT(1);


    struct section_info {
        const char *carat;
        const void *thread;
    };


    static section_info SECTIONS[MAX_NUM_SECTION_IDS] = {{nullptr, nullptr}};


    static granary::static_data<
        granary::atomic_id_set<MAX_NUM_LOCATION_IDS, const char *>
    > LOCATION_IDS;


    STATIC_INITIALISE_ID(rcudbg_location_carats, {
        LOCATION_IDS.construct(nullptr, reinterpret_cast<const char *>(~0ULL));
    });


    /// Returns a unique ID for a string carat location. This unique ID will fit
    /// into 14 bits, and so it's suitable for use in watched addresses.
    unsigned get_location_id(const char *carat_) throw() {
        return LOCATION_IDS->add(carat_);
    }


    /// Gets a location carat given a location id.
    const char *get_location_carat(unsigned location_id) throw() {
        return LOCATION_IDS->get(location_id);
    }


    /// Gets a section ID for a given carat and thread.
    unsigned allocate_section_id(
        const char *carat,
        const void *thread,
        const unsigned conflict_id
    ) throw() {
        unsigned id(0);

        for(;;) {
            id = NEXT_SECTION_ID.fetch_add(1) % MAX_NUM_SECTION_IDS;
            if(conflict_id == id) {
                continue;
            }

            // This is error-prone but whatever.
            section_info &section(SECTIONS[id]);
            if(!section.thread) {
                section.thread = thread;
                section.carat = carat;
                std::atomic_thread_fence(std::memory_order_seq_cst);
                break;
            }
        }
        return id;
    }


    /// Gets the last carat assigned to a particular section id. This won't
    /// necessarily get the right carat, but it will usually get one that's
    /// close enough.
    const char *get_section_carat(unsigned section_id) throw() {
        return SECTIONS[section_id % MAX_NUM_SECTION_IDS].carat;
    }
}

