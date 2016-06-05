#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if __SIZEOF_POINTER__ != 8
#error 64-bit only, sorry.
#endif

/* Kinda assuming x86-64 here. */
#define _PAGE_PRESENT  (1UL<<0)
#define _PAGE_RW       (1UL<<1)
#define _PAGE_ACCESSED (1UL<<5)
#define _PAGE_DIRTY    (1UL<<6)
#define _PAGE_PSE      (1UL<<7)
#define _PAGE_NX       (1UL<<63)
#define PTRS_PER_PGD   512
#define PTRS_PER_PUD   512
#define PTRS_PER_PMD   512
#define PTRS_PER_PTE   512
#define PGDIR_SHIFT    39
#define PUD_SHIFT      30
#define PMD_SHIFT      21
#define PAGE_SHIFT     12

#define PAGE_BITS     12
#define PAGE_SIZE     (1<<12)
#define MAX_PHYS_MASK ((1UL<<46)-1)

#define WORD_SIZE (sizeof(unsigned long))
#define DEBUGFS_PATH "/sys/kernel/debug/pagetables/"
#define VADDR_PATH DEBUGFS_PATH "vaddr"
#define TARGET_PID_PATH DEBUGFS_PATH "pid"

#define FLAGS_MIN_BITS PAGE_BITS
#define HIDE_KERNEL    1

enum pgtable_level {
	PGD_LEVEL,
	PUD_LEVEL,
	PMD_LEVEL,
	PTE_LEVEL,
	LEVEL_COUNT
};

static char *level_name[LEVEL_COUNT+1] = {
	"PGD",
	"PUD",
	"PMD",
	"PTE",
	"Physical"
};

static char *level_path[LEVEL_COUNT] = {
	DEBUGFS_PATH "pgd",
	DEBUGFS_PATH "pud",
	DEBUGFS_PATH "pmd",
	DEBUGFS_PATH "pte"
};

static int level_size[LEVEL_COUNT] = {
	PTRS_PER_PGD,
	PTRS_PER_PUD,
	PTRS_PER_PMD,
	PTRS_PER_PTE
};

static int level_shift[LEVEL_COUNT] = {
	PGDIR_SHIFT,
	PUD_SHIFT,
	PMD_SHIFT,
	PAGE_SHIFT
};

static unsigned long level_mask[LEVEL_COUNT] = {
	(PTRS_PER_PGD-1UL)<<PGDIR_SHIFT,
	(PTRS_PER_PUD-1UL)<<PUD_SHIFT,
	(PTRS_PER_PMD-1UL)<<PMD_SHIFT,
	(PTRS_PER_PTE-1UL)<<PAGE_SHIFT
};

static char *human_suffix[] = {
	"",
	"KiB",
	"MiB",
	"GiB",
	"TiB"
};

static unsigned long vaddr;
static int page_counts[LEVEL_COUNT];

static void set_target_pid(char *pid_str)
{
	FILE *file = fopen(TARGET_PID_PATH, "r+");
	int len = strlen(pid_str);

	if (!file) {
		fprintf(stderr,
			"pagetables: set_target_pid: error opening %s: %s\n",
			TARGET_PID_PATH, strerror(errno));
		exit(1);
	}

	if (fwrite(pid_str, 1, len, file) != len) {
		fprintf(stderr, "pagetables: set_target_pid: write error at %s\n",
			TARGET_PID_PATH);
		exit(1);
	}

	fclose(file);
}

static void print_human_bytes(unsigned long bytes)
{
	int suffix_ind = 0;
	double bytesf = (double)bytes;

	while (bytesf > 1024) {
		suffix_ind++;
		bytesf /= 1024;
	}

	printf("%6.1f %s", bytesf, human_suffix[suffix_ind]);
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
		printf("   ");
}

static void update_vaddr(enum pgtable_level level, int index)
{
	vaddr &= ~level_mask[level];
	vaddr |= ((unsigned long)index << level_shift[level]);
}

static void sync_vaddr(void)
{
	char buf[sizeof("0xdeadbeefdeadbeef")];
	FILE *file = fopen(VADDR_PATH, "r+");
	int len;

	if (!file) {
		fprintf(stderr,
			"pagetables: sync_vaddr: error opening %s: %s\n",
			VADDR_PATH, strerror(errno));
		exit(1);
	}

	len = snprintf(buf, sizeof(buf), "0x%lx", vaddr);
	if (len >= sizeof(buf)) {
		fprintf(stderr,
			"pagetables: sync_vaddr: attempted write %d, >= %lu.\n",
			len, sizeof(buf));
		exit(1);
	}

	if (fwrite(buf, 1, len, file) != len) {
		fprintf(stderr, "pagetables: write error at %s\n", VADDR_PATH);
		exit(1);
	}

	fclose(file);
}

static void update_sync_vaddr(enum pgtable_level level, int index)
{
	update_vaddr(level, index);
	sync_vaddr();
}

static void print_pagetable(enum pgtable_level level)
{
	int i;
	unsigned long entry;
	/* Ignoring transparent huge pages, etc. TODO: Deal properly. */
	int count = level_size[level];
	unsigned long phys_addr_mask = (~(count - 1)) & MAX_PHYS_MASK;
	unsigned long flags_mask = ~phys_addr_mask;

	FILE *file = fopen(level_path[level], "r");

	if (!file) {
		fprintf(stderr, "pagetables: error opening %s: %s\n",
			level_path[level], strerror(errno));
		exit(1);
	}

	/* Top half of PGD entries -> kernel mappings. */
	if (HIDE_KERNEL && level == PGD_LEVEL)
		count /= 2;

	for (i = 0; i < count; i++) {
		unsigned long phys_addr, flags, present, huge;

		if (fread(&entry, 1, WORD_SIZE, file) != WORD_SIZE) {
			fprintf(stderr, "pagetables: error: read error: %s\n",
				strerror(errno));
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
		printf("%03d ", i);
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
			update_sync_vaddr(level, i);

			print_pagetable(level + 1);
		}

		/* Each entry is a page of the next level. */
		page_counts[level+1]++;
	}

	fclose(file);
}

static void print_counts(void)
{
	int i, count;
	int total = 0;

	puts("\n== Page Counts ==\n");
	/* <= to include physical pages too. */
	for (i = 1; i <= LEVEL_COUNT; i++) {
		count = page_counts[i];

		printf("%s pages:\t%8d (", level_name[i], count);
		print_human_bytes((unsigned long)count * PAGE_SIZE);
		printf(")\n");

		total += count;
	}

	printf("\nTOTAL:\t\t%8d (", total);
	print_human_bytes((unsigned long)total * PAGE_SIZE);
	printf(")\n");
}

int main(int argc, char *argv[])
{
	if (argc > 1)
		set_target_pid(argv[1]);

	print_pagetable(PGD_LEVEL);
	print_counts();

	return EXIT_SUCCESS;
}
