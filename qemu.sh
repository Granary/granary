#!/bin/bash

SMP=2
MEM=2048
NETWORK=e1000
#NETWORK=virtio
GRAPHIC= #-nographic
GRAPHIC_FLAG=0

QEMU=$(which qemu-system-x86_64)
$QEMU -cpu host -enable-kvm -smp $SMP -m $MEM $GRAPHIC -hda /home/pag/Code/VM/vm.img -net nic,model=$NETWORK -net "user,hostfwd=tcp::5556-:22" -gdb tcp::9999  &


