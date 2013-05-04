Setting up a virtual machine (VM) for testing Granary
=====================================================

## Pre-requisites

  1. Make sure QEMU is installed.
  2. Make sure KVM is installed.
  3. Enable hardward virtualization.
  4. Download a Linux distribution disk image (.iso) file.

## Step 1: Create a VM image.

```basemake
qemu-img create -f qcow2 vm.img 20G
```

## Step 2: Install the distribution into the image

This will launch the VM and allow you to run through the OS's
installing process.

```basemake
$(which qemu-system-x86_64) -m 2048 -hda vm.img -cdrom <path-to-your-iso> -boot d
```

## Step 3: Create a convenient VM-launcher script

For example:
```basemake
#!/bin/bash

QEMU=$(which qemu-system-x86_64)

SMP=2
MEM=2048

# e1000 is a good choice if you want you want to debug a network driver
NETWORK=virtio 

$QEMU -cpu host -enable-kvm -smp $SMP -m $MEM -hda <path-to-vm.img> -net nic,model=$NETWORK -net "user,hostfwd=tcp::5556-:22" -gdb tcp::9999  &
```

## Step 4: Install SSH and SSH server

We need these so that Granary's `load.py` script can copy files
to and from the VM. Make sure that both SSH and the SSH server
are installed on your host machine, as well as in the guest OS that
runs within the VM.

```basemake
sudo apt-get install ssh
sudo apt-get install openssh-server
ssh-keygen
```

On your host OS, run:

```basemake
ssh-copy-id "<vm-user-name>@localhost -P 5556"
```

In your guest OS, run:

```basemake
ssh-copy-id "<host-user-name>@10.0.2.2 -P 22"
```

## Step 5: Install your build of the kernel into the guest OS

For this step, we will need kernel image and header packages in the
Debian kernel package format. See [Building the Kernel](building-the-kernel.md)
for instructions on how to generate these files.

Copy the Debian kernel packages into your VM, and install them
in the way described in the aforementioned document.
