/*
 * heap.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_HEAP_H_
#define Granary_HEAP_H_

#include <new>

#include "granary/atomic.h"
#include "granary/type_traits.h"

#ifdef __cplusplus
extern "C" {
#endif

/// DynamoRIO-compatible heap allocation functions; these are from globally
/// known allocators.

void *heap_alloc(void *, unsigned long long);
void heap_free(void *, void *, unsigned long long);

#ifdef __cplusplus
}

namespace granary {

    namespace detail {

        /// Allocate some executable memory. It is assumed that size is
        /// sufficiently large to allow for both user space and kernel
        /// space allocation, and that the user of this allocator will
        /// handle page alignment, etc.
        void *global_allocate_executable(unsigned long size) throw();


        /// Allocate some non-executable memory.
        void *global_allocate(unsigned long size) throw();


        /// Free some globally allocated memory.
        void global_free(void *addr) throw();
    }

}


/// Overload operator new for global heap allocation.
inline void *operator new(size_t size) {
    return granary::detail::global_allocate(size);
}


/// Overload operator new for global heap allocation.
inline void *operator new[](size_t size) {
    return granary::detail::global_allocate(size);
}


/// Overload operator delete for global heap freeing.
inline void operator delete(void *addr) {
    granary::detail::global_free(addr);
}


/// Overload operator delete for global heap freeing.
inline void operator delete[](void *addr) {
    granary::detail::global_free(addr);
}


namespace granary {


    /// Defines a generic bump pointer allocator. The allocator is configured
    /// by the Config type.
    ///
    ///	The following config options must be present:
    ///		SLAB_SIZE: 		A low bound on the number of bytes to allocate per slab
    ///						of memory. This is a lower bound because allocation of
    ///						executable memory might require more memory to be
    ///						allocated.
    ///		EXECUTABLE: 	True iff the memory allocated must be executable.
    ///		TRANSIENT: 		True iff the memory allocated is considered short
    ///						lived. The implication is that bump-pointer memory
    ///						is either short lived (and can all be freed at once)
    ///						or persists indefinitely.
    ///     SHARED:         True iff this allocator is shared between cores/threads.
    ///
    /// The allocator has the property that the last allocation can optionally be
    /// released (e.g. in the event that we want to back out of the most recent
    /// allocation and reclaim the memory for future allocations). Note: the
    /// last allocation can only be released if the allocator is not shared.
    ///
    /// The allocator also has the property that it's greedy, i.e. even if the
    /// memory allocated is considered transient, the allocator will still hold on
    /// to all allocated slabs in its free list so that it can re-use it in later
    /// (transient) allocations. Obviously, if the memory is not transient, then
    /// the default greediness is desirable.
    template <typename Config>
    struct bump_pointer_allocator {
    private:

        enum {
            IS_EXECUTABLE = !!Config::EXECUTABLE,
            IS_TRANSIENT = !!Config::TRANSIENT,
            IS_SHARED = !!Config::SHARED,

            // Adjusted slab size for allocating executable memory.
            SLAB_SIZE_ = Config::SLAB_SIZE,
            SLAB_SIZE = IF_KERNEL_ELSE(
                    SLAB_SIZE_,
                    IS_EXECUTABLE
                    ? (SLAB_SIZE_ + ALIGN_TO(SLAB_SIZE_, PAGE_SIZE) + PAGE_SIZE)
                            : SLAB_SIZE_),

                              // Alignment within the slab for the first allocated
                              // memory
                              FIRST_ALLOCATION_ALIGN = IS_EXECUTABLE ? PAGE_SIZE : 16
        };


        /// Represents a single slab of bump-pointer allocatable memory.
        /// No attempt is made to pack as much into the slabs as possible.
        struct slab {
            slab *next;
            uint8_t *data_ptr;
            uint8_t *bump_ptr;
            unsigned remaining;
        };


        /// The next slab from which we can allocate.
        slab *curr;


        /// A free list of slabs.
        slab *free;


        /// Lock used to serialize allocations.
        opt_atomic<bool, IS_SHARED> lock;


        /// The size of the last allocation.
        unsigned last_allocation_size;


        /// Acquire a lock on the allocator.
        inline void acquire(void) throw() {
            while(lock.exchange(true)) { }
        }


        /// Release the lock on the allocator.
        inline void release(void) throw() {
            lock = false;
        }


        /// Allocate a slab and a corresponding memory arena.
        void allocate_slab(void) throw() {
            slab *next_slab(free);

            // take from the free list
            if(next_slab) {
                free = next_slab->next;

            // allocate a new slab and a new arena for the memory
            } else {
                next_slab = new (detail::global_allocate(sizeof(slab))) slab;

                if(IS_EXECUTABLE) {
                    next_slab->data_ptr = unsafe_cast<uint8_t *>(
                        detail::global_allocate_executable(SLAB_SIZE));
                } else {
                    next_slab->data_ptr = unsafe_cast<uint8_t *>(
                        detail::global_allocate(SLAB_SIZE));
                }
            }

            next_slab->next = curr;
            curr = next_slab;
            curr->bump_ptr = curr->data_ptr;
            curr->remaining = SLAB_SIZE;
        }


        /// Allocate `size` bytes of memory with alignment `align`.
        uint8_t *allocate_bare(const unsigned align, const unsigned size) throw() {

            last_allocation_size = size;

            if(nullptr == curr || curr->remaining < size) {
                allocate_slab();
            } else {
                uint64_t next_address(reinterpret_cast<uint64_t>(curr->bump_ptr));
                unsigned align_offset(ALIGN_TO(next_address, align));
                unsigned aligned_size(size + align_offset);

                if(curr->remaining < aligned_size) {
                    allocate_slab();
                } else {
                    curr->bump_ptr += align_offset;
                    last_allocation_size += align_offset;
                }
            }

            uint8_t *bumped_ptr(curr->bump_ptr);

            curr->bump_ptr += size;
            curr->remaining -= last_allocation_size;

            return bumped_ptr;
        }

    public:

        bump_pointer_allocator(void)
            : curr(nullptr)
            , free(nullptr)
            , last_allocation_size(0)
            { }

        template <typename T, typename... Args>
        T *allocate(Args&&... args) throw() {
            acquire();
            T *ptr(new (allocate(alignof(T), sizeof(T))) T(args...));
            release();
            return ptr;
        }

        void *allocate_untyped(unsigned align, unsigned num_bytes) throw() {
            acquire();
            uint8_t *arena(allocate_bare(align, num_bytes));
            release();
            memset(arena, 0, num_bytes);
            return arena;
        }

        template <typename T>
        T *allocate_array(unsigned length) throw() {
            uint8_t *arena(allocate_bare(alignof(T), sizeof(T) * length));

            // initialize each element using placement new syntax; C++ standard
            // allows for placement new[] to introduce array length overhead.
            if(!std::is_trivial<T>::value) {
                T *ptr(unsafe_cast<T *>(arena));
                for(const T *last_ptr(ptr + length); ptr < last_ptr; ++ptr) {
                    new (ptr) T;
                }
            }
            return unsafe_cast<T *>(arena);
        }

        void free_last(void) throw() {
            if(IS_SHARED) {
                FAULT;
            }

            curr->bump_ptr -= last_allocation_size;
            curr->remaining += last_allocation_size;
            last_allocation_size = 0;
        }

        void free_all(void) throw() {
            if(!IS_TRANSIENT || IS_SHARED) {
                FAULT;
            }
            free = curr;
            curr = nullptr;
            last_allocation_size = 0;
        }
    };
}

#endif


#endif /* Granary_HEAP_H_ */
