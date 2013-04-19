
KDIR := /lib/modules/$(shell uname -r)/build
MDIR := $(shell pwd)

obj-m += leak.o

leak-objs := mem_leak.o

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(MDIR) modules

clean:
	rm -rf *.o
	rm -rf *.ko


