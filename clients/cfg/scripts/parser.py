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

  # Initialises an empty basic block.
  def __init__(self):
    for attr in self.PARSER:
      setattr(self, attr, None)

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


class ControlFlowGraph(object):
  """Represents a combined inter- and intra-procedural control-flow graph."""

  # Initialise the combined control-flow graph instance.
  def __init__(self):
    self.inter_roots = set()
    self.intra_roots = set()
    self.basic_blocks = {}

  # Update the combined CFG with a new basic block.
  def _accept_bb(self, bb):
    assert bb
    assert bb.block_id not in self.basic_blocks
    self.basic_blocks[bb.block_id] = bb

  # Parse a string containing an edge definition. Update the control-flow graphs
  # by adding in the edge.
  def _parse_edge(self, line):
    pass

  # Parse an iterable of strings into a combined control-flow graph.
  #
  # Args:
  #   lines:            An iterable of lines.
  def parse(self, lines):
    pending_edge_lines = []
    pending_bb_lines = []
    parsed_format = False
    for line_ in lines:
      line = line_.strip(" \r\n\t")

      # Parse the format for basic blocks.
      if line.startswith("BB_FORMAT"):
        assert not parsed_format
        parsed_format = True
        BasicBlock.parse_format(line)

      # Parse a basic block.
      elif line.startswith("BB"):
        if parsed_format:
          self._accept_bb(BasicBlock.parse(line))
        else:
          pending_bb_lines.append(line)

      # Add an edge line for later parsing.
      elif line.startswith("E"):
        pending_edge_lines.append(line)

      # Unexpected line.
      else:
        print repr(line)
        assert False

    for line in pending_bb_lines:
      self._accept_bb(BasicBlock.parse(line))

    for line in pending_edge_lines:
      self._parse_edge(line)

    del pending_bb_lines
    del pending_edge_lines


# Parse a file given as input.
if __name__ == "__main__":
  import sys
  with open(sys.argv[1]) as lines:
    cfg = ControlFlowGraph()
    cfg.parse(lines)
