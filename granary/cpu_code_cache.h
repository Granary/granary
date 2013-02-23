/*
 * cpu_code_cache.h
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_CPU_CODE_CACHE_H_
#define granary_CPU_CODE_CACHE_H_

#include "granary/globals.h"

namespace granary {


    /// Entry in the code cache.
    struct cpu_private_code_cache_entry {
    public:
        app_pc source;
        app_pc dest;
    };


    /// Represents a simple hash table for a CPU-private code cache. This hash
    /// table is specific to app addresses, but maintains the same public
    /// interface as other Granary hash tables.
    struct cpu_private_code_cache {
    public:

        /// Mask on the entries.
        uint64_t bit_mask;

        /// Array of (key, value) pairs.
        cpu_private_code_cache_entry *entries;

        /// Are we currently growing this hash table?
        bool growing;

        /// Find the value associated with a key in the hash table.
        __attribute__((hot, optimize("O3")))
        app_pc find(const app_pc key) const throw();

        /// Search for an entry in the hash table.
        inline bool load(const app_pc key, app_pc &value) const throw() {
            value = find(key);
            return nullptr != value;
        }

        /// Store a value in the hash table. Returns true iff the entry was
        /// written to the hash table.
        bool store(
            app_pc key,
            app_pc value,
            hash_store_policy update=HASH_OVERWRITE_PREV_ENTRY
        ) throw();

    private:

        /// Insert an entry into the hash table. Returns true iff an element
        /// with the key didn't previously exist in the hash table.
        hash_store_state insert(
            app_pc key,
            app_pc value,
            bool update
        ) throw();

        /// Grow the hash table. This increases the hash table's size by two.
        void grow(void) throw();

    } __attribute__((packed));

}

#endif /* granary_CPU_CODE_CACHE_H_ */
