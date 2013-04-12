/*
 * wrapper_allocator.h
 *
 *  Created on: 2013-04-11
 *      Author: akshayk
 */

#ifndef WRAPPER_ALLOCATOR_H_
#define WRAPPER_ALLOCATOR_H_


#if defined(CAN_WRAP___kmalloc) && CAN_WRAP___kmalloc
#define WRAPPER_FOR___kmalloc 1
FUNCTION_WRAPPER(__kmalloc, (void*), (size_t size, gfp_t flags), {
    void *watch_ptr = __kmalloc(size, flags);
    return watch_ptr;
})
#endif

#if defined(CAN_WRAP_kfree) && CAN_WRAP_kfree
#define WRAPPER_FOR_kfree 1
FUNCTION_WRAPPER(kfree, (void), (void *addr), {
    kfree(addr);
})
#endif


#endif /* WRAPPER_ALLOCATOR_H_ */
