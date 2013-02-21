/*
 * state.cc
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
    template <typename T, unsigned, unsigned> struct list_item_impl;

    template <typename T>
    struct list_meta {
    public:

        enum {
            HAS_NEXT = 0,
            HAS_PREV = 0,
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
    struct list_item_impl<T, 0, 0> {
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
    struct list_item_impl<T, 0, 1> {
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
    struct list_item_impl<T, 1, 0> {
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
    struct list_item_impl<T, 1, 1> {
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

    private:

        item_type *first_;
        item_type *last_;
        item_type *cache_;
        unsigned length_;

    protected:

        /// allocate a new list item
        item_type *allocate(T val) throw() {
            item_type *ptr(nullptr);
            if(nullptr != cache_) {
                ptr = cache_;
                cache_ = ptr->get_next();
            } else {
                ptr = (item_type *) meta_type::allocate(sizeof(item_type));
            }

            ptr->get_value() = val;
            ptr->get_next() = nullptr;
            ptr->get_prev() = nullptr;

            return ptr;
        }

        /// cache an item for later use/allocation
        void cache(item_type *item) throw() {
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
        ~list(void) throw() {
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
            return append(allocate(val));
        }


        inline handle_type prepend(T val) throw() {
            return prepend(allocate(val));
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
            item_type *prev_last = last_;

            if(nullptr == first_) {
                first_ = item;
                last_ = item;
            } else {
                last_ = item;
            }

            // next
            item->get_next() = nullptr;
            if(prev_last) {
                prev_last->get_next() = item;
            }

            // prev
            item->get_prev() = prev_last;

            ++length_;

            return handle_type(item);
        }

        /// Adds an element on to the beginning of the list.
        handle_type prepend(item_type *item) throw() {
            item_type *prev_first = first_;

            if(nullptr == first_) {
                first_ = item;
                last_ = item;
            } else {
                first_ = item;
            }

            // next
            item->get_next() = prev_first;

            // prev
            item->get_prev() = nullptr;
            if(prev_first) {
                prev_first->get_prev() = item;
            }

            ++length_;

            return handle_type(item);
        }

        inline handle_type insert_before(item_type *at_pos, T val) throw() {
            return insert_before(at_pos, allocate(val));
        }

        /// Insert an element before another object in the list.
        handle_type insert_before(item_type *at_pos, item_type *item) throw() {
            if(1 >= length_ || nullptr == at_pos) {
                return prepend(item);
            }

            item_type *before_pos(at_pos->get_prev());
            if(at_pos) {

                ++length_;

                at_pos->get_prev() = item;
                item->get_next() = at_pos;
                item->get_prev() = before_pos;

                if(before_pos) {
                    before_pos->get_next() = item;
                }

                if(at_pos == first_) {
                    first_ = item;
                }

                return handle_type(item);
            } else {
                return prepend(item);
            }
        }

        inline handle_type insert_after(item_type *at_pos, T val) throw() {
            return insert_after(at_pos, allocate(val));
        }

        /// Insert an element after another object in the list
        handle_type insert_after(item_type *at_pos, item_type *item) throw() {
            if(1 >= length_ || nullptr == at_pos) {
                return append(item);
            }

            item_type *after_pos(at_pos->get_next());

            if(at_pos) {

                ++length_;

                at_pos->get_next() = item;
                item->get_prev() = at_pos;
                item->get_next() = after_pos;

                if(after_pos) {
                    after_pos->get_prev() = item;
                }

                if(at_pos == last_) {
                    last_ = item;
                }

                return handle_type(item);
            } else {
                return append(item);
            }
        }

        inline item_type *get_item(handle_type handle) {
            return handle.handle;
        }

        // TODO: add list remove
    };
}

#endif /* granary_LIST_H_ */
