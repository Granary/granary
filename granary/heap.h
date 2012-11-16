/*
 * heap.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_HEAP_H_
#define Granary_HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/// DynamoRIO-compatible heap allocation functions; these are from globally
/// known allocators.

void *heap_alloc(void *, unsigned long long);
void heap_free(void *, void *, unsigned long long);

#ifdef __cplusplus
}

namespace granary {

    struct thread_info;
    struct cpu_info;


    /// Thread-private allocation routines
    void *allocate(thread_info *, unsigned size);
    void free(thread_info *, void *, unsigned size);


    /// CPU-private allocation routines
    void *allocate(cpu_info *, unsigned size);
    void free(cpu_info *, void *, unsigned size);


    /// Allocate executable memory; we provide no support for freeing arbitrary
    /// previously allocated executable memory; however, the previous allocation
    /// request can be de-allocated.

    void *allocate_executable(cpu_info *, unsigned size);
    void *allocate_executable(thread_info *, unsigned size);


    void free_executable(cpu_info *, unsigned size);
    void free_executable(thread_info *, unsigned size);

}

#endif


#endif /* Granary_HEAP_H_ */
