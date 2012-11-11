/*
 * list.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_LIST_H_
#define granary_LIST_H_

#include <cstddef>

#include "granary/heap.h"
#include "granary/type_traits.h"
#include "granary/utils.h"

namespace granary {

    /// configuration type for lists of type T. This enables singly and doubly
    /// linked lists, where list pointers are either embedded or super-imposed.
    template <typename T>
    struct list_meta {
        enum {
            EMBED = false,
            UNROLL = 1,
            NEXT_POINTER = true,
            PREV_POINTER = false,

            NEXT_POINTER_OFFSET = 0,
            PREV_POINTER_OFFSET = 0
        };

        static T *allocate(void) throw() {
            return heap_alloc(0, sizeof(T));
        }

        static void free(T *val) throw() {
            heap_free(0, val, sizeof *val);
        }
    };


    template <typename T, bool embed>
    struct list_item;

    /// base class for a list item with/out a previous pointer
    template <typename T, bool has_prev>
    struct list_item_with_prev;

    template <typename T>
    struct list_item_with_prev<T, true> {
    public:
        T *&get_prev(void) throw() {
            return *unsafe_cast<T **>(
                unsafe_cast<char *>(this) + list_meta<T>::PREV_POINTER_OFFSET);
        }
    };

    template <typename T>
    struct list_item_with_prev<T, false> { };

    /// base class for a list item with/out a previous pointer
    template <typename T, bool has_next>
    struct list_item_with_next;

    template <typename T>
    struct list_item_with_next<T, true> {
    public:
        T *&get_next(void) throw() {
            return *unsafe_cast<T **>(
                unsafe_cast<char *>(this) + list_meta<T>::NEXT_POINTER_OFFSET);
        }
    };

    template <typename T>
    struct list_item_with_next<T, false> { };

    template <typename T>
    struct list_item_with_links : public T {
    public:

        typedef list_meta<T> internal_meta;
        typedef list_item_with_links<T> self_type;

        self_type *links[
            internal_meta::NEXT_POINTER + internal_meta::PREV_POINTER];

        inline T get_item(void) const throw() {
            return *this;
        }
    };

    /// used for asserting that we don't accidentally choose the wrong method
    /// for get_item based on argument dependent lookup
    template <typename T>
    struct is_list_item_with_links {
    public:
        typedef std::false_type type;
        typedef T item_type;
        enum {
            value = false
        };
    };

    template <typename T>
    struct is_list_item_with_links<list_item_with_links<T> > {
    public:
        typedef std::true_type type;
        typedef T item_type;
        enum {
            value = true
        };
    };

    /// meta information about a list of T, where T does not have embedded
    /// next/prev pointers, but where list_item<T, false> does have embedded
    /// next/prev pointers.
    template <typename T>
    struct list_meta<list_item_with_links<T> > {
    public:
        typedef list_meta<T> internal_meta;
        typedef list_item_with_links<T> item_type;

        enum {
            EMBED = true,
            UNROLL = internal_meta::UNROLL,
            NEXT_POINTER = internal_meta::NEXT_POINTER,
            PREV_POINTER = internal_meta::PREV_POINTER,

            HAS_NEXT = internal_meta::NEXT_POINTER,
            HAS_PREV = internal_meta::PREV_POINTER,
            NEXT_OFFSET = offsetof(item_type, links),
            PREV_OFFSET =
                NEXT_OFFSET + (HAS_NEXT && HAS_PREV ? sizeof(item_type *) : 0UL),

            NEXT_POINTER_OFFSET = NEXT_OFFSET,
            PREV_POINTER_OFFSET = PREV_OFFSET
        };

        inline static item_type *allocate(void) throw() {
            return internal_meta::heap_alloc(0, sizeof(item_type));
        }

        inline static void free(item_type *val) throw() {
            internal_meta::heap_free(0, val, sizeof *val);
        }
    };

    /// an implementation of a linked list where next/previous pointers,
    /// if any, are embedded in T.
    template <typename T>
    struct list_item<T, true>
      : T
      , list_item_with_prev<T, list_meta<T>::PREV_POINTER>
      , list_item_with_next<T, list_meta<T>::NEXT_POINTER>
    {
    public:

        typedef list_item<T, true> self_type;

    private:

        inline static typename is_list_item_with_links<T>::item_type
        get_item(const self_type *self, std::true_type) throw() {
            return self->T::get_item();
        }

        inline static T get_item(const self_type *self, std::false_type) throw() {
            return *self;
        }

        typedef typename is_list_item_with_links<T>::type bool_type;

    public:

        inline decltype(get_item(nullptr, bool_type())) get_item(void) const throw() {
            return get_item(this, bool_type());
        }
    };

    /// an implementation of a linked list where the next/previous pointers
    /// are part of the list item, and not of T.
    template <typename T>
    struct list_item<T, false> : public list_item<list_item_with_links<T>, true> {
    public:
        typedef list_item<T, true> self_type;
    };

    /// represents a generic list of T, where the properties of the list are
    /// configured by specializing list_meta<T>.
    template <typename T>
    struct list {
    private:

        typedef list_item<T, list_meta<T>::EMBED> item_type;

        item_type *first_;
        item_type *last_;
        item_type *cache_;
        unsigned length_;

    public:

        list(void) throw()
            : first_(nullptr)
            , last_(nullptr)
            , length_(0U)
        { }

        /// Returns the number of elements in the list.
        inline unsigned length(void) const throw() {
            return length_;
        }

        /// Clear the elements of the list, with the intention of re-using the
        /// already allocated list elements later.
        void clear_for_reuse(void) throw() {

        }

        /// Clear the elements of the list, and release any memory associated
        /// with the elements of the list
        void clear(void) throw() {

        }

        /// Adds an element on to the end of the list.
        inline void append(T val) throw() {
            return insert_after(last(), val);
        }

        /// Adds an element on to the beginning of the list.
        inline void prepend(T val) throw() {
            return insert_before(first(), val);
        }

        /// Return the first element in the list.
        inline T first(void) const throw() {
            if(!first_) {
                return T();
            }

            return first_->get_item();
        }

        /// Return the last element in the list.
        inline T last(void) const throw() {
            if(!last_) {
                return T();
            }

            return last_->get_item();
        }

        /// Insert an element before another object in the list
        void insert_before(T pos, T val) throw() {
            (void) pos;
            (void) val;
        }

        /// Insert an element after another object in the list
        void insert_after(T pos, T val) throw() {
            (void) pos;
            (void) val;
        }

        /// replace one element with another
        void replace(T pos, T val) throw() {
            (void) pos;
            (void) val;
        }

    private:

        static item_type *pos_to_item(T pos) throw() {
            (void) pos;
            return 0;
        }
    };
}

#endif /* granary_LIST_H_ */
