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

## Step 2: Modify the kernel to make it have larger stacks.

In `arch/ia64/include/asm/ptrace.h` modify the following:

```c
#define IA64_STK_OFFSET         ((1 << KERNEL_STACK_SIZE_ORDER)*PAGE_SIZE)
#define IA64_STK_OFFSET         (8*PAGE_SIZE)
```

In `include/asm/page_64_types.h`, modify the following:

```c
#define THREAD_SIZE_ORDER   1
#define THREAD_SIZE_ORDER   3
```

```c
#define IRQ_STACK_ORDER 2
#define IRQ_STACK_ORDER 3
```

## Step 3: Configure and build the kernel

First, copy your current kernel configuration, as that makes life
easier.

```basemake
cp /boot/config-`uname -r`* .config
```

Next, let's double-check our kernel configuration. When working with
Granary, you likely want to make sure that:

  - Loadable kernel module support is enabled.
  - Virtualization is enabled.
  - All (interesting) file systems (except the file system that you're
    using) are selected as loadabl kernel modules (M).

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

## Step 4: Create a Debian kernel package

Run the following command in the 

```basemake
sudo fakeroot make-kpkg --initrd --append-to-version=granary kernel_image kernel_headers
```

## Step 5: Install the kernel

```basemake
sudo dpkg -i ../linux-headers-3.8.2granary_3.8.2granary-10.00.Custom_amd64.deb
sudo dpkg -i ../linux-image-3.8.2granary_3.8.2granary-10.00.Custom_amd64.deb

```