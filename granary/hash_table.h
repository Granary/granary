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

#include "deps/murmurhash/murmurhash.h"

namespace granary {

    /// The maximum sizes of hash tables as different scaling factors.
    extern uint32_t HASH_TABLE_MAX_SIZES[];


    /// Default implementation of meta information for hash table entries.
    template <typename K, typename V>
    struct hash_table_meta {
    public:

        enum {
            HASH_SEED = 0xDEADBEEFU,
        };

        enum {
            DEFAULT_SCALE_FACTOR = 8U
        };

        inline static uint32_t hash(const K &key) throw() {
            uint32_t hash_value(0);
            MurmurHash3_x86_32(&key, sizeof key, HASH_SEED, &hash_value);
            return hash_value;
        }
    };

    namespace detail {

        /// Represents an entry in a hash table.
        template <typename K, typename V, bool is_shared>
        struct hash_table_entry {
        public:
            opt_atomic<bool, is_shared> is_valid;
            opt_atomic<K, is_shared> key;
            opt_atomic<V, is_shared> value;
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


        /// Hash table implementation.
        template <typename K, typename V, bool is_shared>
        struct hash_table_impl {
        public:

            typedef hash_table_entry<K, V, is_shared> entry_type;
            typedef hash_table_entry_slots<K, V, is_shared> slot_type;
            typedef hash_table_meta<K, V> meta_type;

            /// Number of entries stored in the hash table.
            uint32_t num_entries;

            /// Track the scaling factor.
            uint32_t scaling_factor;

            /// The entry slots represent the thing that can change when shared.
            slot_type *entry_slots;
        };
    }

    namespace smp {


        RCU_GENERIC_PROTOCOL(
            (typename K, typename V, bool is_shared),
            detail::hash_table_entry_slots, (K, V, is_shared),
            RCU_VALUE(mask),
            RCU_VALUE(entries));


        RCU_GENERIC_PROTOCOL(
            (typename K, typename V, bool is_shared),
            detail::hash_table_impl, (K, V, is_shared),

            RCU_VALUE(num_entries),
            RCU_VALUE(scaling_factor),
            RCU_REFERENCE(entry_slots));
    }


    /// Basic, non-shared hash table.
    template <typename K, typename V>
    struct hash_table {
    private:

        detail::hash_table_impl<K, V, false> table;

    public:


    };


    /// Shared hash table implementation.
    template <typename K, typename V>
    struct shared_hash_table {
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
        table_type internal_table;


        /// Represents the protected hash table, backed by the internal hash
        /// table.
        smp::rcu_protected<table_type> table;


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
            slots_write_ref_type old_slots;

            add_entry(const K key_, const V value_) throw()
                : new_key(key_)
                , new_value(value_)
                , old_slots()
            { }

            /// Insert an entry into the hash table. Returns true iff an element
            /// with the key didn't previously exist in the hash table.
            bool insert(
                entry_type *slots,
                uint32_t mask,
                K key,
                V value
            ) throw() {
                uint32_t entry_base(meta_type::hash(key));
                bool invalid(false);

                for(;; entry_base += 1) {
                    entry_base &= mask;
                    entry_type &entry(slots[entry_base]);

                    // insert position
                    if(entry.is_valid.compare_exchange_strong(invalid, true)) {
                        entry.value.store(value);
                        entry.key.store(key);

                    // already inserted
                    } if(entry.key.load() == key) {
                        entry.value.store(value);
                        return false;
                    }
                }
                return false;
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
                        entry_old.key.load(), entry_old.value.load());
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
                if(table.num_entries > max_num_entries) {
                    old_slots = table.entry_slots;
                    table.entry_slots = publisher.promote(
                        grow(table.entry_slots));

                    table.scaling_factor = (table.scaling_factor * 2);
                }
            }

            /// After readers are done looking at the old version of the hash
            /// table (assuming the hash table grew), we will add our entry in.
            virtual void after_readers_done(table_write_ref_type table) throw() {
                const bool added_element(insert(
                    slots_write_ref_type(table.entry_slots).entries,
                    slots_write_ref_type(table.entry_slots).mask,
                    new_key,
                    new_value));

                if(added_element) {
                    table.num_entries = table.num_entries + 1;
                }
            }

            /// If this writer grew the hash table, then fee up the old
            /// version of the table.
            virtual void teardown(collector_type &collector) throw() {
                if(old_slots) {
                    delete collector.demote(old_slots);
                }
            }
        };

        /// Initialise the rcu-protected hash table.
        struct initialise : public smp::rcu_writer<table_type> {
        public:

            /// Pointer to the hash table backing the RCU-protected table
            /// interface.
            table_type * const internal_table;

            initialise(table_type *internal_table_)
                : internal_table(internal_table_)
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

        shared_hash_table(void) throw()
            : internal_table()
            , table(smp::RCU_INIT_NULL)
        {
            // size of the default hash table
            const uint32_t capacity(1U << meta_type::DEFAULT_SCALE_FACTOR);
            const uint32_t mask(capacity - 1);

            // allocate the default slots
            slots_type *slots(
                new_trailing_vla<slots_type, entry_type>(capacity));

            slots->mask = mask;

            // initialise the internal table
            internal_table.num_entries = 0;
            internal_table.scaling_factor = meta_type::DEFAULT_SCALE_FACTOR;
            internal_table.entry_slots = slots;

            // initialise the RCU-protected internal table
            initialise table_initialiser(&internal_table);
            table.write(table_initialiser);
        }

        ~shared_hash_table(void) throw() {

            // make it so that the RCU protected class won't try to free the
            // internal table.
            initialise table_initialiser(nullptr);
            table.write(table_initialiser);

            // free up the slots, if any.
            if(internal_table.entry_slots) {
                delete internal_table.entry_slots;
                internal_table.entry_slots = nullptr;
            }
        }

        inline bool load(const K key, V &val) const throw() {
            val = nullptr;
            bool found(table.read(read_table, key, val));
            return found;
        }

        void store(K key, V value) throw() {
            add_entry entry_adder(key, value);
            table.write(entry_adder);
        }
    };
}


#endif /* HASH_TABLE_H_ */
