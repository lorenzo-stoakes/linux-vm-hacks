KERNEL_TREE_PATH?=/lib/modules/$(shell uname -r)/build

obj-m += pagetables.o

all: pagetables pagetables.ko

pagetables: pagetables-usr.c
	$(CC) -g -pedantic -Wall -Werror -o $@ $^

pagetables.ko: pagetables.c
	make -C $(KERNEL_TREE_PATH) M=$(PWD) modules

clean:
	rm -f pagetables
	make -C $(KERNEL_TREE_PATH) M=$(PWD) clean

.PHONY: all clean
