/*
 * utils.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_UTILS_H_
#define Granary_UTILS_H_

#include "pp.h"

#include <stdint.h>
#include <cstring>

namespace granary {

template <typename ToT, typename FromT>
FORCE_INLINE ToT unsafe_cast(const FromT &v) throw()  {
    ToT dest;
    memcpy(&dest, &v, sizeof(ToT));
    return dest;
}

template <typename ToT, typename FromT>
FORCE_INLINE ToT unsafe_cast(FromT *v) throw() {
    return unsafe_cast<ToT>(
        reinterpret_cast<uintptr_t>(v)
    );
}

}


#endif /* Granary_UTILS_H_ */
