/*
 * hash_table.h
 *
 *  Created on: 2012-11-27
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_HASH_TABLE_H_
#define Granary_HASH_TABLE_H_

#include <new>
#include <atomic>

#include "granary/pp.h"
#include "granary/allocator.h"

#include "deps/murmurhash/murmurhash.h"

#if !GRANARY_IN_KERNEL
#include <map>

namespace granary {

#if 0
    template <typename K, typename V>
    struct hash_table {
        std::map<K, V> map;

        bool load(K key, V *val) const throw() {
            auto res(map.find(key));
            if(res->first == key) {
                *val = res->second;
                return true;
            }

            return false;
        }

        void store(K key, V val) throw() {
            map[key] = val;
        }
    };
#endif
}
#endif

#endif /* Granary_HASH_TABLE_H_ */
