Building and Installing the Kernel
==================================

Granary requires that a modified version of the kernel be run.
The modifications amount to altering the kernel thread stack size
so that Granary doesn't blow the stack and crash the kernel.

A useful TODO for someone to try out that would circumvent part of
the necessity to grow the stack size would be to have Granary switch
to a CPU-private stack when it enters the code cache lookup functions.

## Prerequisites

```basemake
sudo apt-get install libncurses5-dev gcc make git exuberant-ctags
sudo apt-get install dpkg kernel-package
```

## Step 1: Download the Linux kernel sources.

```basemake
wget https://www.kernel.org/pub/linux/kernel/v3.x/linux-3.8.tar.xz
```

Uncompress this archive somewhere, e.g. `~/Code/linux-3.8`, and
navigate to this directory in your terminal.

## Step 2: Increase kernel stack sizes.

In `arch/ia64/include/asm/ptrace.h` modify the following:

```c
#define IA64_STK_OFFSET         ((1 << KERNEL_STACK_SIZE_ORDER)*PAGE_SIZE) /* old */
#define IA64_STK_OFFSET         (8*PAGE_SIZE) /* new */
```

In `include/asm/page_64_types.h`, modify the following:

```c
#define THREAD_SIZE_ORDER   1 /* old */
#define THREAD_SIZE_ORDER   3 /* new */
```

```c
#define IRQ_STACK_ORDER 2 /* old */
#define IRQ_STACK_ORDER 3 /* new */
```

## Step 3: Add in support for TLS

Open up `include/linux/sched.h` and find the definition of `struct task_struct`. At
the bottom of the definition of `struct task_struct`, add in the following additional
field:

```c
struct task_struct {
    ...
    void *granary;
}
```

If you want to give granary more task-local state to play with then you can do something like this:

```c
struct task_struct {
    ...
    struct {
        unsigned long granary_data[10];
    } granary;
}
```

Giving Granary more task-local state can be beneficial for tools that make heavier use of task-local state.

## Step 4: Configure and build the kernel

First, copy your current kernel configuration, as that makes life
easier.

```basemake
cp /boot/config-`uname -r`* .config
```

Next, let's double-check our kernel configuration. When working with
Granary, you likely want to make sure that:

  - Loadable kernel module support is enabled.
  - Virtualization is enabled.
  - `ftrace` (Kernel Hacking > Tracing) is disabled.
  - `CONFIG_ARCH_RANDOM` is disabled.
  - `CONFIG_INTEL_TXT` is disabled.
  - `CONFIG_SECURITY` is disabled.
  - All (interesting) file systems (except the file system that you're
    using) are selected as loadable kernel modules (M).

You can do this by modifying your configuration using either of the
following two commands:

```basemake
make menuconfig
make nconfig
```

Finally, build your kernel.

```basemake
make -j 8
```

If in doubt, consult the [Kernel Newbies](http://kernelnewbies.org/KernelBuild) website.

## Step 5: Create a Debian kernel package

Run the following command in the Terminal.

```basemake
sudo fakeroot make-kpkg --initrd --append-to-version=granary kernel_image kernel_headers
```

## Step 6: Install the kernel

```basemake
sudo dpkg -i ../linux-headers-3.8.2granary_3.8.2granary-10.00.Custom_amd64.deb
sudo dpkg -i ../linux-image-3.8.2granary_3.8.2granary-10.00.Custom_amd64.deb

```
