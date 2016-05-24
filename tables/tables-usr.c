#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Kinda assuming x86 here. */
#define _PAGE_PRESENT (1UL<<0)

int main(void)
{
	int i;
	unsigned long entry;
	size_t word_size = sizeof(unsigned long);
	long ptrs_per_pgd = sysconf(_SC_PAGESIZE)/word_size;
	unsigned long flags_mask = ptrs_per_pgd - 1;
	unsigned long phys_addr_mask = ~flags_mask;

	FILE *file = fopen("/sys/kernel/debug/tables/pgd", "r");
	if (!file) {
		perror("tables: error");
		return EXIT_FAILURE;
	}

	for (i = 0; i < ptrs_per_pgd; i++) {
		unsigned long phys_addr, flags;

		if (fread(&entry, 1, word_size, file) != word_size) {
			fprintf(stderr, "tables: error: read error\n");
			fclose(file);
			return EXIT_FAILURE;
		}

		flags = entry&flags_mask;
		if (!(flags&_PAGE_PRESENT))
			continue;

		phys_addr = entry&phys_addr_mask;
		printf("%03d: %016lx\n", i, phys_addr);
	}

	return EXIT_SUCCESS;
}
