"""High-level data flow framework for python CFGs.

Copyright (C) 2013 Peter Goodman. All rights reserved."""


import collections


class DataFlowProblem(object):
  """Represents a data-flow framework for CFGs."""

  INTER = 0
  INTRA = 1

  FORWARD = 0
  BACKWARD = 1

  DONT_FILTER = lambda _: True

  # Initialise the data flow problem with a CFG, the type of
  # graph on which we're operating, and the direction of the
  # data flow.
  def __init__(self, cfg, flow=INTRA, direction=BACKWARD, include=DONT_FILTER):

    assert flow in (self.INTER, self.INTRA)
    assert direction in (self.FORWARD, self.BACKWARD)

    self.edges = collections.defaultdict(set)

    if self.INTRA is flow:

      # Forward, intra-procedural data-flow problem.
      if self.FORWARD is direction:
        for bb, pred_bbs in cfg.intra_predecessors.items():
          if not include(bb):
            continue
          for pred_bb in pred_bbs:
            if include(pred_bb):
              self.edges[bb].add(pred_bb)

        for pred_bb in cfg.tail_callers:
          for bb in cfg.intra_predecessors[pred_bb]:
            if include(bb) and include(pred_bb):
              self.edges[bb].add(pred_bb)

      # Backward, intra-procedural data-flow problem.
      else:
        for bb, succ_bbs in cfg.intra_successors.items():
          if not include(bb):
            continue
          for succ_bb in succ_bbs:
            if include(succ_bb):
              self.edges[bb].add(succ_bb)

        for bb in cfg.tail_callers:
          for succ_bb in cfg.intra_successors[bb]:
            if include(bb) and include(succ_bb):
              self.edges[bb].add(succ_bb)
    else:
      assert False  # TODO :-P
      if self.BACKWARD is direction:
        pass
      else:
        pass

  # Run a data flow problem.
  #
  # Args:
  #   initial:              A function the generates the initial value for
  #                         a given basic block.
  #   process:              A function that operates on a basic block and
  #                         its current value, and returns a value meant
  #                         for consumption by the `meet` function.
  #   meet:                 A function operating on a basic block, its
  #                         current value, and a list of processed values.
  #                         This should return the next value/result for
  #                         the operated on basic block.
  #   check:                A function that checks that the data-flow problem
  #                         will determine by comparing the old and new values
  #                         of a node to ensure that the values represent valid
  #                         transitions witin the data-flow lattice.
  def run(self, initial, process, meet, check):
    changed = True
    results = {}
    
    # Initialise the data-flow problem for every node in graph.
    for bb, target_bbs in self.edges.items():
      if bb not in results:
        results[bb] = initial(bb)
      for target_bb in target_bbs:
        if target_bb not in results:
          results[target_bb] = initial(target_bb)

    while changed:
      changed = False
      for bb, target_bbs in self.edges.items():
        prev_value = results[bb]
        incoming_values = []
        for target_bb in target_bbs:
          incoming_values.append(process(target_bb, results[target_bb]))

        new_value = meet(bb, prev_value, incoming_values)

        if new_value != prev_value:
          assert check(prev_value, new_value)
          changed = True
          results[bb] = new_value

    return results
