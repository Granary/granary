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
#include <cstddef>

#include "granary/pp.h"
#include "granary/allocator.h"
#include "granary/type_traits.h"

#include "granary/rcu/rcu.h"

#include "deps/murmurhash/murmurhash.h"

namespace granary { namespace rcu {


    /// Default implementation of meta information for hash table entries.
    template <typename K, typename V>
    struct hash_table_meta {
    public:

        enum {
            HASH_SEED = 0xDEADBEEFU,
            MAX_BUCKET_SIZE = 4U
        };

        inline static uint32_t hash(const K &key) throw() {
            uint32_t hash_value(0);
            MurmurHash3_x86_32(&key, sizeof key, HASH_SEED, &hash_value);
            return hash_value;
        }
    };

    namespace detail {

        /// Represents a single hash table entry.
        template <typename K, typename V>
        struct hash_table_entry {
        public:
            K key;
            V value;
        };


        /// Represents an array of hash table entries.
        template <typename K, typename V>
        struct hash_table_bucket {
        public:

            typedef hash_table_bucket<K, V> self_type;

            unsigned num_entries;

            // The entries of this bucket; this represents
            // a run-off array.
            hash_table_entry<K, V> entries[1];
        };


        /// Represents a hash table mapping keys to values. The usage of this
        /// structure is as a trailing very large array of rcu-protected hash
        /// table buckets.
        template <typename K, typename V>
        struct hash_table {
        public:

            typedef hash_table<K, V> self_type;

            /// The number of elements in the hash table.
            unsigned entry_mask;

            /// The buckets of the hash table.
            unsigned num_buckets;

            /// The buckets of this hash table; this represents a
            /// run-off array.
            rcu_protected<hash_table_bucket<K, V>> buckets[1];
        };
    }


    /// Represents a hash table mapping keys to values.
    ///
    /// Note: this hash table permits inconsistencies in several ways:
    ///     1) When growing, first the bucket pointer is changed, then
    ///        the number of buckets, then the bucket mask. This means
    ///        that between any of these operations, a reader might not
    ///        find something that is legitimately in the table.
    ///     2) A reader might be reading an old version of the hash
    ///        table while a concurrent writer adds an entry. If the
    ///        entry is added to a new array of buckets while the reader
    ///        observes the old array then that reader will not see
    ///        something that is validly there.
    template <typename K, typename V>
    struct hash_table {
    private:

        typedef detail::hash_table_entry<K, V> entry_type;
        typedef detail::hash_table_bucket<K, V> bucket_type;
        typedef detail::hash_table<K, V> table_type;
        typedef hash_table_meta<K, V> meta_type;

        static_assert(
            offsetof(bucket_type, num_entries) < offsetof(bucket_type, entries),
            "Invalid layout of hash table bucket.");

        static_assert(
            offsetof(table_type, entry_mask) < offsetof(table_type, buckets),
            "Invalid layout of hash table.");

        static_assert(
            offsetof(table_type, num_buckets) < offsetof(table_type, buckets),
            "Invalid layout of hash table.");


        /// The actual hash table.
        rcu_protected<table_type> table;


        /// Should we grow the table? Writers will make grow requests on the
        /// table by incrementing the grow_requests. A grow request will only
        /// be made when the size of a bucket exceeds meta_type::MAX_BUCKET_SIZE.
        std::atomic<unsigned> grow_requests;


        /// Try to find the value for a given key within a specific bucket.
        static bool load_read_bucket(
            rcu_read_reference<bucket_type> &bucket,
            const K &key,
            V &val
        ) throw() {
            const entry_type *entry(&(bucket->entries[0]));
            const entry_type *last_entry(&(entry[bucket->num_entries + 1]));
            for(; entry < last_entry; ++entry) {
                if(entry->key == key) {
                    val = entry->value;
                    return true;
                }
            }
            return false;
        }


        /// Find the bucket that might contain the value for a given key.
        static bool load_read_table(
            rcu_read_reference<table_type> &htab,
            const uint32_t &hash,
            const K &key,
            V &val
        ) throw() {
            const uint32_t index(hash & htab->entry_mask);
            return (htab->buckets[index]).read(load_read_bucket, key, val);
        }

    public:


        /// Search for a key in the hash table. If the key is found, return
        /// true and update the value referenced by `val`. Otherwise, return
        /// false.
        inline bool load(const K key, V &val) const throw() {
            const uint32_t hash(meta_type::hash(key));
            return table.read(load_read_table, hash, key, val);
        }

    private:


        /// Represents the protocol for growing an rcu-protected hash table
        /// bucket. This protocol is also implemented by the table grower
        /// protocol.
        struct bucket_grower : public rcu_writer<bucket_type> {
        public:

            typedef rcu_write_reference<bucket_type> bucket_ptr;


            /// The old bucket that we will be replacing.
            bucket_type *old_bucket;


            /// The key and value to be stored in the new bucket.
            K key;
            V value;


            /// Allocate and copy a new bucket concurrently with other readers,
            /// but being the exclusive writer. At the end, we will publish the
            /// bucket.
            virtual void while_readers_exist(bucket_ptr rcu_bucket) throw() {
                entry_type *old_entries(nullptr);
                unsigned old_num_entries(0);

                if(rcu_bucket.is_valid()) {
                    old_entries = &(rcu_bucket->entries[0]);
                    old_num_entries = rcu_bucket->num_entries;
                }

                const unsigned new_num_entries(old_num_entries + 1);
                bucket_type *new_bucket(
                    new_trailing_vla<bucket_type, entry_type>(new_num_entries));
                new_bucket->num_entries = new_num_entries;

                // move the entries into the newly allocated copy of the
                // data structure.
                if(std::is_trivial<entry_type>::value) {
                    memcpy(&(new_bucket->entries[0]),
                           old_entries,
                           old_num_entries * sizeof(entry_type));
                } else {
                    for(unsigned i(0); i < old_num_entries; ++i) {
                        new_bucket->entries[i] = old_entries[i];
                    }
                }

                // add in the new entry to the new bucket
                new_bucket->entries[old_num_entries].key = key;
                new_bucket->entries[old_num_entries].value = value;

                // publish the new bucket, return and store the old bucket.
                old_bucket = rcu_bucket.publish(new_bucket);
            }


            /// If an old bucket existed then delete it.
            virtual void teardown(bucket_ptr) throw() {
                if(old_bucket) {
                    delete old_bucket;
                }
            }
        };


        /// Represents the protocol for growing an rcu-protected hash table.
        struct table_grower : public rcu_writer<table_type> {
        public:

            typedef rcu_write_reference<table_type> table_ptr;


            /// The old table to be deleted.
            table_type *old_table;

            /// The grow requests on the table
            std::atomic<unsigned> *grow_requests;

            /// Should we grow?
            bool do_grow;


            /// Scan through a hash table bucket and re-hash the entries. This
            /// executes in the read-critical section of one bucket, and performs
            /// an uncontended write to another bucket (the other bucket is
            /// not visible to anyone yet).
            static void copy_bucket_entries(
                rcu_read_reference<bucket_type> &rcu_bucket,
                bucket_grower &bucket_grower,
                table_type &new_table
            ) throw() {

                const entry_type *entry(&(rcu_bucket->entries[0]));
                const unsigned num_entries(rcu_bucket->num_entries);

                for(unsigned i(0); i < num_entries; ++entry) {
                    const uint32_t hash(meta_type::hash(entry->key));
                    const uint32_t index(hash & new_table.entry_mask);

                    bucket_grower.key = entry->key;
                    bucket_grower.value = entry->value;
                    new_table.buckets[index].write(bucket_grower);
                }
            }


            /// When we have mutual exclusion over the hash table, allocate a new
            /// has table, copy over everything to the new hash table (while
            /// concurrent reads might still be happening), then publish the new
            /// hash table.
            virtual void while_readers_exist(table_ptr rcu_table) throw() {

                // double check our grow requests; someone is growing, or there are no grow requests
                if(0 == grow_requests->load()) {
                    do_grow = false;
                    return;
                }

                /// allocate the new table
                const unsigned old_num_buckets(rcu_table->num_buckets);
                const unsigned new_num_buckets(old_num_buckets * 2);
                table_type *new_table(
                    new_trailing_vla<table_type, rcu_protected<bucket_type>>(
                        new_num_buckets));

                // initialize
                new_table->num_buckets = new_num_buckets;
                new_table->entry_mask = new_num_buckets - 1;

                /// The bucket grower, accessed by a bucket reader.
                bucket_grower grower;

                // copy entries over
                rcu_protected<bucket_type> *old_bucket(&(rcu_table->buckets[0]));
                for(unsigned b(0); b < old_num_buckets; ++b, ++old_bucket) {
                    old_bucket->read(copy_bucket_entries, grower, *new_table);
                }

                // publish the new table to any readers
                old_table = rcu_table.publish(new_table);
            }


            /// After all readers are done, but before we release mutual
            /// exclusion, we will permit new grow requests to come in.
            virtual void after_readers_done(table_ptr) throw() {
                if(do_grow) {
                    grow_requests->store(0);
                }
            }


            /// Now no readers are viewing the old buckets, and no writers
            /// are potentially viewing them either; delete the old buckets.
            virtual void teardown(table_ptr) throw() {
                if(!do_grow) {
                    return;
                }

                rcu_protected<bucket_type> *bucket(&(old_table->buckets[0]));
                const rcu_protected<bucket_type> *max_bucket(
                    bucket + old_table->num_buckets);

                // manually deallocate internal buckets because they are VLAs,
                // which are not properly understood by `delete`.
                for(; bucket != max_bucket; ++bucket) {
                    bucket_type *&internal_bucket(rcu_unprotect(*bucket));
                    if(internal_bucket) {
                        delete internal_bucket;
                        internal_bucket = nullptr;
                    }
                }

                delete old_table;
            }
        };


        /// Grow the hash table by a factor of 2.
        inline void grow_table(void) throw() {

            // someone is growing, or there are no grow requests
            unsigned outstanding_requests(grow_requests.load());
            if(0 == outstanding_requests) {
                return;
            }

            // we will double check inside the grower that there
            // are no grow requests.
            table_grower grower;
            grower.grow_requests = &grow_requests;
            grower.do_grow = true;
        }


        /// Read from the table, and potentially update a bucket by
        /// upgrading a read


        /// Read from the table, and potentially write to a bucket.
        static void read_table_store_entry(
            rcu_read_reference<table_type> &table,
            const K &key,
            const V &val
        ) throw() {
            const uint32_t hash(meta_type::hash(key));
            const uint32_t index(hash & table->entry_mask);

            // TODO
            (void) hash;
            (void) index;
            (void) val;
        }

    public:


        /// Store a value in the hash table. This will potentially grow the
        /// hash table.
        void store(const K key, const V val) throw() {
            grow_table();
            table.read(read_table_store_entry, key, val);
        }
    };
}}

#endif /* Granary_HASH_TABLE_H_ */
