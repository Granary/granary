/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * location.h
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#ifndef RCUDBG_LOCATION_H_
#define RCUDBG_LOCATION_H_

namespace client {

   /// Returns a unique ID for a string carat location. This unique ID will fit
   /// into 14 bits, and so it's suitable for use in watched addresses.
   static uint64_t get_location_id(const char *assign_carat_) throw();


   /// Gets a location carat given a location id.
  const char *get_location_carat(uintptr_t location_id) throw();

}


#endif /* RCUDBG_LOCATION_H_ */
