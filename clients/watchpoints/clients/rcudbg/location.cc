/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * location.cc
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#include <stdint.h>
#include <atomic>

namespace client {

   enum {
       MAX_NUM_LOCATION_IDS = 1ULL << 14
   };


   std::atomic<const char *> CARAT_TO_LOCATION_ID[MAX_NUM_LOCATION_IDS] = {
       ATOMIC_VAR_INIT(nullptr)
   };


   /// Returns a unique ID for a string carat location. This unique ID will fit
   /// into 14 bits, and so it's suitable for use in watched addresses.
   static uint64_t get_location_id(const char *carat_) throw() {
       const uintptr_t assign_carat(
           reinterpret_cast<uintptr_t>(carat_));

       uintptr_t index;
       uintptr_t next_index(assign_carat % MAX_NUM_LOCATION_IDS);
       const char *found_carat(nullptr);

       do {
           index = next_index;
           next_index = (index + 1) % MAX_NUM_LOCATION_IDS;

           found_carat = CARAT_TO_LOCATION_ID[index].load(
               std::memory_order_relaxed);

           if(!found_carat) {
               bool updated(CARAT_TO_LOCATION_ID[index].compare_exchange_weak(
                   found_carat,
                   carat_));

               if(updated) {
                   found_carat = carat_;
               }
           }
       } while(found_carat != carat_);

       return index;
   }


   /// Gets a location carat given a location id.
   const char *get_location_carat(uintptr_t location_id) throw() {
       return CARAT_TO_LOCATION_ID[location_id % MAX_NUM_LOCATION_IDS].load();
   }

}

