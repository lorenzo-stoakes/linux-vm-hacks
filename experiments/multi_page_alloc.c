/*
 * NOTE: Assumes x86-64, 4KiB pages.
 *
 * Allocates some memory, then touches a page, then touches other pages in order
 * to allow observation of the changes of page table allocation + demand
 * paging.
 *
 * Blocks waiting for input at each case.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PAGE_SIZE 4096

#define SIZE (PAGE_SIZE * 11)

#if SIZE < PAGE_SIZE*7
#error SIZE needs to be at least 7 pages long.
#endif

#define PGDIR_SHIFT 39
#define PUD_SHIFT   30
#define PMD_SHIFT   21
#define PAGE_SHIFT  12

static void block(void)
{
	getchar();
}

static void print_addr(unsigned char *ptr)
{
	unsigned long pgd_ind, pud_ind, pmd_ind, pte_ind;
	unsigned long addr = (unsigned long)ptr;

	pgd_ind = addr>>PGDIR_SHIFT;
	addr &= ((1UL<<PGDIR_SHIFT)-1);

	pud_ind = addr>>PUD_SHIFT;
	addr &= ((1UL<<PUD_SHIFT)-1);

	pmd_ind = addr>>PMD_SHIFT;
	addr &= ((1UL<<PMD_SHIFT)-1);

	pte_ind = addr>>PAGE_SHIFT;
	addr &= ((1UL<<PAGE_SHIFT)-1);

	printf("Address = %016lx\n", addr);
	printf("PGD/PUD/PMD/PTE/offset=%lu/%lu/%lu/%lu/%lu\n",
		pgd_ind, pud_ind, pmd_ind, pte_ind, addr);
}

int main(void)
{
	unsigned char *ptr;

	puts("Before alloc.");
	block();

	ptr =  malloc(SIZE);
	puts("Alloc'ed.");
	print_addr(ptr);
	block();

	ptr[0] = '1';
	puts("Touched 1st page.");
	block();

	ptr[PAGE_SIZE] = '2';
	puts("Touched 2nd page.");
	block();

	ptr[PAGE_SIZE * 7] = '7';
	puts("Touched 7th page.");
	block();

	memset(ptr, 'x', SIZE);
	puts("Memset.");
	block();

	free(ptr);
	puts("Freed.");
	block();

	return EXIT_SUCCESS;
}
