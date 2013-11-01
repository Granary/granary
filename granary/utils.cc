/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * utils.cc
 *
 *  Created on: 2013-10-31
 *      Author: Peter Goodman
 */

#include <stdint.h>
#include <cstddef>

extern "C" {

    size_t granary_strlen(const char *str) {
        if(!str) {
            return 0;
        }

        size_t ret(0);
        for(; *str; ++str, ++ret) { }
        return ret - 1;
    }


    char *granary_strncpy(char *destination, const char *source, size_t num) {
        char *ret(destination);
        for(size_t i(0); i < num && *source; ++i, ++source) {
            *destination++ = *source;
        }
        return ret;
    }

}

