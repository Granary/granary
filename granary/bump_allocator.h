/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bump_allocator.h
 *
 *  Created on: Jan 11, 2013
 *      Author: pag
 */

#ifndef BUMP_ALLOCATOR_H_
#define BUMP_ALLOCATOR_H_

#include <new>
#include <type_traits>

#include "granary/utils.h"
#include "granary/spin_lock.h"

namespace granary {


    /// Represents a single bump_pointer_slab of bump-pointer allocatable memory.
    /// No attempt is made to pack as much into the bump_pointer_slabs as possible.
    struct bump_pointer_slab {
        bump_pointer_slab *next;
        uint8_t *memory;
        unsigned index;
        unsigned remaining;
        unsigned size;

        /// Return a pointer to a next pointer that we can use to connect
        /// two slab lists together.
        bump_pointer_slab **connect(void) throw() {
            bump_pointer_slab *curr(this);
            for(; curr->next; curr = curr->next) {
                // La la la...
            }

            return &(curr->next);
        }
    };


    /// Defines a generic bump pointer allocator. The allocator is configured
    /// by the Config type.
    ///
    /// The following config options must be present:
    ///     SLAB_SIZE:      A low bound on the number of bytes to allocate per bump_pointer_slab
    ///                     of memory. This is a lower bound because allocation of
    ///                     executable memory might require more memory to be
    ///                     allocated.
    ///     EXECUTABLE:     True iff the memory allocated must be executable.
    ///     TRANSIENT:      True iff the memory allocated is considered short
    ///                     lived. The implication is that bump-pointer memory
    ///                     is either short lived (and can all be freed at once)
    ///                     or persists indefinitely.
    ///     SHARED:         True iff this allocator is shared between cores/threads.
    ///
    /// The allocator has the property that the last allocation can optionally be
    /// released (e.g. in the event that we want to back out of the most recent
    /// allocation and reclaim the memory for future allocations). Note: the
    /// last allocation can only be released if the allocator is not shared.
    ///
    /// The allocator also has the property that it's greedy, i.e. even if the
    /// memory allocated is considered transient, the allocator will still hold on
    /// to all allocated bump_pointer_slabs in its free list so that it can re-use it in later
    /// (transient) allocations. Obviously, if the memory is not transient, then
    /// the default greediness is desirable.
    template <typename Config>
    struct bump_pointer_allocator {
    private:

        enum {
            IS_EXECUTABLE = !!Config::EXECUTABLE,
            IS_TRANSIENT = !!Config::TRANSIENT,
            IS_SHARED = !!Config::SHARED,

            SHARE_DEAD_SLABS = !IS_SHARED && !!Config::SHARE_DEAD_SLABS,

            // Minimum alignment for all allocated objects.
            MIN_ALIGN = Config::MIN_ALIGN,

            // Adjusted bump_pointer_slab size for allocating executable memory.
            SLAB_SIZE_ = Config::SLAB_SIZE,
            SLAB_SIZE = IF_KERNEL_ELSE(
                    SLAB_SIZE_,
                    IS_EXECUTABLE
                        ? (SLAB_SIZE_ + ALIGN_TO(SLAB_SIZE_, PAGE_SIZE))
                        : SLAB_SIZE_),

            EXEC_WHERE = Config::EXEC_WHERE,

            // Alignment within the bump_pointer_slab for the first allocated
            // memory
            FIRST_ALLOCATION_ALIGN = IS_EXECUTABLE ? PAGE_SIZE : 16,

            // Value to default-initialize the memory with.
            MEMSET_VALUE = IS_EXECUTABLE ? 0xCC : 0
        };


        /// The next bump_pointer_slab from which we can allocate.
        bump_pointer_slab *curr;
        bump_pointer_slab *first;


        /// A free list of bump_pointer_slabs.
        bump_pointer_slab *free;


        /// A global free list of bump_pointer_slabs.
        static bump_pointer_slab *global_free;
        static atomic_spin_lock global_free_lock;


        /// Lock used to serialise allocations.
        spin_lock lock;


        /// The size of the last allocation.
        unsigned last_allocation_size;
        uint8_t *last_allocation;
        bump_pointer_slab *last_allocation_slab;


        /// The last person who allocated.
        IF_TEST( const void *last_allocator; )
        IF_TEST( int curr_owner_cpu_id; )


        /// Acquire a lock on the allocator.
        inline void acquire(void) throw() {
            if(IS_SHARED) {
                lock.acquire();
            }
        }


        /// Release the lock on the allocator.
        inline void release(void) throw() {
            if(IS_SHARED) {
                lock.release();
            }
        }


        /// Search for a slab in a slab list.
        bump_pointer_slab *slab_search(
            bump_pointer_slab **unlink_ptr,
            bump_pointer_slab *found,
            unsigned size
        ) throw() {

            bump_pointer_slab **bigger_unlink_ptr(nullptr);
            bump_pointer_slab *bigger_found(nullptr);

            // Try to find a slab of exact size.
            for(; found;) {
                if(found->size == size) {
                    goto found_exact_slab;
                } else if(found->size > size) {
                    if(!bigger_found
                    || (bigger_found && bigger_found->size > found->size)) {
                        bigger_found = found;
                        bigger_unlink_ptr = unlink_ptr;
                    }
                }

                unlink_ptr = &(found->next);
                found = found->next;
            }

            if(bigger_found) {
                *bigger_unlink_ptr = bigger_found->next;
                return bigger_found;
            } else {
                return nullptr;
            }

        found_exact_slab:
            *unlink_ptr = found->next;
            return found;
        }


        /// Allocate a new slab, either by finding it in a free list, or by
        /// manually allocating it.
        bump_pointer_slab *allocate_slab(unsigned size) throw() {
            bump_pointer_slab *found(nullptr);

            // Try to find one in the free list.
            if(free) {
                found = slab_search(&free, free, size);
                if(found) {
                    goto initialise;
                }
            }

            // See if we might be able to search in the global free list.
            if(SHARE_DEAD_SLABS
            && global_free && global_free_lock.try_acquire()) {
                found = slab_search(&global_free, global_free, size);
                global_free_lock.release();
                if(found) {
                    goto initialise;
                }
            }

            // Need to allocate a new one.
            found = allocate_memory<bump_pointer_slab>();
            if(IS_EXECUTABLE) {
                found->memory = unsafe_cast<uint8_t *>(
                    detail::global_allocate_executable(size, EXEC_WHERE));
                found->size = size;
            } else {
                found->memory = allocate_memory<uint8_t>(SLAB_SIZE);
                found->size = size;
            }

        initialise:
            memset(found->memory, MEMSET_VALUE, found->size);
            found->index = 0;
            found->remaining = found->size;
            found->next = nullptr;
            return found;
        }


        /// Allocate `size` bytes of memory with alignment `align`.
        uint8_t *allocate_bare(
            const unsigned align,
            const unsigned size
        ) throw() {

            last_allocation_size = 0;
            last_allocation = nullptr;
            last_allocation_slab = nullptr;

            unsigned slab_size(size + ALIGN_TO(size, SLAB_SIZE));
            if(!slab_size) {
                slab_size = SLAB_SIZE;
            }

            ASSERT(slab_size >= SLAB_SIZE);
            IF_TEST( int i(0); )
            for(; IF_TEST(i < 3); IF_TEST(++i)) {

                // Allocate a slab if we're missing one or if this slab appears
                // to be unable to service out current request.
                if(!curr || curr->remaining < size) {
                    bump_pointer_slab *new_curr(allocate_slab(slab_size));
                    new_curr->next = curr;
                    curr = new_curr;
                    if(!first) {
                        first = curr;
                    }
                }

                // Handle a staged allocation.
                if(!size) {
                    return &(curr->memory[curr->index]);
                }

                ASSERT(curr->index <= curr->size);
                ASSERT(curr->remaining <= curr->size);

                // Figure out how we need to align the memory returned from
                // the slab.
                const uintptr_t mem_addr(reinterpret_cast<uintptr_t>(
                    &(curr->memory[curr->index])));
                const unsigned align_offset(ALIGN_TO(mem_addr, align));
                const unsigned aligned_size(size + align_offset);

                if(curr->remaining >= aligned_size) {
                    curr->index += align_offset;
                    curr->remaining -= align_offset;
                    break;

                // Make this slab unusable, and maintain the invariant that
                // `size == (index + remaining)`.
                } else {
                    curr->remaining = 0;
                    curr->index = curr->size;
                    continue;
                }
            }

            ASSERT(2 >= i);
            ASSERT(curr->remaining >= size);

            uint8_t *ret(&(curr->memory[curr->index]));

#if CONFIG_ENABLE_ASSERTIONS
            for(i = 0; i < size; ++i) {
                ASSERT(MEMSET_VALUE == ret[i]);
            }
#endif
            last_allocation_size = size;
            last_allocation = ret;
            last_allocation_slab = curr;
            curr->index += size;
            curr->remaining -= size;

            ASSERT((curr->index + curr->remaining) == curr->size);
            ASSERT(0 == (reinterpret_cast<uintptr_t>(ret) % align));

            return ret;
        }


        /// Free a list of bump_pointer_slabs.
        static void free_slab_list(bump_pointer_slab *list) throw() {
            for(bump_pointer_slab *next(nullptr); list; list = next) {
                next = list->next;
                if(list->memory) {
                    if(IS_EXECUTABLE) {
                        detail::global_free_executable(
                            list->memory, list->size);
                    } else {
                        free_memory<uint8_t>(list->memory, list->size);
                    }
                    list->memory = nullptr;
                }

                free_memory<bump_pointer_slab>(list);
            }
        }

    public:

        bump_pointer_allocator(void) throw()
            : curr(nullptr)
            , first(nullptr)
            , free(nullptr)
            , lock()
            , last_allocation_size(0)
            , last_allocation(nullptr)
            , last_allocation_slab(nullptr)
            _IF_TEST( last_allocator(nullptr) )
            _IF_TEST( curr_owner_cpu_id(-1) )
        { }

        ~bump_pointer_allocator(void) throw() {
            free_slab_list(free);
            free = nullptr;
            last_allocation_size = 0;
            last_allocation = nullptr;
            last_allocation_slab = nullptr;
            free_slab_list(curr);
            curr = nullptr;
        }

        template <typename T, typename... Args>
        T *allocate(Args&&... args) throw() {
            enum {
                ALIGN = (unsigned) alignof(T),
                MIN_ALIGN_ = ALIGN < MIN_ALIGN ? (unsigned) MIN_ALIGN : ALIGN
            };

            IF_TEST( const void *allocator(__builtin_return_address(0)); )

            acquire();
            IF_TEST( last_allocator = allocator; )
            T *ptr(new (allocate_bare(MIN_ALIGN_, sizeof(T))) T(args...));
            release();
            return ptr;
        }

        void *allocate_untyped(unsigned align, unsigned num_bytes) throw() {
            IF_TEST( const void *allocator(__builtin_return_address(0)); )

            acquire();
            IF_TEST( last_allocator = allocator; )
            uint8_t *arena(allocate_bare(align, num_bytes));
            release();
            ASSERT(is_valid_address(arena));

            return arena;
        }

        template <typename T>
        inline const T *allocate_staged(void) throw() {
            IF_TEST( const void *allocator(__builtin_return_address(0)); )
            acquire();
            IF_TEST( last_allocator = allocator; )
            void *ret(allocate_bare(MIN_ALIGN, 0));
            release();
            return unsafe_cast<const T *>(ret);
        }

        template <typename T>
        T *allocate_array(unsigned length) throw() {
            enum {
                ALIGN = (unsigned) alignof(T),
                MIN_ALIGN_ = ALIGN < MIN_ALIGN ? (unsigned) MIN_ALIGN : ALIGN
            };

            IF_TEST( const void *allocator(__builtin_return_address(0)); )
            acquire();
            IF_TEST( last_allocator = allocator; )
            uint8_t *arena(allocate_bare(MIN_ALIGN_, sizeof(T) * length));
            release();

            // Initialise each element using placement new syntax; C++ standard
            // allows for placement new[] to introduce array length overhead.
            if(!std::is_trivial<T>::value) {
                T *ptr(unsafe_cast<T *>(arena));
                for(const T *last_ptr(ptr + length); ptr < last_ptr; ++ptr) {
                    new (ptr) T;
                }
            }

            return unsafe_cast<T *>(arena);
        }

        /// Free the last thing allocated. If that thing needed to be aligned
        /// then the alignment space is not freed.
        ///
        /// If this is a shared allocator, or if the allocator isn't shared but
        /// follows a locking discipline that uses `lock_coarse` (coarse-grained
        /// allocator locking) then this function should be used VERY carefully.
        void free_last(void) throw() {
            IF_TEST( const void *allocator(__builtin_return_address(0)); )
            acquire();
            IF_TEST( last_allocator = allocator; )

            if(curr
            && last_allocation_size
            && curr == last_allocation_slab) {

                ASSERT((curr->index + curr->remaining) == curr->size);
                ASSERT(curr->index >= last_allocation_size);
                ASSERT((last_allocation + last_allocation_size)
                    == &(curr->memory[curr->index]));
                curr->index -= last_allocation_size;
                curr->remaining += last_allocation_size;
                ASSERT(curr->index <= curr->size);
                ASSERT((curr->index + curr->remaining) == curr->size);

                memset(
                    &(curr->memory[curr->index]),
                    MEMSET_VALUE,
                    last_allocation_size);
            }

            last_allocation_size = 0;
            last_allocation = nullptr;
            last_allocation_slab = nullptr;
            release();
        }

        /// Free all allocated objects.
        void free_all(void) throw() {
            if(!IS_TRANSIENT || IS_SHARED) {
                FAULT;
            }

            IF_TEST( const void *allocator(__builtin_return_address(0)); )
            acquire();
            IF_TEST( last_allocator = allocator; )
            last_allocation_size = 0;
            last_allocation = nullptr;
            last_allocation_slab = nullptr;

            if(first) {
                ASSERT(!(first->next));
                ASSERT(curr);
                first->next = free;
                first = nullptr;
            }
            
            if(curr) {
                free = curr;
                curr = nullptr;
            }

            if(SHARE_DEAD_SLABS
            && free && global_free_lock.try_acquire()) {
                *(free->connect()) = global_free;
                global_free = free;
                global_free_lock.release();

                free = nullptr;
            }

            release();
        }


        /// Acquire a coarse-grained lock on the allocator. This allows us
        /// to mark an allocator as non-shared (even when it is), but do coarse-
        /// grained locking across multiple operations, rather than fine-grained
        /// locking around individual operations.
        void lock_coarse(IF_TEST(int cpu_id)) throw() {
#if CONFIG_ENABLE_TRACE_ALLOCATOR || CONFIG_FOLLOW_CONDITIONAL_BRANCHES
            if(!IS_SHARED) {
                lock.acquire();
                IF_TEST( curr_owner_cpu_id = cpu_id; )
            }
#elif CONFIG_ENABLE_ASSERTIONS
            UNUSED(cpu_id);
#endif
        }


        /// Release the coarse-grained lock.
        void unlock_coarse(void) throw() {
#if CONFIG_ENABLE_TRACE_ALLOCATOR || CONFIG_FOLLOW_CONDITIONAL_BRANCHES
            if(!IS_SHARED) {
                IF_TEST( curr_owner_cpu_id = -1; )
                lock.release();
            }
#endif
        }
    };

    template <typename Config>
    bump_pointer_slab *bump_pointer_allocator<Config>::global_free = nullptr;

    template <typename Config>
    atomic_spin_lock bump_pointer_allocator<Config>::global_free_lock;
}


#endif /* BUMP_ALLOCATOR_H_ */
