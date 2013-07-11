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

    # Links together blocks in the intra-procedural control-flow graph.
    'successors',
    'predecessors'
  )

  PARSE_BOOL  = lambda s: bool(int(s))
  PARSE_STR   = lambda s: s
  PARSE_NUM   = lambda s: int(s)

  PARSER = {
    'is_root':                    PARSE_BOOL,
    'is_function_entry':          PARSE_BOOL,
    'is_function_exit':           PARSE_BOOL,
    'is_app_code':                PARSE_BOOL,
    'is_allocator':               PARSE_BOOL,
    'is_deallocator':             PARSE_BOOL,
    'num_executions':             PARSE_NUM,
    'function_id':                PARSE_NUM,
    'block_id':                   PARSE_NUM,
    'num_outgoing_jumps':         PARSE_NUM,
    'has_outgoing_indirect_jmp':  PARSE_BOOL,
    'app_name':                   PARSE_STR,
    'app_offset_begin':           PARSE_NUM,
    'app_offset_end':             PARSE_NUM,
    'num_interrupts':             PARSE_NUM,
  }

  FORMAT = None
  HAS_FORMAT = False
  FORMAT_PREFIX_LEN = len("BB_FORMAT(")
  BB_PREFIX_LEN = len("BB(")

  # Initialises a new basic block.
  def __init__(self):
    self.successors = set()
    self.predecessors = set()

  # Sets the basic block parsing format.
  @classmethod
  def parse_format(cls, str_):
    assert not cls.HAS_FORMAT
    cls.FORMAT = str_.strip("\r\n\t ")[cls.FORMAT_PREFIX_LEN:-1].split(',')
    cls.HAS_FORMAT = True

  # Parse a string into a basic block instance according to the set parsing
  # format.
  @classmethod
  def parse(cls, str_):
    assert cls.HAS_FORMAT
    parts = str_.strip("\r\n\t ")[cls.BB_PREFIX_LEN:-1].split(",")
    self = BasicBlock()
    for (attr, val) in zip(cls.FORMAT, parts):
      setattr(self, attr, cls.PARSER[attr](val))
    return self

