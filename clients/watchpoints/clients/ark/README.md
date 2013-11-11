ARK
=================

Ark is a datastructure archeology project

Using the Granary watchpoint/DBT framework Ark discovers the shape of on disk data structures.

Process
=================
1. Wrap "data entry points" like memmap, read, fread, etc.
2. Watch root memory address of blocks used at data entry points. Granary propogates any taint that is based on memory offsets. Will need to ensure watchpoints are added to memory addresses that are calculated outside the context of a memory read (I think)
3. Instrument all access to watched memory. Discover what the most common access type to each offset is. Discover if double indirection occurs i.e. pointer was written to disk 
