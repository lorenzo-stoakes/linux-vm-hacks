#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Kinda assuming x86 here. */
#define _PAGE_PRESENT (1UL<<0)
#define PAGE_BITS     12

#define WORD_SIZE (sizeof(unsigned long))
#define DEBUGFS_PATH "/sys/kernel/debug/pagetables/"

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

static void print_pagetable(char *path, int count)
{
	int i;
	unsigned long entry;
	unsigned long flags_mask = count - 1;
	unsigned long phys_addr_mask = ~flags_mask;

	FILE *file = fopen(path, "r");
	if (!file) {
		perror("pagetables: error");
		return;
	}

	for (i = 0; i < count; i++) {
		unsigned long phys_addr, flags;

		if (fread(&entry, 1, WORD_SIZE, file) != WORD_SIZE) {
			fprintf(stderr, "pagetables: error: read error\n");
			fclose(file);
			return;
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

	fclose(file);
}

int main(void)
{
	int ptrs_per_pgd = (int)sysconf(_SC_PAGESIZE)/WORD_SIZE;

	/* Top half of PGD entries -> kernel mappings. */
	if (SKIP_KERNEL)
		ptrs_per_pgd /= 2;

	print_pagetable(DEBUGFS_PATH "pgd", ptrs_per_pgd);

	return EXIT_SUCCESS;
}
