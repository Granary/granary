/*
 * bump_allocator.h
 *
 *  Created on: Jan 11, 2013
 *      Author: pag
 */

#ifndef BUMP_ALLOCATOR_H_
#define BUMP_ALLOCATOR_H_

#include <new>
#include "granary/atomic.h"
#include "granary/type_traits.h"
#include "granary/utils.h"

namespace granary {


    /// Defines a generic bump pointer allocator. The allocator is configured
    /// by the Config type.
    ///
    /// The following config options must be present:
    ///     SLAB_SIZE:      A low bound on the number of bytes to allocate per slab
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
                        ? (SLAB_SIZE_ + ALIGN_TO(SLAB_SIZE_, PAGE_SIZE))
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
        slab *first;


        /// A free list of slabs.
        slab *free;


        /// Lock used to serialise allocations.
        opt_atomic<bool, IS_SHARED> lock;


        /// The size of the last allocation.
        unsigned last_allocation_size;


        /// Acquire a lock on the allocator.
        inline void acquire(void) throw() {
            while(lock.exchange(true)) { }
        }


        /// Release the lock on the allocator.
        inline void release(void) throw() {
            lock.store(false);
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
#if CONFIG_PRECISE_ALLOCATE
            (void) align;
            if(IS_EXECUTABLE) {
                return unsafe_cast<uint8_t *>(
                    detail::global_allocate_executable(ALIGN_TO(size, PAGE_SIZE)));
            } else {
                return unsafe_cast<uint8_t *>(detail::global_allocate(size));
            }
#else
            last_allocation_size = size;

            if(nullptr == curr) {
                allocate_slab();
                first = curr;

            } else if(curr->remaining < size) {
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
#endif
        }

        /// Free a list of slabs.
        static void free_slab_list(slab *list) throw() {
            for(slab *next(nullptr); list; list = next) {
                next = list->next;
                if(list->data_ptr) {
                    if(IS_EXECUTABLE) {
                        detail::global_free_executable(
                            list->data_ptr, SLAB_SIZE);
                    } else {
                        detail::global_free(list->data_ptr);
                    }
                    list->data_ptr = nullptr;
                    list->bump_ptr = nullptr;
                }
                detail::global_free(list);
            }
        }

    public:

        bump_pointer_allocator(void) throw()
            : curr(nullptr)
            , first(nullptr)
            , free(nullptr)
            , last_allocation_size(0)
        { }

        ~bump_pointer_allocator(void) throw() {
            // de-allocate the free list
            free_slab_list(free);
            free = nullptr;

            // de-allocate the active list
            free_slab_list(curr);
            curr = nullptr;
        }

        template <typename T, typename... Args>
        T *allocate(Args&&... args) throw() {
            enum {
                ALIGN = alignof(T),
                MIN_ALIGN = ALIGN < 16 && IS_EXECUTABLE ? 16 : ALIGN
            };
            acquire();
            T *ptr(new (allocate_bare(MIN_ALIGN, sizeof(T))) T(args...));
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
        inline T *allocate_staged(void) throw() {
            return unsafe_cast<T *>(allocate_bare(16, 0));
        }

        template <typename T>
        array<T> allocate_array(unsigned length) throw() {
            enum {
                ALIGN = alignof(T),
                MIN_ALIGN = (ALIGN < 16 && IS_EXECUTABLE) ? 16 : ALIGN
            };
            uint8_t *arena(allocate_bare(MIN_ALIGN, sizeof(T) * length));

            // initialise each element using placement new syntax; C++ standard
            // allows for placement new[] to introduce array length overhead.
            if(!std::is_trivial<T>::value) {
                T *ptr(unsafe_cast<T *>(arena));
                for(const T *last_ptr(ptr + length); ptr < last_ptr; ++ptr) {
                    new (ptr) T;
                }
            }
            return array<T>(unsafe_cast<T *>(arena), length);
        }

        /// Free the last thing allocated.
        void free_last(void) throw() {
            if(IS_SHARED) {
                FAULT;
            }

            if(curr && last_allocation_size) {
                curr->bump_ptr -= last_allocation_size;
                curr->remaining += last_allocation_size;
            }

            last_allocation_size = 0;
        }

        /// Free all allocated objects.
        void free_all(void) throw() {
            if(!IS_TRANSIENT || IS_SHARED) {
                FAULT;
            }

            if(first) {
                first->next = free;
                first = nullptr;
            }

            free = curr;
            curr = nullptr;
            last_allocation_size = 0;
        }
    };
}


#endif /* BUMP_ALLOCATOR_H_ */
