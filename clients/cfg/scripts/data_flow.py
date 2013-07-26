"""High-level data flow framework for python CFGs.

Copyright (C) 2013 Peter Goodman. All rights reserved."""


class DataFlow(object):
  """Represents a data-flow framework for CFGs."""

  FLOW_INTER = 0
  FLOW_INTRA = 1

  def __init__(self, cfg, flow=DataFlow.FLOW_INTRA):
    pass