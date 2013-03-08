/*
 * hash_table.h
 *
 *  Created on: 2013-01-20
 *      Author: pag
 */

#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include "granary/globals.h"
#include "granary/smp/rcu.h"
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

            delete old_slots;
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
                delete table_.entry_slots;
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


    namespace smp {


        RCU_GENERIC_PROTOCOL(
            (typename K, typename V),
            detail::hash_table_entry_slots, (K, V, true),
            RCU_VALUE(mask),
            RCU_VALUE(entries));


        RCU_GENERIC_PROTOCOL(
            (typename K, typename V),
            detail::hash_table_impl, (K, V, true),

            RCU_VALUE(num_entries),
            RCU_VALUE(scaling_factor),
            RCU_VALUE(should_grow),
            RCU_REFERENCE(entry_slots));
    }


    /// Shared hash table implementation.
    template <typename K, typename V>
    struct rcu_hash_table {
    private:

        typedef hash_table_meta<K, V> meta_type;
        typedef detail::hash_table_impl<K, V, true> table_type;
        typedef detail::hash_table_entry_slots<K, V, true> slots_type;
        typedef detail::hash_table_entry<K, V, true> entry_type;

        typedef smp::rcu_read_reference<table_type> table_read_ref_type;
        typedef smp::rcu_write_reference<table_type> table_write_ref_type;

        typedef smp::rcu_read_reference<slots_type> slots_read_ref_type;
        typedef smp::rcu_write_reference<slots_type> slots_write_ref_type;

        typedef smp::rcu_publisher<table_type> publisher_type;
        typedef smp::rcu_collector<table_type> collector_type;


        /// Represents the internal table of this hash table. The internal
        /// table need not be heap-allocated because it doesn't change, only
        /// the set of slots changes.
        table_type internal_table_;


        /// Represents the protected hash table, backed by the internal hash
        /// table.
        smp::rcu_protected<table_type> table_;


        /// Find an entry in the table using linear probing.
        static bool read_table(
            table_read_ref_type table,
            const K &key,
            V &value
        ) throw() {
            slots_read_ref_type slots(table.entry_slots);

            const uint32_t mask(slots.mask);
            const entry_type * const entries(slots.entries);

            uint32_t entry_base(meta_type::hash(key));

            for(;; entry_base += 1) {
                entry_base &= mask;
                const entry_type &entry(entries[entry_base]);

                // we've found a place where there is no entry
                if(!entry.is_valid.load()) {
                    return false;
                }

                if(entry.key.load() == key) {
                    value = entry.value;
                    return true;
                }
            }

            return false;
        }

        /// Add an entry to the hash table.
        struct add_entry : public smp::rcu_writer<table_type> {
        public:

            const K new_key;
            const V new_value;
            const bool overwrite_old_value;
            hash_store_state stored_state;
            bool should_grow;
            slots_write_ref_type old_slots_;

            add_entry(const K key_, const V value_, bool update) throw()
                : new_key(key_)
                , new_value(value_)
                , overwrite_old_value(update)
                , stored_state(HASH_ENTRY_SKIPPED)
                , should_grow(false)
                , old_slots_()
            { }

            /// Insert an entry into the hash table. Returns true iff an element
            /// with the key didn't previously exist in the hash table.
            hash_store_state insert(
                entry_type *slots,
                uint32_t mask,
                K key,
                V value,
                bool update
            ) throw() {
                uint32_t entry_base(meta_type::hash(key));
                unsigned scan(0);
                hash_store_state state(HASH_ENTRY_SKIPPED);

                for(;; ++scan, entry_base += 1) {
                    entry_base &= mask;
                    entry_type &entry(slots[entry_base]);

                    // found the insert position; don't need to CAS because
                    // insertion is guarded by the RCU mutex.
                    if(!entry.is_valid.load()) {
                        entry.value.store(value);
                        entry.key.store(key);
                        entry.is_valid.store(true); // publish
                        state = HASH_ENTRY_STORED_NEW;
                        break;
                    }

                    // already inserted
                    if(entry.key.load() == key) {
                        if(update) {
                            entry.value.store(value);
                            state = HASH_ENTRY_STORED_OVERWRITE;
                        }
                        break;
                    }
                }

                // need to grow
                if(meta_type::MAX_SCAN_SCALE_FACTOR < scan) {
                    should_grow = true;
                }

                return state;
            }

            /// Grow the hash table. This increases the hash table's size by
            /// two.
            slots_type *grow(slots_write_ref_type slot_ref) throw() {

                entry_type *old_slots(slot_ref.entries);
                const uint32_t num_old_slots(slot_ref.mask + 1U);
                const uint32_t num_new_slots(num_old_slots << 1);

                slots_type *new_slots(
                    new_trailing_vla<slots_type, entry_type>(num_new_slots));

                const uint32_t new_mask(num_new_slots - 1U);
                new_slots->mask = new_mask;

                // transfer elements to the new slots
                for(uint32_t i(0); i < num_old_slots; ++i) {
                    entry_type &entry_old(old_slots[i]);
                    if(!entry_old.is_valid.load()) {
                        continue;
                    }

                    insert(
                        &(new_slots->entries[0]), new_mask,
                        entry_old.key.load(), entry_old.value.load(),
                        true);
                }

                return new_slots;
            }

            /// See if we need to grow the hash table. This is done while
            /// readers might exist so that we can properly wait on all
            /// readers seeing the old version of the hash table.
            virtual void while_readers_exist(
                table_write_ref_type table,
                publisher_type &publisher
            ) throw() {
                const uint32_t max_num_entries(
                    HASH_TABLE_MAX_SIZES[table.scaling_factor]);

                // check if we need to grow the hash table.
                if(table.num_entries > max_num_entries
                || table.should_grow) {

                    old_slots_ = table.entry_slots;
                    table.should_grow = false;
                    table.entry_slots = publisher.promote(
                        grow(table.entry_slots));

                    table.scaling_factor = (table.scaling_factor + 1);
                }
            }

            /// After readers are done looking at the old version of the hash
            /// table (assuming the hash table grew), we will add our entry in.
            virtual void after_readers_done(table_write_ref_type table) throw() {
                stored_state = insert(
                    slots_write_ref_type(table.entry_slots).entries,
                    slots_write_ref_type(table.entry_slots).mask,
                    new_key,
                    new_value,
                    overwrite_old_value);

                // we added an element.
                if(HASH_ENTRY_STORED_NEW == stored_state) {
                    table.num_entries = table.num_entries + 1;
                }

                // signal a forced growth on the next insert.
                if(should_grow) {
                    table.should_grow = true;
                }
            }

            /// If this writer grew the hash table, then fee up the old
            /// version of the table.
            virtual void teardown(collector_type &collector) throw() {
                if(old_slots_) {
                    delete collector.demote(old_slots_);
                }
            }
        };

        /// Initialise the rcu-protected hash table.
        struct initialise : public smp::rcu_writer<table_type> {
        public:

            /// Pointer to the hash table backing the RCU-protected table
            /// interface.
            table_type * const internal_table;

            initialise(table_type *internal_table__)
                : internal_table(internal_table__)
            { }

            /// See if we need to grow the hash table. This is done while
            /// readers might exist so that we can properly wait on all
            /// readers seeing the old version of the hash table.
            virtual void while_readers_exist(
                table_write_ref_type,
                publisher_type &publisher
            ) throw() {
                publisher.publish(publisher.promote(internal_table));
            }
        };

    public:

        rcu_hash_table(void) throw()
            : internal_table_()
            , table_(smp::RCU_INIT_NULL)
        {
            // size of the default hash table
            const uint32_t capacity(1U << meta_type::DEFAULT_SCALE_FACTOR);
            const uint32_t mask(capacity - 1);

            // allocate the default slots
            slots_type *slots(
                new_trailing_vla<slots_type, entry_type>(capacity));

            slots->mask = mask;

            // initialise the internal table
            internal_table_.num_entries = 0;
            internal_table_.scaling_factor = meta_type::DEFAULT_SCALE_FACTOR;
            internal_table_.entry_slots = slots;

            // initialise the RCU-protected internal table
            initialise table_initialiser(&internal_table_);
            table_.write(table_initialiser);
        }

        ~rcu_hash_table(void) throw() {

            // make it so that the RCU protected class won't try to free the
            // internal table.
            initialise table_initialiser(nullptr);
            table_.write(table_initialiser);

            // free up the slots, if any.
            if(internal_table_.entry_slots) {
                delete internal_table_.entry_slots;
                internal_table_.entry_slots = nullptr;
            }
        }

        /// Load a value from the hash table.
        inline bool load(const K key, V &val) const throw() {
            return table_.read(read_table, key, val);
        }

        /// Store a value in the hash table. Returns true iff the entry was
        /// written to the hash table.
        inline bool store(
            K key,
            V value,
            hash_store_policy update=HASH_OVERWRITE_PREV_ENTRY
        ) throw() {
            add_entry entry_adder(
                key, value, HASH_OVERWRITE_PREV_ENTRY == update);
            table_.write(entry_adder);
            return HASH_ENTRY_SKIPPED != entry_adder.stored_state;
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
    };
}


#endif /* HASH_TABLE_H_ */
