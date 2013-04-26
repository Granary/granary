"""Post-process an assembly file and do simple symbol concatenation 
(in case the assembly pre-processor does not support it) and replace
the @N@ symbol with a new line.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

import sys
import re

NL = re.compile(r"@N@")
PASTE = re.compile(r"(.*?)\s*##\s*(.*?)")

# open up a post-processed assembly file and replace each @N@ with a new line
# and if the preprocessor didn't do token pasting, then automatically perform
# it.
with open(sys.argv[1]) as lines:
  for line in lines:
    line = NL.sub("\n", line.rstrip(" \t\r\n")).rstrip("\n")
    while True:
      old_line = line
      line = PASTE.sub(r"\1\2", line)
      if old_line == line:
        break
    print line
