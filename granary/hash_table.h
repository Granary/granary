/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * hash_table.h
 *
 *  Created on: 2013-01-20
 *      Author: pag
 */

#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include "granary/globals.h"
#include "granary/smp/spin_lock.h"

#include "deps/murmurhash/murmurhash.h"

namespace granary {


    /// The maximum sizes of hash tables as different scaling factors.
    extern uint32_t HASH_TABLE_MAX_SIZES[];


    /// Default implementation of meta information for hash table entries.
    template <typename K, typename V>
    struct hash_table_meta {
    public:

        enum {
            DEFAULT_SCALE_FACTOR = 4U,
            MAX_SCAN_SCALE_FACTOR = 16U
        };

        inline static uint32_t hash(const K &key) throw() {
            enum {
                HASH_SEED = 0xDEADBEEFU,
            };
            uint32_t hash_value(0);
            MurmurHash3_x86_32(&key, sizeof key, HASH_SEED, &hash_value);
            return hash_value;
        }
    };

    namespace detail {

        /// Represents an entry in a hash table.
        template <typename K, typename V, bool is_shared>
        struct hash_table_entry;


        /// Shared version of a hash table entry.
        template <typename K, typename V>
        struct hash_table_entry<K, V, true> {
        public:
            std::atomic<bool> is_valid;
            std::atomic<K> key;
            std::atomic<V> value;
        };


        /// Private/unshared version of a hash table entry.
        template <typename K, typename V>
        struct hash_table_entry<K, V, false> {
        public:
            K key;
            V value;
        };


        /// Represents a very large array containing the entry slots of a hash
        /// table.
        template <typename K, typename V, bool is_shared>
        struct hash_table_entry_slots {
        public:

            /// Bitmask used for masking a hash so that it brings it in range
            /// the entries of the table.
            uint32_t mask;

            /// The entry slots of the table. Linear probing is used to deal
            /// with conflicts.
            hash_table_entry<K, V, is_shared> entries[1]; // VLA
        };


        template <typename K, typename V, bool is_shared>
        struct hash_table_impl;


        /// Hash table implementation.
        template <typename K, typename V>
        struct hash_table_impl<K, V, false> {
        public:

            typedef hash_table_entry<K, V, false> entry_type;
            typedef hash_table_entry_slots<K, V, false> slot_type;
            typedef hash_table_meta<K, V> meta_type;

            /// Number of entries stored in the hash table.
            uint32_t num_entries;

            /// Track the scaling factor.
            uint32_t scaling_factor;

            /// The entry slots represent the thing that can change when shared.
            slot_type *entry_slots;
        };


        template <typename K, typename V>
        struct hash_table_impl<K, V, true> {
        public:

            typedef hash_table_entry<K, V, true> entry_type;
            typedef hash_table_entry_slots<K, V, true> slot_type;
            typedef hash_table_meta<K, V> meta_type;

            /// Number of entries stored in the hash table.
            uint32_t num_entries;

            /// Track the scaling factor.
            uint32_t scaling_factor;

            /// The entry slots represent the thing that can change when shared.
            slot_type *entry_slots;

            /// Should the table be grown?
            bool should_grow;
        };
    }


    /// Basic, non-shared hash table.
    template <
        typename K,
        typename V,
        typename meta_type=hash_table_meta<K,V>
    >
    struct hash_table {
    private:

        typedef detail::hash_table_impl<K, V, false> table_type;
        typedef detail::hash_table_entry_slots<K, V, false> slots_type;
        typedef detail::hash_table_entry<K, V, false> entry_type;

        table_type table_;
        K default_key_;
        bool growing_;

        /// Insert an entry into the hash table. Returns true iff an element
        /// with the key didn't previously exist in the hash table.
        hash_store_state insert(
            K key,
            V value,
            bool update
        ) throw() {
            entry_type *slots(&(table_.entry_slots->entries[0]));
            const uint32_t mask(table_.entry_slots->mask);
            uint32_t entry_base(meta_type::hash(key));
            unsigned scan(0);
            hash_store_state state(HASH_ENTRY_SKIPPED);

            for(;; ++scan, entry_base += 1) {
                entry_base &= mask;
                entry_type &entry(slots[entry_base]);

                // insert position
                if(default_key_ == entry.key) {
                    entry.value = value;
                    entry.key = key;
                    state = HASH_ENTRY_STORED_NEW;
                    break;

                }

                // already inserted
                if(entry.key == key) {
                    if(update) {
                        entry.value = value;
                        state = HASH_ENTRY_STORED_OVERWRITE;
                    }
                    break;
                }
            }

            // resize if we needed to scan too far to insert this element.
            if(meta_type::MAX_SCAN_SCALE_FACTOR < scan && !growing_) {
                growing_ = true;
                grow();
                growing_ = false;
            }

            return state;
        }

        /// Grow the hash table. This increases the hash table's size by two.
        void grow(void) throw() {
            slots_type *old_slots(table_.entry_slots);
            const uint32_t num_old_slots(old_slots->mask + 1U);
            const uint32_t num_new_slots(num_old_slots * 2);

            slots_type *new_slots(
                new_trailing_vla<slots_type, entry_type>(num_new_slots));

            // update the table
            new_slots->mask = num_new_slots - 1U;
            table_.entry_slots = new_slots;
            table_.scaling_factor += 1;

            // transfer elements to the new slots
            entry_type *old_entries(&(old_slots->entries[0]));
            for(uint32_t i(0); i < num_old_slots; ++i) {
                entry_type &entry_old(old_entries[i]);
                if(default_key_ == entry_old.key) {
                    continue;
                }

                insert(entry_old.key, entry_old.value, true);
            }

            free_trailing_vla<slots_type, entry_type>(old_slots, num_old_slots);
        }

    public:

        /// Constructor, default-initialise the slots.
        hash_table(void) throw()
            : growing_(false)
        {
            // size of the default hash table
            const uint32_t capacity(1U << meta_type::DEFAULT_SCALE_FACTOR);

            // allocate the default slots
            slots_type *slots(
                new_trailing_vla<slots_type, entry_type>(capacity));

            slots->mask = capacity - 1;

            // initialise the internal table
            table_.num_entries = 0;
            table_.scaling_factor = meta_type::DEFAULT_SCALE_FACTOR;
            table_.entry_slots = slots;

            // initialise the default key
            memset(&default_key_, 0, sizeof(K));
        }


        /// Destructor, free the slots.
        ~hash_table(void) throw() {
            if(table_.entry_slots) {
                slots_type *old_slots(table_.entry_slots);
                const uint32_t num_slots(old_slots->mask + 1U);
                free_trailing_vla<slots_type, entry_type>(
                    table_.entry_slots, num_slots);
                table_.entry_slots = nullptr;
            }
        }

        /// Find the value associated with a key in the hash table.
        V find(const K key) const throw() {
            const slots_type * const slots(table_.entry_slots);
            const uint32_t mask(slots->mask);
            const entry_type * const entries(&(slots->entries[0]));
            uint32_t entry_base(meta_type::hash(key));

            for(;; entry_base += 1) {
                entry_base &= mask;
                const entry_type &entry(entries[entry_base]);

                // default value; nothing there.
                if(default_key_ == entry.key) {
                    break;

                // found it.
                } else if(entry.key == key) {
                    return entry.value;
                }
            }

            return V();
        }

        /// Search for an entry in the hash table.
        inline bool load(const K key, V &value) const throw() {
            value = find(key);
            return (V() != value);
        }

        /// Store a value in the hash table. Returns true iff the entry was
        /// written to the hash table.
        bool store(
            K key,
            V value,
            hash_store_policy update=HASH_OVERWRITE_PREV_ENTRY
        ) throw() {
            const uint32_t max_num_entries(
                HASH_TABLE_MAX_SIZES[table_.scaling_factor]);

            // check if we need to grow the hash table.
            if(table_.num_entries > max_num_entries) {
                growing_ = true;
                grow();
                growing_ = false;
            }

            const hash_store_state state(insert(
                key, value, HASH_OVERWRITE_PREV_ENTRY == update));

            if(HASH_ENTRY_STORED_NEW == state) {
                table_.num_entries += 1;
                return true;
            }

            return HASH_ENTRY_SKIPPED != state;
        }

        template <typename... Args>
        inline void for_each_entry(
            void (*callback)(K, V, Args&...),
            Args&... args
        ) throw() {
            slots_type *slots(table_.entry_slots);
            const uint32_t num_slots(slots->mask + 1U);

            // traverse each slot entry
            entry_type *entries(&(slots->entries[0]));

            for(uint32_t i(0); i < num_slots; ++i) {
                entry_type &entry(entries[i]);
                if(default_key_ == entry.key) {
                    continue;
                }

                callback(entry.key, entry.value, args...);
            }
        }
    };


    /// Simple hash-table based set.
    template <typename K>
    struct hash_set {
    private:
        hash_table<K, bool> table;

    public:

        void add(K key) throw() {
            table.store(key, true);
        }

        void remove(K key) throw() {
            table.store(key, false);
        }

        bool contains(K key) throw() {
            return table.find(key);
        }
    };


    /// Represents a locked hash table.
    template <
        typename K,
        typename V,
        typename meta_type=hash_table_meta<K,V>
    >
    struct locked_hash_table {
    private:

        mutable smp::spin_lock lock;
        hash_table<K, V, meta_type> table;

    public:

        /// Load a value from the hash table.
        inline bool load(const K key, V &val) const throw() {
            lock.acquire();
            bool ret(table.load(key, val));
            lock.release();
            return ret;
        }

        /// Store a value in the hash table. Returns true iff the entry was
        /// written to the hash table.
        inline bool store(
            K key,
            V value,
            hash_store_policy update=HASH_OVERWRITE_PREV_ENTRY
        ) throw() {
            lock.acquire();
            bool ret(table.store(key, value, update));
            lock.release();
            return ret;
        }

        inline bool find(
            K key
        ) throw() {
            lock.acquire();
            bool flag(table.find(key));
            lock.release();
            return flag;
        }

        template <typename... Args>
        inline void for_each_entry(
            void (*callback)(K, V, Args&...),
            Args&... args
        ) throw() {
            lock.acquire();
            table.for_each_entry(callback, args...);
            lock.release();
        }
    };
}


#endif /* HASH_TABLE_H_ */
