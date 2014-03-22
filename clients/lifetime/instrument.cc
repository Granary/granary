/* Copyright 2014 Peter Goodman, all rights reserved. */

#include "clients/lifetime/metadata.h"
#include "clients/lifetime/instrument.h"

using namespace granary;

namespace client {

// A function invoked on every memory read of a watched address `addr`.
void on_read(uintptr_t addr, unsigned long num_bytes) {
  auto obj_meta = wp::descriptor_of(addr);
  auto ds_meta = obj_meta->data_structure;
  auto byte_offset_in_struct = addr - obj_meta->base_addr;
  granary::printf(
      "Reading %d bytes from address %lx, %u bytes into struct of size %u\n",
      num_bytes, addr, byte_offset_in_struct, ds_meta->num_bytes);

  // TODO(akshay):
  //    1) Update read counter for the associated bytes.
  //    2) Determine the value being read. This will required using
  //       the `unwatched_addr` function on `addr`, then casting it to an
  //       pointer to an integral type whose size matches `num_bytes`, de-
  //       referencing the pointer (to read the data).
  //    3) Store the value being read into the `ds_meta` somehow.
  //        a) We might want to store some number of values and the number of
  //           times we saw that value. If the number of distinct values seen
  //           exceeds some small threshold, then treat the field as non-
  //           constant.
}

// A function invoked on every memory write to a watched address `addr`.
void on_write(uintptr_t addr, unsigned long num_bytes) {
  auto obj_meta = wp::descriptor_of(addr);
  auto ds_meta = obj_meta->data_structure;
  auto byte_offset_in_struct = addr - obj_meta->base_addr;
  granary::printf(
      "Writing %d bytes to address %lx, %u bytes into struct of size %u\n",
      num_bytes, addr, byte_offset_in_struct, ds_meta->num_bytes);

  // TODO(akshay): Similar to above, but for writes. For writes we don't care
  //               about the value being written, because specializing on writes
  //               is unlikely to lead very far at a large scale.
}

// Versions of the above functions that are safe to call from within
// instrumented code.
app_pc on_read_func = nullptr;
app_pc on_write_func = nullptr;

// Initialize this client / tool. This creates the safe/clean-callable versions
// of the `on_read` and `on_write` callback functions.
void init(void) {
  cpu_state_handle cpu;
  IF_TEST( cpu->in_granary = false; )
  cpu.free_transient_allocators();
  on_read_func = generate_clean_callable_address(on_read);
  cpu.free_transient_allocators();
  on_write_func = generate_clean_callable_address(on_write);
}

// Inject a function call to `on_read` before every memory read of a
// watched address.
DEFINE_READ_VISITOR(lifetime, {
  insert_clean_call_after(ls, label, on_read_func, op, (unsigned long) size);
})

// Inject a function call to `on_write` before every memory write to a
// watched address.
DEFINE_WRITE_VISITOR(lifetime, {
  insert_clean_call_after(ls, label, on_write_func, op, (unsigned long) size);
})

DEFINE_INTERRUPT_VISITOR(lifetime, {})

}  // namespace client
