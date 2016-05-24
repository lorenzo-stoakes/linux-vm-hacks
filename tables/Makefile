KERNEL_TREE_PATH?=/lib/modules/$(shell uname -r)/build

obj-m += tables.o

all: tables.ko

tables.ko: tables.c
	make -C $(KERNEL_TREE_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_TREE_PATH) M=$(PWD) clean

.PHONY: all clean
