#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Kinda assuming x86 here. */
#define _PAGE_PRESENT (1UL<<0)

int main(void)
{
	int i;
	unsigned long *buf;
	int err = EXIT_SUCCESS;
	long page_size = sysconf(_SC_PAGESIZE);
	long ptrs_per_pgd = page_size/sizeof(unsigned long);
	unsigned long flags_mask = ptrs_per_pgd - 1;
	unsigned long phys_addr_mask = ~flags_mask;

	FILE *file = fopen("/sys/kernel/debug/tables/pgd", "r");
	if (!file) {
		perror("tables: error");
		return EXIT_FAILURE;
	}

	buf = malloc(page_size);
	if (fread(buf, 1, page_size, file) != page_size) {
		fprintf(stderr, "tables: error: read error\n");
		err = EXIT_FAILURE;
		goto done;
	}

	for (i = 0; i < ptrs_per_pgd; i++) {
		unsigned long curr = buf[i];
		unsigned long phys_addr = curr&phys_addr_mask;
		unsigned long flags = curr&flags_mask;

		if (!(flags&_PAGE_PRESENT))
			continue;
		printf("%03d: %016lx\n", i, phys_addr);
	}

done:
	fclose(file);
	free(buf);

	return err;
}
