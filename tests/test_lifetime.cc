/* Copyright 2014 Peter Goodman, all rights reserved. */

#if defined(CLIENT_LIFETIME) && !CONFIG_ENV_KERNEL

#include "granary/globals.h"
#include "granary/client.h"
#include "granary/code_cache.h"
#include "granary/policy.h"
#include "granary/mangle.h"
#include "granary/test.h"
#include "clients/instrument.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace test {

struct my_struct_t {
  char name[80];
  int age;

  unsigned char unmodified;
};

struct my_struct_t * get_a_struct(const char *name, int age) {
  struct my_struct_t *ret = (struct my_struct_t *) malloc(
    sizeof(struct my_struct_t));
  strcpy(ret->name, name);
  ret->age = age;
  ret->unmodified = 0x01;
  return ret;
}

int make_objs(void) {
  struct my_struct_t *john_madden, *lebron_james, *bob_barker;

  john_madden = get_a_struct("John Madden", 57);
  lebron_james = get_a_struct("Lebron James", 31);
  bob_barker = get_a_struct("Bob Barker", 99);

  bob_barker->age = 31;
  lebron_james->name[7] = 'L';

  lebron_james->age = john_madden->name[0];

  // Do a few reads.
  if (bob_barker->unmodified ||
      lebron_james->unmodified ||
      john_madden->unmodified) {
    john_madden->age = lebron_james->age;
  }

  return 0;
}

static void test_find_const(void) {

  auto policy = GRANARY_INIT_POLICY;
  auto func = (granary::app_pc) make_objs;
  policy.return_address_in_code_cache(true);
  granary::basic_block bb_func(granary::code_cache::find(func, policy));

  char x[10];
  strcpy(x, "hi");
  free(malloc(1));

  bb_func.call<void>();

}

ADD_TEST(test_find_const,
    "Test the `lifetime` tool.")
}

#endif

