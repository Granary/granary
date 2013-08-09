/*
 * kernel_type_id.h
 *
 *  Created on: 2013-08-08
 *      Author: akshayk
 */

#ifndef KERNEL_TYPE_ID_H_
#define KERNEL_TYPE_ID_H_

template <typename T>
struct type_id {
    enum {
        VALUE = 0
    };
};

template <typename T>
struct type_id<const T> {
    enum {
        VALUE = type_id<T>::VALUE
    };
};

template <typename T>
struct type_id<volatile T> {
    enum {
        VALUE = type_id<T>::VALUE
    };
};

template <typename T>
struct type_id<const volatile T> {
    enum {
        VALUE = type_id<T>::VALUE
    };
};

#define DEFINE_TYPE(id, type) \
    template <> \
    struct type_id<type> { \
    public: \
        enum { \
            VALUE = id \
        }; \
    };



#endif /* KERNEL_TYPE_ID_H_ */
