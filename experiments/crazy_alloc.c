#include <stdio.h>
#include <stdlib.h>

#define SIZE (1UL * 1024 * 1024 * 1024 * 8)

int main(void)
{
	if (!malloc(SIZE)) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
