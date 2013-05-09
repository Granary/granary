/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: May 9, 2013
 *      Author: Peter Goodman
 */

#ifndef granary_BASIC_WRAPPERS_H_
#define granary_BASIC_WRAPPERS_H_

namespace granary {

/// Basic type wrappers for POINTER TYPES are defined here so that client code
/// can override the wrappers for the basic pointer types should they desire.

#ifndef WRAPPER_FOR_void_pointer
    WRAP_BASIC_TYPE(void)
#endif

#ifndef WRAPPER_FOR_int8_t_pointer
    WRAP_BASIC_TYPE(int8_t)
#endif

#ifndef WRAPPER_FOR_uint8_t_pointer
    WRAP_BASIC_TYPE(uint8_t)
#endif

#ifndef WRAPPER_FOR_int16_t_pointer
    WRAP_BASIC_TYPE(int16_t)
#endif

#ifndef WRAPPER_FOR_uint16_t_pointer
    WRAP_BASIC_TYPE(uint16_t)
#endif

#ifndef WRAPPER_FOR_int32_t_pointer
    WRAP_BASIC_TYPE(int32_t)
#endif

#ifndef WRAPPER_FOR_uint32_t_pointer
    WRAP_BASIC_TYPE(uint32_t)
#endif

#ifndef WRAPPER_FOR_int64_t_pointer
    WRAP_BASIC_TYPE(int64_t)
#endif

#ifndef WRAPPER_FOR_uint64_t_pointer
    WRAP_BASIC_TYPE(uint64_t)
#endif

#ifndef WRAPPER_FOR_float_pointer
    WRAP_BASIC_TYPE(float)
#endif

#ifndef WRAPPER_FOR_double_pointer
    WRAP_BASIC_TYPE(double)
#endif

}

#endif /* granary_BASIC_WRAPPERS_H_ */
