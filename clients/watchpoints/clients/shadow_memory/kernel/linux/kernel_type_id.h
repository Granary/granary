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

template<bool B, class T = void>
struct enable_if {};

template<class T>
struct enable_if<true, T> { typedef T type; };

template<class T>
struct enable_if<false, T> { typedef T type; };

#define MODULE_TYPE_ID(type_name) \
    template <> \
    struct type_id<struct type_name> { \
    public: \
        enum { \
            VALUE = type_name##_ID \
        }; \
    };

#if CONFIG_ENV_KERNEL
#   include "clients/watchpoints/clients/shadow_memory/kernel/linux/kernel_type_descriptor.h"
#else
#   error "User space type shadows are not yet supported."
#endif

#undef MODULE_TYPE_ID

const uint16_t TYPE_SIZES[] = {
    0, // NOTE: type id == 0 is an undefined type!

#define MODULE_TYPE_ID(type_name) sizeof(struct type_name),
#if CONFIG_ENV_KERNEL
#   include "clients/watchpoints/clients/shadow_memory/kernel/linux/kernel_type_descriptor.h"
#else
#   error "User space type shadows are not yet supported."
#endif
    0
};

#endif /* KERNEL_TYPE_ID_H_ */
