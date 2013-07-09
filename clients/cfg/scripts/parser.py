"""Parse the output from CFG tool into inter- and intra-procedural CFGs.

Copyright (C) 2013 Peter Goodman. All rights reserved.
"""


class BasicBlock(object):
  """Represents a basic block within an intra-procedural control-flow graph."""

  __slots__ = (
    'is_root',
    'is_function_entry',
    'is_function_exit',
    'is_app_code',
    'is_allocator',
    'is_deallocator',
    'num_executions',
    'function_id',
    'block_id',
    'num_outgoing_jumps',
    'has_outgoing_indirect_jmp',
    'app_name',
    'app_offset_begin',
    'app_offset_end',
    'num_interrupts',
  )

  FORMAT = ""

  @classmethod
  def parse_format(cls, str):
    pass

  @classmethod
  def parse(cls, str):
    pass
