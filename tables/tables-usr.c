#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WORD_SIZE sizeof(unsigned long)

int main(void)
{
	int i;
	int err = EXIT_SUCCESS;
	long page_size = sysconf(_SC_PAGESIZE);
	unsigned long *buf;

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

	for (i = 0; i < page_size/WORD_SIZE; i++) {
		unsigned long curr = buf[i];

		if (!curr)
			continue;
		printf("%03d: %016lx\n", i, curr);
	}

done:
	fclose(file);
	free(buf);

	return err;
}
