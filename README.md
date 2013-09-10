Granary
=======
Granary is a kernel space dynamic binary translation framework. The main goal of Granary
is to enable flexible and efficient instrumentation of Linux kernel modules, while
imposing no overhead to non-module kernel code.

Granary's three key novelties are:
  1. Mixed-mode execution. Granary is able to quickly "attach" and "detach" instrumentation.
     Under normal operation, Granary controls and instruments the execution of module code,
     while leaving the kernel to execute natively.
  2. Policy-driven instrumentation. Granary is able to runtime code specialisation by allowing
     tool authors to explicitly recognize different execution contexts, and associate different
     instrumentation to code running in those contexts using "instrumentation policies". Policy-
     driven instrumentation lets Granary do things like instrument the code running inside an
     RCU read-side critical section differently than the code running outside of a read-side
     critical section.
  3. Reifying instrumentation. Granary bridges the gap between static and dynamic analysis by
     integrating static analysis results into its dynamic analyses. Reifying instrumentation
     allows Granary to implement low-overhead attach/detach, inspect the object graph during
     kernel/module transitions, and assign runtime type information to dynamically allocated
     objects.

Copyright Notice
----------------
Copyright &copy; 2012-2013 Peter Goodman. All rights reserved.

Granary is licensed under a 3-clause BSD license. See [LICENSE.md](LICENSE.md) for
the license. See [ACKNOWLEDGEMENTS.md](ACKNOWLEDGEMENTS.md) for the list of contributors.

Getting Started
---------------
 * You will need a recent version of the GCC (at least`gcc`/`g++` 4.8), or a
   recent version of Clang (at least `clang`/`clang++` 3.2).
 * You will need Python 2.7 or above, but not Python 3.
 * You will need `make`/`gmake`.
 * Consult the [documentation files](docs/) on how to build a kernel that will work with Granary,
   how to set up a virtual machine, and how to set up SSH to enable (fast) loading.
 * Consult the [online discussion group](https://groups.google.com/forum/#!forum/granary-dev)
   for any other questions / details.

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


Compiling for kernel space
--------------------------

First, check [the related documents](docs/) to make sure that your kernel is
built in a Granary-compatible way.

This will prepare a number of auto-generated source and non-source files
for Granary. This step does not need to be repeated, unless you plan on
changing any of the scripts in the scripts folder.

Note: If you are using a remote machine, e.g. a VM, then specify `--remote`,
otherwise leave it absent from the *load.py script invocation. It helps if
your local and remote machines are running the same build of the kernel. This
is because `{fast,slow}load.py --symbols` makes a copy of `/proc/kallsyms`.

```basemake
python scripts/slowload.py --symbols
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
make clean KERNEL=1 ; make all KERNEL=1
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
python scripts/slowload.py --remote
```

Note: if you are loading Granary into a VM, then it is suggested to use
clang/clang++ as the compiler toolchain (as exampled above), because the debugging
sections emitted by clang are orders of magnitude smaller, which improves the
execution time of the `slowload.py` script. Alternatively, the `fastload.py` script
can be used, which requires some extra SSH setup, but greatly improves load times
regardless of the compiler. The `fastload.py` script is the preferred mechanism for
loading; however, `slowload.py` requires less setup.

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

Compiling for user space
------------------------
Note: Granary is a kernel-space DBT system first. The user space implementation is
highly experimental and mostly suited toward prototyping and debugging small Granary
features.

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
