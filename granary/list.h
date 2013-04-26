/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * list.cc
 *
 *  Created on: 2013-01-13
 *      Author: pag
 *     Version: $Id$
 */


#ifndef granary_LIST_H_
#define granary_LIST_H_

#include "granary/globals.h"

namespace granary {

    template <typename> struct list_item;
    template <typename T, bool, bool> struct list_item_impl;

    template <typename T>
    struct list_meta {
    public:

        enum {
            HAS_NEXT = false,
            HAS_PREV = false,
            NEXT_POINTER_OFFSET = 0,
            PREV_POINTER_OFFSET = 0
        };

        static void *allocate(unsigned size) throw() {
            return detail::global_allocate(size);
        }

        static void free(void *val, unsigned) throw() {
            detail::global_free(val);
        }
    };

    template <typename T>
    struct list_item_impl<T, false, false> {
    public:
        T value;
        list_item<T> *next;
        list_item<T> *prev;

        inline T &get_value(void) throw() {
            return value;
        }

        inline list_item<T> *&get_next(void) throw() {
            return next;
        }

        inline list_item<T> *&get_prev(void) throw() {
            return prev;
        }
    };

    template <typename T>
    struct list_item_impl<T, false, true> {
    public:
        union {
            T value;
            char bytes[sizeof(T)];
        };

        list_item<T> *prev;

        inline T &get_value(void) throw() {
            return value;
        }

        inline list_item<T> *&get_next(void) throw() {
            return *unsafe_cast<list_item<T> **>(
                &(bytes[list_meta<T>::NEXT_POINTER_OFFSET]));
        }

        inline list_item<T> *&get_prev(void) throw() {
            return prev;
        }
    };

    template <typename T>
    struct list_item_impl<T, true, false> {
    public:
        union {
            T value;
            char bytes[sizeof(T)];
        };

        list_item<T> *next;

        inline T &get_value(void) throw() {
            return value;
        }

        inline list_item<T> *&get_prev(void) throw() {
            return *unsafe_cast<list_item<T> **>(
                &(bytes[list_meta<T>::PREV_POINTER_OFFSET]));
        }

        inline list_item<T> *&get_next(void) throw() {
            return next;
        }
    };

    template <typename T>
    struct list_item_impl<T, true, true> {
    public:
        union {
            T value;
            char bytes[sizeof(T)];
        };

        T &get_value(void) throw() {
            return value;
        }

        inline list_item<T> *&get_next(void) throw() {
            return *unsafe_cast<list_item<T> **>(
                &(bytes[list_meta<T>::NEXT_POINTER_OFFSET]));
        }

        inline list_item<T> *&get_prev(void) throw() {
            return *unsafe_cast<list_item<T> **>(
                &(bytes[list_meta<T>::PREV_POINTER_OFFSET]));
        }
    };


    /// Represents an item in the list.
    template <typename T>
    struct list_item {
    public:
        typedef list_meta<T> meta_type;
        typedef list_item_impl<
            T,
            meta_type::HAS_PREV,
            meta_type::HAS_NEXT
        > item_type;

        item_type item;

        inline T &get_value(void) throw() {
            return item.get_value();
        }

        inline list_item<T> *&get_next(void) throw() {
            return item.get_next();
        }

        inline list_item<T> *&get_prev(void) throw() {
            return item.get_prev();
        }
    };


    /// represents a handle to a list item
    template <typename T>
    struct list_item_handle {
    protected:

        template <typename> friend struct list;

        list_item<T> *handle;

        list_item_handle(list_item<T> *handle_) throw()
            : handle(handle_)
        { }

    public:

        list_item_handle(void) throw()
            : handle(nullptr)
        { }

        inline T &operator*(void) throw() {
            return handle->get_value();
        }

        inline T *operator->(void) throw() {
            return &(handle->get_value());
        }

        inline bool is_valid(void) const throw() {
            return nullptr != handle;
        }

        inline list_item_handle next(void) throw() {
            return list_item_handle(handle->get_next());
        }

        inline list_item_handle prev(void) throw() {
            return list_item_handle(handle->get_prev());
        }
    };

    /// represents a generic list of T, where the properties of the list are
    /// configured by specializing list_meta<T>.
    template <typename T>
    struct list {
    public:

        typedef list<T> self_type;
        typedef list_meta<T> meta_type;
        typedef list_item<T> item_type;
        typedef list_item_handle<T> handle_type;

    protected:

        item_type *first_;
        item_type *last_;
        item_type *cache_;
        unsigned length_;

        /// allocate a new list item
        virtual item_type *allocate_(T val) throw() {
            item_type *ptr(nullptr);
            if(nullptr != cache_) {
                ptr = cache_;
                cache_ = ptr->get_next();
                memset(ptr, 0, sizeof(item_type));
            } else {
                ptr = (item_type *) meta_type::allocate(sizeof(item_type));
            }

            ptr->get_value() = val;
            return ptr;
        }

        inline static item_type *allocate(self_type *this_, T val) throw() {
            return this_->allocate_(val);
        }

        /// cache an item for later use/allocation
        void cache(item_type *item) throw() {
            item->get_prev() = nullptr;
            item->get_next() = cache_;
            cache_ = item;
        }

    public:

        /// Initialise an empty list.
        list(void) throw()
            : first_(nullptr)
            , last_(nullptr)
            , cache_(nullptr)
            , length_(0U)
        { }

        /// Move constructor.
        list(self_type &&that) throw()
            : first_(that.first_)
            , last_(that.last_)
            , cache_(that.cache_)
            , length_(that.length_)
        {
            that.first_ = nullptr;
            that.last_ = nullptr;
            that.cache_ = nullptr;
            that.length_ = 0U;
        }


        /// Destroy the list by clearing all items.
        virtual ~list(void) throw() {
            clear();
        }

        /// Returns the number of elements in the list.
        inline unsigned length(void) const throw() {
            return length_;
        }

        /// Clear the elements of the list, with the intention of re-using the
        /// already allocated list elements later.
        void clear_for_reuse(void) throw() {
            if(!first_) {
                return;
            }

            last_->get_next() = cache_;
            cache_ = first_;
            first_ = nullptr;
            last_ = nullptr;
            length_ = 0;
        }

        /// Clear the elements of the list, and release any memory associated
        /// with the elements of the list
        void clear(void) throw() {
            if(!first_) {
                return;
            }

            // clear out all normal items
            item_type *item(first_);
            item_type *next(nullptr);
            for(; nullptr != item; item = next) {
                next = item->get_next();
                meta_type::free(item, sizeof *item);
            }

            // clear out any cached items
            item = cache_;
            next = nullptr;
            for(; nullptr != item; item = next) {
                next = item->get_next();
                meta_type::free(item, sizeof *item);
            }

            first_ = nullptr;
            last_ = nullptr;
            cache_ = nullptr;
            length_ = 0;
        }

        inline handle_type append(T val) throw() {
            return append(allocate(this, val));
        }


        inline handle_type prepend(T val) throw() {
            return prepend(allocate(this, val));
        }


        /// Return the first element in the list.
        inline handle_type first(void) const throw() {
            if(!first_) {
                return handle_type();
            }

            return handle_type(first_);
        }

        /// Return the last element in the list.
        inline handle_type last(void) const throw() {
            if(!last_) {
                return handle_type();
            }

            return handle_type(last_);
        }

        inline handle_type insert_before(handle_type pos, T val) throw() {
            return insert_before(pos.handle, val);
        }

        inline handle_type insert_after(handle_type pos, T val) throw() {
            return insert_after(pos.handle, val);
        }

    protected:

        /// Adds an element on to the end of the list.
        handle_type append(item_type *item) throw() {

            item_type *before_item(last_);

            last_ = item;
            item->get_prev() = before_item;
            item->get_next() = nullptr;
            if(before_item) {
                before_item->get_next() = item;
            }

            if(1 == ++length_) {
                first_ = item;
            }

            return handle_type(item);
        }

        /// Adds an element on to the beginning of the list.
        handle_type prepend(item_type *item) throw() {
            item_type *after_item(first_);

            first_ = item;
            item->get_next() = after_item;
            item->get_prev() = nullptr;

            if(after_item) {
                after_item->get_prev() = item;
            }

            if(1 == ++length_) {
                last_ = item;
            }

            return handle_type(item);
        }

        inline handle_type insert_before(item_type *at_pos, T val) throw() {
            return insert_before(at_pos, allocate(this, val));
        }

        /// Insert an element before another object in the list.
        handle_type insert_before(item_type *after_item, item_type *item) throw() {

            if(1 >= length_ || !after_item) {
                return prepend(item);
            }

            //         <----    <----
            // <before_item> <item> <after_item>
            //           ---->  ---->

            item_type *before_item(after_item->get_prev());
            if(!before_item) {
                return prepend(item);
            }

            return chain(before_item, item, after_item);
        }

        inline handle_type insert_after(item_type *at_pos, T val) throw() {
            return insert_after(at_pos, allocate(this, val));
        }

        /// Insert an element after another object in the list
        handle_type insert_after(item_type *before_item, item_type *item) throw() {

            if(1 >= length_ || !before_item) {
                return append(item);
            }

            item_type *after_item(before_item->get_next());
            if(!after_item) {
                return append(item);
            }

            return chain(before_item, item, after_item);
        }

        /// Chain an element into the list.
        handle_type chain(
            item_type *before_item,
            item_type *item,
            item_type *after_item
        ) throw() {
            ++length_;

            item->get_prev() = before_item;
            before_item->get_next() = item;

            item->get_next() = after_item;
            after_item->get_prev() = item;

            return handle_type(item);
        }


        inline item_type *get_item(handle_type handle) {
            return handle.handle;
        }

        // TODO: add list remove
    };
}

#endif /* granary_LIST_H_ */
