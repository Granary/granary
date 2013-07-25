"""Convert a CFG into a DOT graph.

Copyright (C) 2013 Peter Goodman. All rights reserved.
"""


import os
import pickle
import re
import commands
import sys
import parser


# File name of the symbol cache.
FILE_SYMBOL_CACHE = "/tmp/symbol_cache.pickle"


# readelf lines have this format:
#       511: 000000000000ab40   661 FUNC    GLOBAL DEFAULT    2 e1000_phy_get_info
RE_ELF_OFFSET_AND_NAME = re.compile(r"^.*: ([0-9a-f]+) .* ([a-zA-Z0-9_]+)\r?\n?$")


# kallsyms lines have this format:
#       000000000000f068 D x86_bios_cpu_apicid
RE_KALL_OFFSET_AND_NAME = re.compile(r"^([0-9a-f]+) .* ([a-zA-Z0-9_]+)\r?\n?$")


# String prefix for kernel addresses.
KERN_ADDR_PREFIX = "ffffffff8"
KERN_ADDR_PREFIX_LEN = len(KERN_ADDR_PREFIX)


# Run a shell command and capture the output.
def run_command(command):
  status, output = commands.getstatusoutput(command)
  if 0 == status:
    return output.split("\n")
  return []  


def update_symbol_cache_common(cache, name, line, regex):
  global KERN_ADDR_PREFIX
  m = regex.match(line)
  if not m:
    return

  offset_str = m.group(1)
  if offset_str.startswith(KERN_ADDR_PREFIX):
    offset_str = offset_str[KERN_ADDR_PREFIX_LEN:]

  offset, func = int(offset_str, base=16), m.group(2)
  cache[name, offset] = func


# Parse the output of `readelf`.
def update_symbol_cache_elf(cache, name, lines):
  for line in lines:
    update_symbol_cache_common(cache, name, line, RE_ELF_OFFSET_AND_NAME)


# Parse the output of `kallsyms`.
def update_symbol_cache_kallsyms(cache, lines):
  for line in lines:
    update_symbol_cache_common(cache, "linux", line, RE_KALL_OFFSET_AND_NAME)


# Print out information about a single basic block.
def print_bb(bb, seen_bbs, symbols):
  if bb in seen_bbs:
    return

  seen_bbs.add(bb)
  block_ref = (bb.app_name, bb.app_offset_begin)

  print "b%d" % bb.block_id,
  color = ""
  if not bb.is_app_code:
    color = "color=blue"
  if block_ref in symbols:
    print "[%s label=\"%s\"]" % (color, symbols[block_ref]),
  elif color:
    print "[%s]" % color,
  print ";"


# Print out a DOT digraph of the edges in a CFG.
def print_cfg(edges, symbols):
  seen_bbs = set()
  print "digraph {"
  for bb, out_bbs in edges.items():
    print_bb(bb, seen_bbs, symbols)
    for out_bb in out_bbs:
      print_bb(out_bb, seen_bbs, symbols)
      print "b%d -> b%d;" % (bb.block_id, out_bb.block_id)
  print "}"


# Handle different commands / configurations for printing CFGs.
if "__main__" == __name__:

  call_graph = False
  clear_sym_cache = False
  ko_paths = []
  cfg_path = None
  vmlinux_path = None

  # Parse the arguments.
  for arg in sys.argv[1:]:
    if "--call_graph" == arg.strip():
      call_graph = True
    elif "--clear_sym_cache" == arg.strip():
      clear_sym_cache = True
    else:
      path = arg.strip()
      if os.path.exists(path):

        if not cfg_path \
        and ".ko" not in path \
        and "vmlinux" not in path \
        and ".syms" not in path:
          cfg_path = path
        else:
          ko_paths.append(path)
      else:
        print "Path", path, "doesn't appear to exist."
        exit(-1) 

  if not cfg_path:
    print "The path to the dumped CFG file must be provided."
    exit(-1)

  # Kill our existing symbol cache. We'd want to do this any time
  # the kernel or specific modules are rebuilt.
  if clear_sym_cache and os.path.exists(FILE_SYMBOL_CACHE):
    os.remove(FILE_SYMBOL_CACHE)

  # Initialise or load our KO cache.
  sym_cache = {}
  if not clear_sym_cache and os.path.exists(FILE_SYMBOL_CACHE):
    sym_cache = pickle.load(open(FILE_SYMBOL_CACHE, "rb"))

  # Update our KO cache with names from the kernel object file.
  for path in ko_paths:

    if path.endswith(".ko"):
      update_symbol_cache_elf(
          sym_cache,
          os.path.basename(path)[:-3],
          run_command("readelf -s %s | grep ' FUNC '" % path))
    elif path.endswith("kernel.syms"):
      update_symbol_cache_kallsyms(sym_cache, open(path))
    else:
      update_symbol_cache_elf(
          sym_cache,
          "linux",
          run_command("readelf -s %s | grep ' FUNC '" % path))

  # Update the cache with the new symbols.
  if ko_paths:
    pickle.dump(sym_cache, open(FILE_SYMBOL_CACHE, "wb"))

  # Parse the CFG dump.
  cfg = parser.ControlFlowGraph()
  with open(cfg_path) as lines:
    cfg.parse(lines)

  # Choose which edges to show.
  edges = None
  if call_graph:
    edges = cfg.inter_successors
  else:
    edges = cfg.intra_successors

  # Print out the chosen CFG.
  print_cfg(edges, sym_cache)

