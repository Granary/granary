/*
 * descriptors.h
 *
 *  Created on: 2013-04-11
 *      Author: akshayk
 */

#ifndef DESCRIPTORS_H_
#define DESCRIPTORS_H_

using namespace granary;

namespace client {

struct descriptors {
public:
	uint64_t base_address;
	uint64_t limit;
} __attribute__((packed));

typedef struct decriptors_config descriptors_config;

struct decriptors_config {
    enum {
        SLAB_SIZE = 16,
        EXECUTABLE = false,
        TRANSIENT = false,
        SHARED = true,
        EXEC_WHERE = EXEC_CODE_CACHE,
        MIN_ALIGN = 16
    };
};

}



#endif /* DESCRIPTORS_H_ */
