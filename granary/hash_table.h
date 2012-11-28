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

#include "granary/allocator.h"

extern "C" {
#   include "deps/murmurhash/murmurhash.h"
}


namespace granary {

    template <typename K>
    struct hash_function {
    public:

        enum {
            SEED = 0xDEADBEEFU
        };

        static uint32_t hash(K &&key_) throw() {
            K key(key_);
            uint32_t curr_hash(0);
            MurmurHash3_x86_32(&key, sizeof key, SEED, &(curr_hash));
            return curr_hash;
        }
    };

    /// Implementation of open-addressing hash table.
    template <typename K, typename V>
    struct hash_table {
    public:

        enum {
            MAX_OPEN_ADDRESSING = 32,
            BASE_SIZE = 512
        };

        /// Represents a hash table entry.
        struct entry {
            K key;
            V value;

            /// the index of the next entry with the same hash value.
            uint32_t next_index;

            /// the index of the previous entry with the same hash value.
            uint32_t prev_index;

        } __attribute__((packed));


        /// Represents a group of entries. This is implemented an an extended
        /// sequence in that as much space for an entry_list + size-1 * entry
        /// is allocated. The address of `first__` is seen as the address of
        /// the first entry, and the remaining entries run off the end of the
        /// structure.
        struct entry_list {

            /// The number of entry slots in this list.
            unsigned size;

            /// The number of references being held on this list.
            std::atomic<int> ref_count;

            /// Dummy element representing the first element in the list.
            entry first__;

            entry_list(unsigned size_) throw()
                : size(size_)
            { }

            void acquire(void) volatile throw() {
                ref_count.fetch_add(1, std::memory_order_relaxed);
            }

            void release(void) volatile throw() {
                if(1 == ref_count.fetch_sub(1, std::memory_order_release)) {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    delete this; // evil :-P
                }
            }

            /// Allocate the entry list along with all the needed space for
            /// entries.
            static entry_list *allocated(unsigned num_entries) throw() {
                void *mem(detail::global_allocate(
                    sizeof(entry_list) + sizeof(entry) * num_entries));
                return new (mem) entry_list(num_entries);
            }

        } __attribute__((packed));


        /// Represents a handle on an entry. Note: entry handles have strict
        /// ownership semantics.
        struct entry_handle {
        private:

            entry *entry_;
            entry_list *list_;

            entry_handle(entry *e, entry_list *ll) throw()
                : entry_(e)
                , list_(ll)
            {
                // TODO: there is a race condition here where the list is
                ///      deleted out from under us before we acquire a ref
                ///      count.
                list_->acquire();
            }

        public:

            entry_handle(void) = delete;

            /// Move constructor; takes ownership.
            entry_handle(entry_handle &&that) throw()
                : list_(that.list_)
                , entry_(that.entry_)
            {
                that.list_ = nullptr;
            }


            /// Destructor; potentially released a set of entries.
            ~entry_handle(void) throw() {
                if(list_) {
                    list_->release();
                }
                list_ = nullptr;
                entry_ = nullptr;
            }


            /// Move assignment operator.
            entry_handle &operator=(entry_handle &&that) throw() {
                if(this != &that) {
                    list_ = that.list_;
                    entry_ = that.entry_;
                    that.list_ = nullptr;
                }
                return *this;
            }

            inline const V *operator->(void) const throw() {
                return &(entry_->value);
            }

            inline V &operator*(void) throw() {
                return entry_->value;
            }
        };


        /// All table entries.
        volatile entry_list *entries;


        /// Spin lock for growing.
        std::atomic<bool> grow_lock;


        /// Initialize the entries;
        ///
        /// Note: construction must be done in such a way that the hash table
        ///       is not shared/used until *after* it is constructed.
        hash_table(void) throw()
            : entries(new entry_list(BASE_SIZE))
        {
            entries->acquire();
        }


        /// Gets an element in the hash table if it exists. If it doesn't
        /// exist then the element is created.
        entry_handle upsert(K &&key) throw() {
            const uint32_t hash(hash_function<K>::hash(key));
            while(grow_lock) { /* spin */ }

            entry_list *elist(entries);
            elist->acquire();

            uint32_t last_index(0);
            entry *e(find_entry(hash, key, elist, &last_index));

            // need to add the item; we will start at index `last_index`.
            if(nullptr == e) {

            }

            entry_handle handle(e);
            elist->release();

            return handle;
        }

    private:



        /// Find where an entry is stored in the table.
        entry *find_entry(const uint32_t hash,
                           K &&key,
                           entry_list *elist,
                           uint32_t *last_index) throw() {

            entry *entries(&(elist->first__));
            uint32_t prev_index(0);
            for(uint32_t index(1 + (hash & elist->size)); ; ) {

                if(entries[index].key == key) {
                    return &(entries[index]);
                }

                prev_index = index;
                index = entries->next_index;

                if(prev_index == index) {
                    return nullptr;
                } else {
                    *last_index = prev_index;
                }
            }

            return nullptr;
        }
    };

}

#endif /* Granary_HASH_TABLE_H_ */
