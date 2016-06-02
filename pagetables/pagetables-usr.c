#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Kinda assuming x86 here. */
#define _PAGE_PRESENT (1UL<<0)
#define PAGE_BITS     12

#define SKIP_KERNEL 1

void print_bin(unsigned long val)
{
	int i;
	char buf[PAGE_BITS];

	buf[PAGE_BITS] = '\0';
	for (i = 0; i < PAGE_BITS && val != 0; i++) {
		buf[PAGE_BITS-1-i] = (val&1) ? '1' : '0';
		val >>= 1;
	}

	printf("%s", buf + PAGE_BITS - i);
}

int main(void)
{
	int i;
	unsigned long entry;
	size_t word_size = sizeof(unsigned long);
	long ptrs_per_pgd = sysconf(_SC_PAGESIZE)/word_size;
	unsigned long flags_mask = ptrs_per_pgd - 1;
	unsigned long phys_addr_mask = ~flags_mask;

	FILE *file = fopen("/sys/kernel/debug/pagetables/pgd", "r");
	if (!file) {
		perror("pagetables: error");
		return EXIT_FAILURE;
	}

	/* Top half of PGD entries -> kernel mappings. */
	if (SKIP_KERNEL)
		ptrs_per_pgd /= 2;

	for (i = 0; i < ptrs_per_pgd; i++) {
		unsigned long phys_addr, flags;

		if (fread(&entry, 1, word_size, file) != word_size) {
			fprintf(stderr, "pagetables: error: read error\n");
			fclose(file);
			return EXIT_FAILURE;
		}

		/* Skip empty entries. */
		if (!entry)
			continue;

		flags = entry&flags_mask;
		phys_addr = entry&phys_addr_mask;

		printf("%03d: ", i);
		if (flags&_PAGE_PRESENT)
			printf("%016lx ", phys_addr);
		else
			printf("<swapped> ");

		print_bin(flags);

		printf("\n");
	}

	return EXIT_SUCCESS;
}
