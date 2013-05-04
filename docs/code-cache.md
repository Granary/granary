WARNING!!! This document is outdated.

Code Cache
==========

TODO

key ideas:
- CPU private bump-pointer allocated regions; staging a basic block just
  bumps the pointer; this block pointer is eventually shared with all cores.
-- would be good if this bump-pointer were module specific, to enable better
   module unloading.

- staged hash table lookups for IBL
-- fast path: cpu private hashtable
-- slow path: eventually consistent hashtable; rcu-updated where quiescence point
              is that all module code has returned back into the kernel.


- basic blocks are looked up based on module code address AND interrupt state.

