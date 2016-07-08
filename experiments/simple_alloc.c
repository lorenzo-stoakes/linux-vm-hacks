#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PAGES      1025
#define ADDITIONAL 69

int main(void)
{
	long i;
	long page_size = sysconf(_SC_PAGESIZE);
	long bytes = PAGES*page_size + ADDITIONAL;
	unsigned char *buf = malloc(bytes);

	for (i = 0; i < bytes; i++)
		buf[i] = 'x';

	free(buf);

	return EXIT_SUCCESS;
}
