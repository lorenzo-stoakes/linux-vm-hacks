#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Kinda assuming x86 here. */
#define _PAGE_PRESENT  (1UL<<0)
#define _PAGE_RW       (1UL<<1)
#define _PAGE_ACCESSED (1UL<<5)
#define _PAGE_DIRTY    (1UL<<6)
#define _PAGE_PSE      (1UL<<7)
#define _PAGE_NX       (1UL<<63)

#define PAGE_BITS     12
#define MAX_PHYS_MASK ((1UL<<46)-1)

#define WORD_SIZE (sizeof(unsigned long))
#define DEBUGFS_PATH "/sys/kernel/debug/pagetables/"
#define DEBUGFS_PATH_LEN (sizeof(DEBUGFS_PATH))

#define FLAGS_MIN_BITS PAGE_BITS
#define HIDE_KERNEL    1

enum pgtable_level {
	PGD_LEVEL,
	PUD_LEVEL,
	PMD_LEVEL,
	PTE_LEVEL,
	LEVEL_COUNT
};

char *level_name[LEVEL_COUNT] = {
	"pgd",
	"pud",
	"pmd",
	"pte"
};

static char *get_level_path(enum pgtable_level level, int is_index)
{
	char buf[DEBUGFS_PATH_LEN + sizeof("xxxindex")];

	sprintf(buf, DEBUGFS_PATH "%s%s", level_name[level],
		is_index ? "index" : "");

	return strdup(buf);
}

static void print_bin(unsigned long val, int min_len)
{
	int i;
	char buf[WORD_SIZE + 1];

	buf[WORD_SIZE] = '\0';
	for (i = 0; i < WORD_SIZE && val != 0; i++) {
		buf[WORD_SIZE-1-i] = (val&1) ? '1' : '0';
		val >>= 1;
	}

	for (; i < min_len; i++)
		buf[WORD_SIZE-1-i] = '0';

	printf("%s", buf + WORD_SIZE - i);
}

static void print_indent(int level)
{
	int i;

	for (i = 0; i < level; i++)
		printf("\t");
}

static void set_pgtable_index(int level, int index)
{
	char index_str[4];
	char *path = get_level_path(level, 1);
	int len = sprintf(index_str, "%d", index);
	FILE *file = fopen(path, "r+");

	if (!file) {
		fprintf(stderr,
			"pagetables: set_pgtable_index: error opening %s: %s\n",
			path, strerror(errno));
		exit(1);
	}

	if (fwrite(index_str, 1, len, file) != len) {
		fprintf(stderr, "pagetables: write error at %s\n", path);
		exit(1);
	}

	free(path);
	fclose(file);
}

static void print_pagetable(enum pgtable_level level)
{
	int i;
	unsigned long entry;
	/* Ignoring transparent huge pages, etc. TODO: Deal properly. */
	int count = (int)sysconf(_SC_PAGESIZE)/WORD_SIZE;
	unsigned long phys_addr_mask = (~(count - 1)) & MAX_PHYS_MASK;
	unsigned long flags_mask = ~phys_addr_mask;

	char *path = get_level_path(level, 0);
	FILE *file = fopen(path, "r");

	if (!file) {
		fprintf(stderr, "pagetables: error opening %s: %s\n", path,
			strerror(errno));
		exit(1);
	}
	free(path);

	/* Top half of PGD entries -> kernel mappings. */
	if (HIDE_KERNEL && level == PGD_LEVEL)
		count /= 2;

	for (i = 0; i < count; i++) {
		unsigned long phys_addr, flags, present, huge;

		if (fread(&entry, 1, WORD_SIZE, file) != WORD_SIZE) {
			fprintf(stderr, "pagetables: error: read error\n");
			exit(1);
		}

		/* Skip empty entries. */
		if (!entry)
			continue;

		phys_addr = entry&phys_addr_mask;
		flags = entry&flags_mask;
		present = flags&_PAGE_PRESENT;
		/* TODO: We just don't deal with huge pages yet, fix. */
		huge = flags&_PAGE_PSE;

		print_indent(level);
		printf("%03d: ", i);
		if (huge)
			printf("<huge> ");
		else if (present)
			printf("%016lx ", phys_addr);
		else
			printf("<swapped> ");

		print_bin(flags, FLAGS_MIN_BITS);

		if (flags&_PAGE_NX)
			printf(" NX");

		printf("\n");

		if (present && !huge && level < LEVEL_COUNT-1) {
			set_pgtable_index(level, i);
			print_pagetable(level + 1);
		}
	}

	fclose(file);
}

int main(void)
{
	print_pagetable(PGD_LEVEL);

	return EXIT_SUCCESS;
}
