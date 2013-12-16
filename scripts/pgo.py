"""Parse the output of the CFG tool, and construct some code for
profile-guided optimization.

Copyright (C) 2013, Peter Goodman. All rights reserved.
"""

import collections
import sys


class Edge(object):
  __slots__ = ('count', 'source', 'target', 'is_fall_through')
  def __init__(self):
    self.count = 0
    self.target = None
    self.source = None
    self.is_fall_through = False

class BasicBlock(object):
  __slots__ = ('successors', 'count', 'edge_lines')
  def __init__(self):
    self.successors = collections.defaultdict(Edge)
    self.count = 0
    self.edge_lines = []


def O(*args):
  print "".join(str(a) for a in args)


if __name__ == "__main__":

  BBS = collections.defaultdict(BasicBlock)
  VIRTUAL_CALLS = collections.defaultdict(set)
  LINES = []
  LAST_BB = None

  with open(sys.argv[1], "r") as lines:
    for line in lines:
      line = line.strip(" \r\n")
      if "BB" in line:
        parts = line.split(",")
        start = parts[4]
        LAST_BB = BBS[start]
        BBS[start].count += int(parts[5])
      else:
        if "JMP*" in line or "CALL*" in line:
          parts = line[:-1].split(",")
          VIRTUAL_CALLS[parts[0][-16:]].add(parts[1])
        elif "CALL" not in line:
           LAST_BB.edge_lines.append(line)


  for bb in BBS.values():
    for line in bb.edge_lines:
      parts = line[:-1].split(",")
      if line.startswith("Jcc"):
        not_taken_count = int(parts[3])

        bb.successors[parts[1]].source = bb
        bb.successors[parts[1]].count += bb.count - not_taken_count
        bb.successors[parts[1]].target = BBS[parts[1]]

        bb.successors[parts[2]].is_fall_through = True
        bb.successors[parts[2]].source = bb
        bb.successors[parts[2]].count += not_taken_count
        bb.successors[parts[2]].target = BBS[parts[2]]

      elif line.startswith("JMP"):
        addr = line[len("JMP("):-len(")")]
        if addr in BBS:
          bb.successors[addr].source = bb
          bb.successors[addr].target = BBS[addr]
          bb.successors[addr].count += bb.count
      else:
        assert False

  # Start with all calls that have only one t
  virtual_calls = []  

  for addr in VIRTUAL_CALLS:
    # Keep only kernel addresses.
    if "ffffffff8" not in addr:
      continue

    targets = VIRTUAL_CALLS[addr]

    # Only one target; take it.
    if 1 == len(targets):
      virtual_calls.append((addr, targets.pop()))

    # More than one target; take the most frequent.
    else:
      target_counts = []
      for target_addr in targets:
        if target_addr in BBS:
          target_counts.append((target_addr, BBS[target_addr].count))
      target_counts.sort(key=lambda p: p[0], reverse=True)

      if target_counts and "ffffffff8" in target_counts[0][0]:
        virtual_calls.append((addr, target_counts[0][0]))

  # Get the (source, dest) as pairs of ints, sorted by their source address.
  virtual_calls = map(lambda p: (int(p[0], 16), int(p[1], 16)), virtual_calls)
  virtual_calls.sort(key=lambda p: p[0])

  jccs = []
  for addr in BBS:
    bb = BBS[addr]

    # Keep only Jccs.
    if len(bb.successors) is not 2:
      continue

    # Keep only kernel addresses.
    if "ffffffff8" not in addr:
      continue

    # Find the most often taken branch. If it's the fall-through
    # then don't include it in the output, as that's our default
    # behavior anyway.
    first, second = list(bb.successors.items())
    if first[1].count > second[1].count:
      if not first[1].is_fall_through:
        jccs.append((int(addr, 16), int(first[0], 16)))
    else:
      if not second[1].is_fall_through:
        jccs.append((int(addr, 16), int(second[0], 16)))

  jccs.sort(key=lambda p: p[0])

  KERNEL_BASE = 0xffffffff80000000

  O("/* Auto-generated file for PGO */")
  O("#include \"granary/globals.h\"")
  O("namespace granary {")
  O("    /// Predicted indirect CTIs.")
  O("    static const cti_info INDIRECT_CTIS[] = {")
  for source_addr, dest_addr in virtual_calls:
    O("        {", hex(source_addr - KERNEL_BASE), ",", hex(dest_addr - KERNEL_BASE), "},")
  O("        {0, 0}")
  O("    };")
  O("    /// Choices for which side of a Jcc to follow.")
  O("    static const cti_info CONDITIONAL_CTIS[] = {")
  for source_bb_addr, dest_addr in jccs:
    O("        {", hex(source_bb_addr - KERNEL_BASE), ",", hex(dest_addr - KERNEL_BASE), "},")
  O("        {0, 0}")
  O("    };")
  O("    enum {")
  O("        NUM_INDIRECT_CTIS = ", len(virtual_calls), ",")
  O("        NUM_CONDITIONAL_CTIS = ", len(jccs))
  O("    };")
  O("}")
  O()
