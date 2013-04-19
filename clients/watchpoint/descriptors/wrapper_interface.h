/*
 * wrapper_interface.h
 *
 *  Created on: 2013-04-11
 *      Author: akshayk
 */

#ifndef WRAPPER_INTERFACE_H_
#define WRAPPER_INTERFACE_H_


#ifdef __GNUC__
#   define likely(x)       __builtin_expect((x),1)
#   define unlikely(x)     __builtin_expect((x),0)
#else
#   define likely(x)       (x)
#   define unlikely(x)     (x)
#endif

#include "clients/watchpoint/descriptors/descriptor_table.h"



namespace client {
    enum {
        WP_ADDRESS_NOT_ENABLED               = 0x8000000000000000ULL,
        WP_ADDRESS_ENABLED                   = ~WP_ADDRESS_NOT_ENABLED, // high-order bit not set

        WP_ADDRESS_INDEX_MASK                = 0xffff000000000000ULL,
        WP_ADDRESS_INDEX_OFFSET              = (12 * 4),
        WP_ADDRESS_DISPLACEMENT_MASK         = 0x0000ffffffffffffULL
    };

    /// Static initialisation of global data
    //static descriptor_table DESCRIPTOR_TABLE;

    inline static bool is_watched_address(uint64_t addr) {
        if(!addr)
            return !(addr & WP_ADDRESS_NOT_ENABLED);
        else
            return false;
    }

    template <typename T>
    inline T *to_watched_address(T *ptr, uint64_t size) throw() {
        if(!ptr) {
            return ptr;
        }

        if(likely(is_watched_address(reinterpret_cast<uint64_t>(ptr)))) {
            return ptr;
        } else {
            const uint64_t index(descriptor_table::allocate_descriptor<T>(ptr, size));
            const uint64_t displacement_part(WP_ADDRESS_DISPLACEMENT_MASK & (reinterpret_cast<uint64_t>(ptr)));
            const uint64_t index_part(index << WP_ADDRESS_INDEX_OFFSET);
            return (T *) ((index_part | displacement_part) & WP_ADDRESS_ENABLED);
        }
    }

    template <typename T>
    inline T *to_unwatched_address(T *ptr) throw() {
        if(!ptr) {
            return ptr;
        }
        return (T*)(WP_ADDRESS_INDEX_MASK | (reinterpret_cast<uint64_t>(ptr)));
    }


    template <typename T>
    bool is_watchpoint(T *&ptr) throw() {
        return is_watched_address((uint64_t) ptr);
    }

    template <typename T>
    void add_watchpoint(T *&ptr, unsigned long size = 0) throw() {
        if(size) {
            ptr = to_watched_address(ptr, size);
        }else {
            //ptr = to_watched_address(ptr, (uint64_t)sizeof(T));
        }
    }

    template <typename T>
    void remove_watchpoint(T *&ptr) throw() {
        ptr = to_unwatched_address(ptr);
    }

}

#endif /* WRAPPER_INTERFACE_H_ */
