/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * printf.cc
 *
 *  Created on: 2013-02-13
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/printf.h"

#define LOG 1

#if LOG
#   include <unistd.h>
#   include <cstdarg>
#endif

namespace granary {
#if LOG
    static int granary_out(2); // STDERR


    static unsigned cstr_length(const char *ch) throw() {
        unsigned len(0);
        for(; *ch; ++ch) {
            ++len;
        }
        return len;
    }


    static char *buffer_int_generic(
        char *buff,
        uint64_t data,
        bool is_64_bit,
        bool is_signed,
        unsigned base
    ) throw() {
        if(16 == base) {
            *buff++ = '0';
            *buff++ = 'x';
        }

        if(!data) {
            *buff++ = '0';
            return buff;
        }

        // Sign-extend a 32-bit signed value to 64-bit.
        if(!is_64_bit && is_signed) {
            data = static_cast<uint64_t>(static_cast<int64_t>(
                static_cast<int32_t>(data & 0xFFFFFFFFULL)));
        }

        // Treat everything as 64-bit.
        if(is_signed) {
            const int64_t signed_data(static_cast<int64_t>(data));
            if(signed_data < 0) {
                *buff++ = '-';
                data = static_cast<uint64_t>(-signed_data);
            }
        }

        uint64_t max_base(base);
        for(; data / max_base; max_base *= base) { }
        for(max_base /= base; max_base; max_base /= base) {
            const uint64_t digit(data / max_base);
            ASSERT(digit < base);
            if(digit < 10) {
                *buff++ = digit + '0';
            } else {
                *buff++ = (digit - 10) + 'a';
            }
            data -= digit * max_base;
        }

        return buff;
    }

#endif

    int printf(const char *format, ...) throw() {
#if !LOG
        (void) format;
        return 0;
#else
        ASSERT(-1 != granary_out);

        enum {
            WRITE_BUFF_SIZE = 255
        };

        int num_written(0);
        char write_buff[WRITE_BUFF_SIZE + 1] = {'\0'};
        char *write_ch(&(write_buff[0]));
        const char * const write_ch_end(&(write_buff[WRITE_BUFF_SIZE]));
        char * const write_ch_begin(&(write_buff[0]));

        va_list args;
        va_start(args, format);

        bool is_64_bit(false);
        bool is_signed(false);
        unsigned base(10);
        const char *sub_string(nullptr);
        uint64_t generic_int_data(0);

        for(const char *ch(format); *ch; ) {

            // Buffer parts of the string between arguments.
            for(; write_ch < write_ch_end && *ch && '%' != *ch; ) {
                *write_ch++ = *ch++;
            }

            // Output the so-far buffered string.
            if(write_ch > write_ch_begin) {
                num_written += write(
                    granary_out, write_ch_begin, write_ch - write_ch_begin);
                write_ch = write_ch_begin;
            }

            // If we're done, then break out.
            if(!*ch) {
                break;

            // Not an argument, it's actually a %%
            } else if('%' == *ch && '%' == ch[1]) {
                *write_ch++ = '%';
                ch += 2;
                continue;
            }

            ASSERT('%' == *ch);
            ++ch;

            is_64_bit = false;
            is_signed = false;
            base = 10;

        retry:
            switch(*ch) {

            // Character.
            case 'c':
                *write_ch++ = static_cast<char>(va_arg(args, int));
                ++ch;
                break;

            // String.
            case 's':
                sub_string = va_arg(args, const char *);
                num_written += write(
                    granary_out, sub_string, cstr_length(sub_string));
                ++ch;
                break;

            // Signed decimal number.
            case 'd': is_signed = true; goto generic_int;

            // Unsigned hexadecimal number.
            case 'x': is_signed = false; base = 16; goto generic_int;

            // Pointer.
            case 'p': is_64_bit = true; base = 16; goto generic_int;

            // Unsigned number.
            case 'u':
            generic_int:
                generic_int_data = 0;
                if(is_64_bit) {
                    generic_int_data = va_arg(args, uint64_t);
                } else {
                    generic_int_data = va_arg(args, uint32_t);
                }
                write_ch = buffer_int_generic(
                    write_ch, generic_int_data, is_64_bit, is_signed, base);
                ++ch;
                break;

            // Long (64-bit) number.
            case 'l': is_64_bit = true; ++ch; goto retry;

            // End of string.
            case '\0':
                *write_ch++ = '%';
                break;

            // Unknown.
            default: ASSERT(false); ++ch; break;
            }
        }

        va_end(args);

        // Output the so-far buffered string.
        if(write_ch > write_ch_begin) {
            num_written += write(
                granary_out, write_ch_begin, write_ch - write_ch_begin);
        }

        return num_written;
#endif
    }
}

