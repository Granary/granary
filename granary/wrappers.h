/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: May 9, 2013
 *      Author: Peter Goodman
 */

#ifndef granary_BASIC_WRAPPERS_H_
#define granary_BASIC_WRAPPERS_H_


#ifdef APP_WRAPPER_FOR_pointer
    POINTER_WRAPPER_QUAL(const, {
        INHERIT_INOUT
    })

    POINTER_WRAPPER_QUAL(volatile, {
        INHERIT_INOUT
    })

    POINTER_WRAPPER_QUAL(const volatile, {
        INHERIT_INOUT
    })
#endif


namespace granary {

/// Basic type wrappers for POINTER TYPES are defined here so that client code
/// can override the wrappers for the basic pointer types should they desire.

#ifndef APP_WRAPPER_FOR_void_pointer
    WRAP_BASIC_TYPE(void)
#endif

#ifndef APP_WRAPPER_FOR_int8_t_pointer
    static_assert(sizeof(char) == sizeof(int8_t), "Type size mismatch.");
    WRAP_BASIC_TYPE(char)
#endif

#ifndef APP_WRAPPER_FOR_uint8_t_pointer
    static_assert(sizeof(char) == sizeof(int8_t), "Type size mismatch.");
    WRAP_BASIC_TYPE(unsigned char)
#endif

#ifndef APP_WRAPPER_FOR_int16_t_pointer
    static_assert(sizeof(short) == sizeof(int16_t), "Type size mismatch.");
    WRAP_BASIC_TYPE(short)
#endif

#ifndef APP_WRAPPER_FOR_uint16_t_pointer
    static_assert(sizeof(short) == sizeof(int16_t), "Type size mismatch.");
    WRAP_BASIC_TYPE(unsigned short)
#endif

#ifndef APP_WRAPPER_FOR_int32_t_pointer
    static_assert(sizeof(int) == sizeof(int32_t), "Type size mismatch.");
    WRAP_BASIC_TYPE(int)
#endif

#ifndef APP_WRAPPER_FOR_uint32_t_pointer
    static_assert(sizeof(int) == sizeof(int32_t), "Type size mismatch.");
    WRAP_BASIC_TYPE(unsigned)
#endif

#ifndef APP_WRAPPER_FOR_int64_t_pointer
    static_assert(sizeof(long) == sizeof(int64_t), "Type size mismatch.");
    WRAP_BASIC_TYPE(long)
#endif

#ifndef APP_WRAPPER_FOR_uint64_t_pointer
    static_assert(sizeof(long) == sizeof(int64_t), "Type size mismatch.");
    WRAP_BASIC_TYPE(unsigned long)
#endif

#ifndef APP_WRAPPER_FOR_float_pointer
    WRAP_BASIC_TYPE(float)
#endif

#ifndef APP_WRAPPER_FOR_double_pointer
    WRAP_BASIC_TYPE(double)
#endif

}

#endif /* granary_BASIC_WRAPPERS_H_ */
