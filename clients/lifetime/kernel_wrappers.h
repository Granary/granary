/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef CLIENTS_LIFETIME_KERNEL_WRAPPERS_H_
#define CLIENTS_LIFETIME_KERNEL_WRAPPERS_H_

#include "clients/lifetime/metadata.h"

using namespace client::wp;

#if defined(CAN_WRAP___kmalloc) && CAN_WRAP___kmalloc
#define APP_WRAPPER_FOR___kmalloc
FUNCTION_WRAPPER(APP, __kmalloc, (void *), (size_t size, gfp_t gfp), {
  void *ret_address(__builtin_return_address(0));
  void *ret(__kmalloc(size, gfp));
  if(is_valid_address(ret)) {
    add_watchpoint(ret, ret, size, ret_address);
  }
  return ret;
})
#endif

#if defined(CAN_WRAP_kfree) && CAN_WRAP_kfree
#define APP_WRAPPER_FOR_kfree
FUNCTION_WRAPPER_VOID(APP, kfree, (void *addr), {
  kfree(unwatched_address_check(addr));
})
#endif

#endif  // CLIENTS_LIFETIME_KERNEL_WRAPPERS_H_
