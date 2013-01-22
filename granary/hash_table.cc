/*
 * hash_table.cc
 *
 *  Created on: 2013-01-21
 *      Author: pag
 */

#include "granary/hash_table.h"

namespace granary {

    /// Load factor for determining when the hash table should grow.
    static constexpr double LOAD_FACTOR = 0.75;

    /// Find the overloaded size
    static constexpr uint32_t load_for(const uint32_t size) {
        return static_cast<uint32_t>(
            static_cast<double>(size) * LOAD_FACTOR);
    }

    /// Calculate the maximum load for each size of the hash table, given
    /// the above load factor, and knowing that the hash table size will
    /// double.
    uint32_t HASH_TABLE_MAX_SIZES[] = {
        load_for(1 << 0),   load_for(1 << 1),   load_for(1 << 2),
        load_for(1 << 3),   load_for(1 << 4),   load_for(1 << 5),
        load_for(1 << 6),   load_for(1 << 7),   load_for(1 << 8),
        load_for(1 << 9),   load_for(1 << 10),  load_for(1 << 11),
        load_for(1 << 12),  load_for(1 << 13),  load_for(1 << 14),
        load_for(1 << 15),  load_for(1 << 16),  load_for(1 << 17),
        load_for(1 << 18),  load_for(1 << 19),  load_for(1 << 20),
        load_for(1 << 21),  load_for(1 << 22),  load_for(1 << 23),
        load_for(1 << 24),  load_for(1 << 25),  load_for(1 << 26),
        load_for(1 << 27),  load_for(1 << 28),  load_for(1 << 29),
        load_for(1 << 30),  load_for(1 << 31)
    };
}
