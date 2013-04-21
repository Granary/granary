/*
 * descriptors.h
 *
 *  Created on: 2013-04-11
 *      Author: akshayk
 */

#ifndef DESCRIPTORS_H_
#define DESCRIPTORS_H_

namespace client {

    struct descriptor {
    public:
        uint64_t base_address;
        uint64_t limit;
    } __attribute__((packed));
}



#endif /* DESCRIPTORS_H_ */
