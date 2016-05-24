KERNEL_TREE_PATH?=/lib/modules/$(shell uname -r)/build

obj-m += tables.o

all: tables tables.ko

tables: tables-usr.c
	$(CC) -g -pedantic -Wall -Werror -o $@ $^

tables.ko: tables.c
	make -C $(KERNEL_TREE_PATH) M=$(PWD) modules

clean:
	rm -f tables
	make -C $(KERNEL_TREE_PATH) M=$(PWD) clean

.PHONY: all clean
