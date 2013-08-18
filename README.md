Granary
=======

Copyright Notice
----------------
Copyright &copy; 2012-2013 Peter Goodman. All rights reserved.

Getting Started
---------------
 * You will need a recent version of the GCC (at least`gcc`/`g++` 4.8), or a
   recent version of Clang (at least `clang`/`clang++` 3.2).
 * You will need Python 2.7 or above, but not Python 3.
 * You will need `make`/`gmake`.

Common Makefile Options
-----------------------
 * `KERNEL` specifies if Granary is being built for kernel instrumentation (`1`)
   or user space instrumentation (`0`). The default value is `0`.
 * `KERNEL_DIR` specifies the location of one's kernel build directory. This is
   only relevant if `KERNEL=1`. The default value is `/lib/modules/$(shell uname -r)/build`.
 * `GR_CC` specifies the C compiler to be used. The default is `gcc-4.8`.
 * `GR_CXX` specifies the C++ compiler to be used. The default is `g++-4.8`.
 * `GR_PYTHON` specifies the version of Python to use. Granary depends on Python 2.7;
   however, some setups default to other versions. If Python 2.7 is not your default
   version, and if it is installed, then you can select the specific binary with this
   flag.
 * `GR_CLIENT` specifies the client/tool to be used. The default is `null`. Other
   clients include `cfg`, `null_plus`, `track_entry_exit`, `bounds_checker`,
   `everything_watched`, `leak_detector`, `watchpoint_null`, `rcudbg`,
   `shadow_memory`, and `watchpoint_stats`. This might not be a complete list.
 * `GR_EXTRA_CC_FLAGS` and `GR_EXTRA_CXX_FLAGS` allow one to specify extra flags to
   the C and C++ compilers, respectively.
 * `GR_DLL` allows one to tell the Makefile to generate a dynamically linked library
   (for use when `KERNEL=0`). The default value is `0`. If `GR_DLL=1` and `KERNEL=0`
   then the Makefile will generate `libgranary.so` on Linux and `libgranary.dyld` on
   Mac.

Installing
----------
This is a necessary first step. This step creates a number of folders, generates
files, etc.

```basemake
make env
```

Compiling for user space
------------------------

```basemake
make detach KERNEL=0
make wrappers KERNEL=0
make clean KERNEL=0 ; make all KERNEL=0
```

### Compiling with clang
Other options for clang are `GR_ASAN` and `GR_LIBCXX`.


Instrumenting user space programs
---------------------------------

### Linux
```basemake
LD_PRELOAD=./libgranary.so my_program
```

### Mac OS X
```basemake
DYLD_INSERT_LIBRARIES=./libgranary.dylib my_program
```


Compiling for kernel space
--------------------------
This will prepare a number of auto-generated source and non-source files
for Granary. This step does not need to be repeated, unless you plan on
changing any of the scripts in the scripts folder.

Note: If you are using a remote machine, e.g. a VM, then specify `--remote`,
otherwise leave it absent from the load.py script invocation. It helps if
your local and remote machines are running the same build of the kernel. This
is because `load.py --symbols` makes a copy of `/proc/kallsyms`.

```basemake
python scripts/load.py --symbols
make detach KERNEL=1
make wrappers KERNEL=1
```

The next command cleans and builds Granary. Because of the amount of template
code, it is often suggested that you clean before each build. If, however,
you are not modifying the template-heavy code (wrappers), then cleaning is
not necessary.

Note: not cleaning will not guarantee a faster build.

Note: Do not use the `-j` option to invoke a parallel make for kernel builds.
A python script runs as one of the last compilation dependencies, and its job
is to extract specific functions from the assembly of the now-compiled files of
Granary. A parallel make will mean this script sees inconsistent / incomplete
files. Todo: make it so that this last dependency depends on all others, so that
parallel make is supported.

```basemake
make clean KERNEL=1 ; make all KERNEL=1 GR_CC=clang GR_CXX=clang++
```

Instrumenting kernel modules
----------------------------
To instrument kernel modules, Granary must be loaded and started. These are two
distinct steps so that if remote loading/debugging is used, then there is a chance
to get Granary's symbol locations within the virtualised environment.

Once Granary is compiled, then execute the following commands. If you are loading
Granary locally, then `--remote` need not be specified. This step might require
typing the administrator password, either for the local or remote machine, depending
on the `--remote` flag.

```basemake
python scripts/load.py --remote
```

Note: if you are loading Granary into a VM, then it is suggested to use
clang/clang++ as the compiler toolchain (as exampled above), because the debugging
sections emitted by clang are orders of magnitude smaller, which improves the
execution time of the `load.py` script.

Note: this is a good time to attach `gdb` if you are running Granary remotely and
are concerned that Granary might crash during its initialisation. Instruct `gdb` to
`source` the auto-generated `granary.syms` file in the Granary source directory
for Granary's symbols and their locations in memory.

On the machine on which Granary will instrument modules, invoke the following
command after Granary is loaded.

```basemake
sudo touch /dev/granary
```

This will initialise Granary. You can inspect that Granary is correctly initialised
by invoking `dmesg`. Reinvoking `sudo touch /dev/granary` either has no effect, or--
if Granary is configured with performance counters enabled--outputs performance 
statistics to the kernel log.

After Granary is loaded and initialised, modules can be instrumented. To instrument
a module, simply load it into the kernel. Loading a module is done using either
`modprobe` or `insmod`.
