/*
 * async.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_THREAD_H_
#define granary_THREAD_H_

namespace granary {


    struct thread_info;
    struct cpu_info;


    /// Information maintained by granary about each thread.
    struct thread_info {

    };


    /// Information maintained by granary about each CPU.
    struct cpu_info {
#if GRANARY_IN_KERNEL

#endif
    };
}

#endif /* granary_THREAD_H_ */
