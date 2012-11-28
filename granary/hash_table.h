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


namespace granary {


    /// Default meta information about the hash table, as well as its hash
    /// function.
    template <typename K, typename V>
    struct hash_table_meta {
        enum {
            NUM_DEFAULT_ENTRIES = 512,
            SEED = 0xDEADBEEFU
        };

        inline static uint32_t hash(K key_) throw() {
            K key(key_);
            uint32_t curr_hash(0);
            MurmurHash3_x86_32(&key, sizeof key, SEED, &(curr_hash));
            return curr_hash;
        }

        inline static uint32_t mix(uint32_t hash) throw() {
            return fmix(hash);
        }
    };


    /// Implementation of an eventually consistent hash table.
    template <typename K, typename V>
    struct hash_table {
    public:

        typedef hash_table_meta<K, V> meta;


        /// Default-initialize key; used to detect unused entries.
        static const K DEFAULT_KEY;


        /// A single hash table entry.
        struct entry {
            K key;
            V value;
        };


        /// Contains the entries of the hash table.
        struct entry_list {
            entry *entries;
            uint32_t num_slots;
            uint32_t mask;

            /// Ref count for readers.
            std::atomic<uint32_t> num_readers;

            void acquire(void) throw() {
                num_readers.fetch_add(1, std::memory_order_relaxed);
            }

            void release(void) throw() {
                if(1 == num_readers.fetch_sub(1, std::memory_order_release)) {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    delete this; // evil
                }
            }
        };


        /// the entries of the hash table.
        std::atomic<entry_list *> read_entries;
        std::atomic<entry_list *> write_entries;


        /// Spin lock for writers.
        std::atomic_bool grow_lock;


        /// Reference counter used to protect the acquisition of entry lists.
        std::atomic<uint32_t> read_critical_counter;


        hash_table(void) throw()
            : read_entries(allocate_entries(meta::NUM_DEFAULT_ENTRIES))
            , write_entries(read_entries.load())
            , grow_lock(false)
        {}

        /// Perform a lookup on a key.
        bool load(K key, V *value) throw() {
            entry_list *elist(nullptr);
            const entry *entries(nullptr);
            const uint32_t hash1(meta::hash(key));

            begin_read_critical(); {
                elist = read_entries.load();
                elist->acquire();
                entries = elist->entries;
            } end_read_critical();

            const uint32_t mask(elist->mask);

            const entry &e1(entries[hash1 & mask]);
            if(e1.key == key) {
                *value = e1.value;
                elist->release();
                return true;
            }

            const uint32_t hash2(meta::mix(hash1));
            const entry &e2(entries[hash2 & mask]);
            if(e2.key == key) {
                *value = e2.value;
                elist->release();
                return true;
            }

            const uint32_t hash3(meta::mix(hash2));
            const entry &e3(entries[hash3 & mask]);
            if(e3.key == key) {
                *value = e3.value;
                elist->release();
                return true;
            }

            // all entries are taken; need to grow the table
            if(e1.key != DEFAULT_KEY
            && e2.key != DEFAULT_KEY
            && e3.key != DEFAULT_KEY) {
                uint32_t next_num_slots(elist->num_slots);
                elist->release();
                try_grow_table(next_num_slots);
            }

            // value isn't in the hash table
            return false;
        }

        /// Store a value in the hash table.
        void store(K key, V value) throw() {
            entry_list *elist(nullptr);
            entry *entries(nullptr);
            begin_read_critical(); {
                elist = write_entries.load();
                elist->acquire();
                entries = elist->entries;
            } end_read_critical();
            store_fast(key, value, entries, elist->mask);
            elist->release();
        }

    private:

        static entry_list *allocate_entries(uint32_t num_entries) {
            entry_list *list(new entry_list);
            list->entries = new entry[num_entries];
            list->num_slots = num_entries;
            list->mask = (num_entries << 1) - 1;
            list->acquire();
            return list;
        }

        void store_fast(K key, V value, entry *entries, uint32_t mask) throw() {
            const uint32_t hash1(meta::hash(key));
            const uint32_t hash2(meta::mix(hash1));
            const uint32_t hash3(meta::mix(hash2));

            entry &e1(entries[hash1 & mask]);
            e1.key = key;
            e1.value = value;

            entry &e2(entries[hash2 & mask]);
            e2.key = key;
            e2.value = value;

            entry &e3(entries[hash3 & mask]);
            e3.key = key;
            e3.value = value;
        }

        void copy(entry *source_min, entry *source_max, entry *dest, uint32_t dest_mask) {
            for(; source_min < source_max; ++source_min) {
                if(DEFAULT_KEY != source_min->key) {
                    store_fast(
                        source_min->key, source_min->value, dest, dest_mask);
                }
            }
        }

        /// Try to resize the hash table.
        void try_grow_table(uint32_t num_entries) {
            entry_list *write_list(nullptr);

            if(grow_lock) {
                return;
            }

            while(grow_lock.exchange(true)) { /* spin */ }

            // don't need to grow, we just grew the table.
            write_list = write_entries.load();
            if(write_list->num_slots >= num_entries) {
                grow_lock = false;
                return;
            }

            entry_list *new_list(allocate_entries(num_entries));
            entry_list *old_list(read_entries.load());

            // make copied hash table available to writers
            write_entries.store(new_list);

            copy(
                old_list->entries, old_list->entries + old_list->num_slots,
                new_list->entries, new_list->mask);

            // make copied hash table available to readers
            read_entries.store(new_list);

            // wait for any readers to finish; this will allow old readers to
            // release
            while(0 != read_critical_counter.load()) { /* spin */ }
            old_list->release();
            old_list = nullptr;

            // done
            grow_lock = false;
        }

        inline void begin_read_critical(void) {
            read_critical_counter.fetch_add(1, std::memory_order_relaxed);
        }

        inline void end_read_critical(void) {
            read_critical_counter.fetch_sub(1, std::memory_order_release);
        }
    };

    /// Static initialize the default value.
    template <typename K, typename V>
    const K hash_table<K, V>::DEFAULT_KEY = K();
}

#endif /* Granary_HASH_TABLE_H_ */
