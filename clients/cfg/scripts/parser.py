"""Parse the output from CFG tool into inter- and intra-procedural CFGs.

Copyright (C) 2013 Peter Goodman. All rights reserved.
"""

import collections


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
    'used_regs',
    'entry_regs',
  )

  PARSE_BOOL  = lambda s: bool(int(s))
  PARSE_STR   = lambda s: s
  PARSE_NUM   = lambda s: int(s, base=10)

  BOOLEAN_OR  = lambda a, b: a or b
  BITWISE_OR  = lambda a, b: a | b
  NUM_ADD     = lambda a, b: a + b
  CHOOSE_FIRST= lambda a, b: a

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
    'used_regs':                  PARSE_NUM,
    'entry_regs':                 PARSE_NUM,
  }

  COMBINER = {
    'is_root':                    BOOLEAN_OR,
    'is_function_entry':          BOOLEAN_OR,
    'is_function_exit':           BOOLEAN_OR,
    'is_app_code':                BOOLEAN_OR,
    'is_allocator':               BOOLEAN_OR,
    'is_deallocator':             BOOLEAN_OR,
    'num_executions':             NUM_ADD,
    'function_id':                CHOOSE_FIRST,
    'block_id':                   CHOOSE_FIRST,
    'num_outgoing_jumps':         CHOOSE_FIRST,
    'has_outgoing_indirect_jmp':  BOOLEAN_OR,
    'app_name':                   CHOOSE_FIRST,
    'app_offset_begin':           min,
    'app_offset_end':             max,
    'num_interrupts':             NUM_ADD,
    'used_regs':                  BITWISE_OR,
    'entry_regs':                 BITWISE_OR,
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

  # Combine this basic block with another basic block.
  def combine_with(self, other):
    for attr, combiner in self.COMBINER.items():
      setattr(self, attr, combiner(getattr(self, attr), getattr(other, attr)))


class ControlFlowGraph(object):
  """Represents a combined inter- and intra-procedural control-flow graph."""

  INTRA_PREFIX_LEN = len("INTRA(")
  INTER_PREFIX_LEN = len("INTER(")

  # Initialise the combined control-flow graph instance.
  def __init__(self):
    self.inter_roots = set()
    self.intra_roots = set()
    self.basic_blocks = {}
    self.code_blocks = {}
    self.ordered_calls = collections.defaultdict(list)
    self.tail_callers = set()
    self.inter_successors = collections.defaultdict(set)
    self.inter_predecessors = collections.defaultdict(set)
    self.intra_successors = collections.defaultdict(set)
    self.intra_predecessors = collections.defaultdict(set)

  # Update the combined CFG with a new basic block. This also merges identical
  # basic blocks that were separated by policy information.
  def _accept_bb(self, bb):
    assert bb
    assert bb.block_id not in self.basic_blocks
    bb_id = bb.block_id
    block_id = (bb.app_name, bb.app_offset_begin)
    if block_id in self.code_blocks:
      merge_bb = self.code_blocks[block_id]
      merge_bb.combine_with(bb)
      bb = merge_bb
    else:
      self.code_blocks[block_id] = bb
    self.basic_blocks[bb_id] = bb

  # Parse a string containing an edge definition. Update the control-flow graphs
  # by adding in the edge.
  def _parse_edge(self, line):
    is_intra = True
    if line.startswith("INTRA"):
      succ, pred = self.intra_successors, self.intra_predecessors
      source_id, dest_id = map(int, line[self.INTRA_PREFIX_LEN:-1].split(","))
      if source_id not in self.basic_blocks \
      or dest_id not in self.basic_blocks:
        return # TODO!!

    elif line.startswith("INTER"):
      is_intra = False
      succ, pred = self.inter_successors, self.inter_predecessors
      source_id, dest_id = map(int, line[self.INTER_PREFIX_LEN:-1].split(","))
      
      if source_id not in self.basic_blocks \
      or dest_id not in self.basic_blocks:
        return  # TODO!!

      # Connect the two graphs, and rename the source block by the block id
      # that begins the function.
      source_bb = self.basic_blocks[source_id]
      self.ordered_calls[source_bb].append(self.basic_blocks[dest_id])
      source_id = source_bb.function_id

      if source_id not in self.basic_blocks:
        return  # TODO!!

    else:
      assert False

    source_bb = self.basic_blocks[source_id]
    dest_bb = self.basic_blocks[dest_id]

    # Look for bugs, e.g. intra(ret, ret).
    # TODO: Why do we have these bugs???
    if source_bb is dest_bb and source_bb.is_function_exit and is_intra:
      return

    # Look for tail-calls.
    if dest_bb.is_function_entry and is_intra:
      self.tail_callers.add(source_bb)
      self.ordered_calls[source_bb].append(dest_bb)
      succ, pred = self.inter_successors, self.inter_predecessors

    # Connect the graph.
    succ[source_bb].add(dest_bb)
    pred[dest_bb].add(source_bb)

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
      elif line.startswith("INTER") or line.startswith("INTRA"):
        pending_edge_lines.append(line)

      # Unexpected line.
      else:
        continue

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
