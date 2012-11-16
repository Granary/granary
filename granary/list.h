/*
 * list.h
 *
 * This file defines a generic list structure. The list structure is configured
 * by specializing the list_meta template. Specializing this template allows the
 * list to adapt to elements with/out embedded pointers, as well as to super-
 * impose traversal pointers.
 *
 * Note:
 *      - This class uses value semantics in many places. At low levels of
 *        optimizations, this is likely to result in performance degradation.
 *      - This class assumes that you have either no pointers, a next pointer,
 *        or a next pointer and a previous pointer.
 *      - This class is not thread safe.
 *
 * Limitations:
 *      - If one type of traversal pointer is embedded, then this class cannot
 *        super-impose the opposite kind of traversal pointer.
 *      - This class is meant to contain structs/classes.
 *      - The type being put into the list must be default constructible and
 *        copy constructible.
 *
 *      - Currently this implementation only works for doubly linked lists!
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_LIST_H_
#define granary_LIST_H_

#include "granary/globals.h"

namespace granary {

    /// forward declaration
    template <typename> struct list;


    /// configuration type for lists of type T. This enables singly and doubly
    /// linked lists, where list pointers are either embedded or super-imposed.
    template <typename T>
    struct list_meta {
        enum {
            EMBED = false,
            UNROLL = 1,

            NEXT_POINTER_OFFSET = 0,
            PREV_POINTER_OFFSET = 0
        };

        static void *allocate(unsigned size) throw() {
            return heap_alloc(0, size);
        }

        static void free(void *val, unsigned size) throw() {
            heap_free(0, val, size);
        }
    };


    template <typename T, bool embed>
    struct list_item;


    template <typename T>
    struct list_item_with_links;


    template <typename T, typename ItemT>
    struct list_item_handle;


    /// base class for a list item with/out a previous pointer
    template <typename ItemT, typename T, bool has_prev>
    struct list_item_with_prev;


    template <typename ItemT, typename T>
    struct list_item_with_prev<ItemT, T, true> {
    private:

        template <typename> friend struct list;
        template <typename, typename> friend struct list_item_handle;
        template <typename, typename, bool> friend struct list_item_with_next;

        inline ItemT **prev_pointer(void) throw() {
            enum {
                OFFSET = list_meta<T>::PREV_POINTER_OFFSET
            };
            char *byte_this(unsafe_cast<char *>(this));
            return unsafe_cast<ItemT **>(byte_this + OFFSET);
        }

    public:

        inline ItemT prev(void) const throw() {
            enum {
                OFFSET = list_meta<T>::PREV_POINTER_OFFSET
            };
            const char *byte_this(unsafe_cast<const char *>(this));
            const ItemT *ptr(*unsafe_cast<const ItemT **>(byte_this + OFFSET));
            if(nullptr == ptr) {
                ItemT prev;

                if(list_meta<T>::NEXT_POINTER) {
                    *(prev.next_pointer()) = unsafe_cast<ItemT *>(this);
                }

                return prev;
            }
            return *ptr;
        }
    };


    template <typename ItemT, typename T>
    struct list_item_with_prev<ItemT, T, false> {
    private:

        template <typename> friend struct list;
        template <typename, typename> friend struct list_item_handle;
        template <typename, typename, bool> friend struct list_item_with_next;

        inline ItemT **prev_pointer(void) throw() {
            return nullptr;
        }
    };


    /// base class for a list item with/out a previous pointer
    template <typename ItemT, typename T, bool has_next>
    struct list_item_with_next;


    template <typename ItemT, typename T>
    struct list_item_with_next<ItemT, T, true> {
    private:

        template <typename> friend struct list;
        template <typename, typename> friend struct list_item_handle;
        template <typename, typename, bool> friend struct list_item_with_prev;

        inline ItemT **next_pointer(void) throw() {
            enum {
                OFFSET = list_meta<T>::NEXT_POINTER_OFFSET
            };
            char *byte_this(unsafe_cast<char *>(this));
            return unsafe_cast<ItemT **>(byte_this + OFFSET);
        }

    public:

        inline ItemT next(void) const throw() {
            enum {
                OFFSET = list_meta<T>::NEXT_POINTER_OFFSET
            };
            const char *byte_this(unsafe_cast<const char *>(this));
            const ItemT *ptr(*unsafe_cast<const ItemT **>(byte_this + OFFSET));
            if(nullptr == ptr) {
                ItemT next;
                *(next.prev_pointer()) = unsafe_cast<ItemT *>(this);
                return next;
            }
            return *ptr;
        }
    };


    template <typename ItemT, typename T>
    struct list_item_with_next<ItemT, T, false> {
    private:
        template <typename> friend struct list;
        template <typename, typename> friend struct list_item_handle;
        template <typename, typename, bool> friend struct list_item_with_prev;

        inline ItemT **next_pointer(void) throw() {
            return nullptr;
        }
    };


    /// Here we super-impose link pointers onto the base class of type T.
    template <typename T>
    struct list_item_with_links {
    public:

        typedef list_item<T, false> pointer_type;

        T val;
        pointer_type *next_;
        pointer_type *prev_;

        list_item_with_links(void)
            : val()
            , next_(nullptr)
            , prev_(nullptr)
        { }

        inline T operator*(void) const throw() {
            return this->val;
        }
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
            NEXT_POINTER_OFFSET = offsetof(item_type, next_),
            PREV_POINTER_OFFSET = offsetof(item_type, prev_)
        };

        inline static void *allocate(unsigned size) throw() {
            return internal_meta::allocate(0, size);
        }

        inline static void free(void *val, unsigned size) throw() {
            internal_meta::free(val, size);
        }
    };


    /// An implementation of a linked list where next/previous pointers,
    /// if any, are embedded in T.
    ///
    /// Note:
    ///     We depend on the list_item_with_* base classes occupying zero
    ///     size, which makes this class essentially T.
    template <typename T>
    struct list_item<T, true>
        : list_item_with_prev<list_item<T, true>, T, true>
        , list_item_with_next<list_item<T, true>, T, true>
    {
    public:

        typedef list_item<T, true> self_type;

    private:

        template <typename> friend class list;

        T val;

        /// get a raw pointer to the value, where the value does not have
        /// embedded navigation pointers.
        inline static T *get_raw_pointer(self_type *self) throw() {
            return &(self->val);
        }

    public:

        list_item(void)
            : val()
        { }

        inline T &operator*(void) throw() {
            return val;
        }

        inline T *operator->(void) throw() {
            return &(val);
        }
    };


    /// an implementation of a linked list where the next/previous pointers
    /// are part of the list item, and not of T.
    template <typename T>
    struct list_item<T, false>
        : list_item_with_prev<list_item<T, false>, T, true>
        , list_item_with_next<list_item<T, false>, T, true>
    {
    public:

        typedef list_item<T, false> self_type;

    private:

        template <typename> friend class list;

        list_item_with_links<T> val;

        /// get a raw pointer to the value, where the value does not have
        /// embedded navigation pointers.
        inline static T *get_raw_pointer(self_type *self) throw() {
            return &(self->val.val);
        }

    public:

        list_item(void)
            : val()
        { }

        inline T &operator*(void) throw() {
            return val.val;
        }

        inline T *operator->(void) throw() {
            return &(val.val);
        }
    };


    /// represents a handle to a list item
    template <typename T, typename ItemT>
    struct list_item_handle {
    protected:

        template <typename> friend struct list;

        ItemT *handle;

        list_item_handle(ItemT *handle_) throw()
            : handle(handle_)
        { }

    public:

        list_item_handle(void) throw()
            : handle(nullptr)
        { }

        T &operator*(void) throw() {
            return (**handle);
        }

        T *operator->(void) throw() {
            return &(**handle);
        }

        list_item_handle next(void) throw() {
            return list_item_handle(*(handle->next_pointer()));
        }

        list_item_handle prev(void) throw() {
            return list_item_handle(*(handle->prev_pointer()));
        }
    };


    /// represents a generic list of T, where the properties of the list are
    /// configured by specializing list_meta<T>.
    template <typename T>
    struct list {
    public:

        typedef list<T> self_type;
        typedef list_meta<T> meta_type;
        typedef list_item<T, meta_type::EMBED> item_type;
        typedef list_item_handle<T, item_type> handle_type;

    private:

        item_type *first_;
        item_type *last_;
        item_type *cache_;
        unsigned length_;

        /// get a pointer to either one of the next/previous pointers in an
        /// item.
        inline static item_type **get_link_pointer(item_type *item) throw() {
            return item->next_pointer();
        }

    protected:

        /// allocate a new list item
        item_type *allocate(T val) throw() {
            item_type *ptr(nullptr);
            if(nullptr != cache_) {
                ptr = cache_;
                cache_ = *get_link_pointer(ptr);
            } else {
                ptr = (item_type *) meta_type::allocate(sizeof(item_type));
            }

            *item_type::get_raw_pointer(ptr) = val;
            *(ptr->next_pointer()) = nullptr;
            *(ptr->prev_pointer()) = nullptr;
            return ptr;
        }

        /// cache an item for later use/allocation
        void cache(item_type *item) throw() {
            *get_link_pointer(item) = cache_;
            cache_ = item;
        }

    public:

        /// Initialize an empty list.
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

            *get_link_pointer(last_) = cache_;
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
                next = *get_link_pointer(item);
                meta_type::free(item, sizeof *item);
            }

            // clear out any cached items
            item = cache_;
            next = nullptr;
            for(; nullptr != item; item = next) {
                next = *get_link_pointer(item);
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
        inline item_type last(void) const throw() {
            if(!last_) {
                return item_type();
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
            *(item->next_pointer()) = nullptr;
            if(prev_last) *(prev_last->next_pointer()) = item;

            // prev
            *(item->prev_pointer()) = prev_last;

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
            *(item->next_pointer()) = prev_first;

            // prev
            *(item->prev_pointer()) = nullptr;
            if(prev_first) *(prev_first->prev_pointer()) = item;

            ++length_;

            return handle_type(item);
        }

        inline handle_type insert_before(item_type *at_pos, T val) throw() {
            return insert_before(at_pos, allocate(val));
        }

        /// Insert an element before another object in the list.
        handle_type insert_before(item_type *at_pos, item_type *item) throw() {
            if(1 >= length_) {
                return prepend(item);
            }

            item_type *before_pos(*(at_pos->prev_pointer()));
            item_type *after_pos(*(at_pos->next_pointer()));

            if(at_pos) {

                ++length_;

                *(at_pos->prev_pointer()) = item;
                *(item->next_pointer()) = at_pos;
                *(item->prev_pointer()) = before_pos;

                if(before_pos) {
                    *(before_pos->next_pointer()) = item;
                }

                if(at_pos == first_) {
                    first_ = item;
                }

                return handle_type(item);
            } else {
                return prepend(item);
            }

            (void) after_pos;
        }

        inline handle_type insert_after(item_type *at_pos, T val) throw() {
            return insert_after(at_pos, allocate(val));
        }

        /// Insert an element after another object in the list
        handle_type insert_after(item_type *at_pos, item_type *item) throw() {
            if(1 >= length_) {
                return append(item);
            }

            item_type *before_pos(*(at_pos->prev_pointer()));
            item_type *after_pos(*(at_pos->next_pointer()));

            if(at_pos) {

                ++length_;

                *(at_pos->next_pointer()) = item;
                *(item->prev_pointer()) = at_pos;
                *(item->next_pointer()) = after_pos;

                if(after_pos) {
                    *(after_pos->prev_pointer()) = item;
                }

                if(at_pos == last_) {
                    last_ = item;
                }

                return handle_type(item);
            } else {
                return append(item);
            }

            (void) before_pos;
        }

        inline item_type *get_item(handle_type handle) {
            return handle.handle;
        }
    };
}

#endif /* granary_LIST_H_ */
