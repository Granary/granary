
# evil hack to get all the kernel's config macros spit out.
EXTRA_CFLAGS = -dM -E

obj-m += empty.o

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

macros:
	make -w -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
