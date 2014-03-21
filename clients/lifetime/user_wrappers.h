/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef CLIENTS_LIFETIME_USER_WRAPPERS_H_
#define CLIENTS_LIFETIME_USER_WRAPPERS_H_

#include "clients/lifetime/metadata.h"

using namespace client::wp;

#if defined(CAN_WRAP_realloc) && CAN_WRAP_realloc
#   define APP_WRAPPER_FOR_realloc
FUNCTION_WRAPPER(APP, realloc, (void *), (void *ptr, size_t size), {
  void *ret_address(__builtin_return_address(0));
  void *old_ptr(nullptr);
  if(is_watched_address(ptr)) {
    old_ptr = ptr;
    ptr = unwatched_address(ptr);
  }
  void *new_ptr(realloc(ptr, size));
  if(new_ptr) {
    if(ptr != new_ptr && new_ptr && old_ptr) {
      free_descriptor_of(old_ptr);
    }
    add_watchpoint(new_ptr, new_ptr, size, ret_address);
  }
  return new_ptr;
})
#endif


#define MALLOCATOR(func, size) \
  { \
    void *ret_address(__builtin_return_address(0)); \
    void *ptr(func(size)); \
    if(ptr) { \
      add_watchpoint(ptr, ptr, size, ret_address); \
    } \
    return ptr; \
  }


#if defined(CAN_WRAP_malloc) && CAN_WRAP_malloc
#   define APP_WRAPPER_FOR_malloc
FUNCTION_WRAPPER(APP, malloc, (void *), (size_t size), MALLOCATOR(malloc, size))
#endif


#if defined(CAN_WRAP_valloc) && CAN_WRAP_valloc
#   define APP_WRAPPER_FOR_valloc
FUNCTION_WRAPPER(APP, valloc, (void *), (size_t size), MALLOCATOR(valloc, size))
#endif


#if defined(CAN_WRAP_calloc) && CAN_WRAP_calloc
#   define APP_WRAPPER_FOR_calloc
FUNCTION_WRAPPER(APP, calloc, (void *), (size_t count, size_t size), {
  void *ret_address(__builtin_return_address(0));
  void *ptr(calloc(count, size));
  if(ptr) {
    add_watchpoint(ptr, ptr, (count * size), ret_address);
  }
  return ptr;
})
#endif


#if defined(CAN_WRAP_free) && CAN_WRAP_free
#   define APP_WRAPPER_FOR_free
FUNCTION_WRAPPER_VOID(APP, free, (void *ptr), {
  if(is_watched_address(ptr)) {
    free_descriptor_of(ptr);
    ptr = unwatched_address(ptr);
  }
  return free(ptr);
})
#endif



#endif  // CLIENTS_LIFETIME_USER_WRAPPERS_H_
