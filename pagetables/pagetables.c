#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/sched.h>
#include <asm/pgtable.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lorenzo Stoakes <lstoakes@gmail.com>");
MODULE_DESCRIPTION("Simple experimental tool for extracting page tables.");

static size_t pgdindex, pudindex;

static struct dentry *pagetables_dir;

static ssize_t pgd_read(struct file *file, char __user *out, size_t size,
			loff_t *off)
{
	return simple_read_from_buffer(out, size, off, current->mm->pgd,
				sizeof(unsigned long) * PTRS_PER_PGD);
}

static const struct file_operations pgd_fops = {
	.owner = THIS_MODULE,
	.read = pgd_read
};

static ssize_t pud_read(struct file *file, char __user *out, size_t size,
			loff_t *off)
{
	pgd_t pgd;
	pud_t *pudp;

	if (pgdindex >= PTRS_PER_PGD)
		return -EINVAL;

	pgd = current->mm->pgd[pgdindex];
	if (pgd_none(pgd) || pgd_bad(pgd))
		return -EINVAL;

	pudp = (pud_t *)pgd_page_vaddr(pgd);

	return simple_read_from_buffer(out, size, off, pudp,
				sizeof(unsigned long) * PTRS_PER_PUD);
}

static const struct file_operations pud_fops = {
	.owner = THIS_MODULE,
	.read = pud_read
};

static ssize_t pmd_read(struct file *file, char __user *out, size_t size,
			loff_t *off)
{
	pgd_t pgd;
	pud_t *pudp, pud;
	pmd_t *pmdp;

	if (pgdindex >= PTRS_PER_PGD || pudindex >= PTRS_PER_PUD)
		return -EINVAL;

	pgd = current->mm->pgd[pgdindex];
	if (pgd_none(pgd) || pgd_bad(pgd))
		return -EINVAL;

	pudp = (pud_t *)pgd_page_vaddr(pgd);
	pud = pudp[pudindex];
	if (pud_none(pud) || pud_bad(pud))
		return -EINVAL;

	pmdp = (pmd_t *)pud_page_vaddr(pud);

	return simple_read_from_buffer(out, size, off, pmdp,
				sizeof(unsigned long) * PTRS_PER_PMD);
}

static const struct file_operations pmd_fops = {
	.owner = THIS_MODULE,
	.read = pmd_read
};

static int __init pagetables_init(void)
{
	struct dentry *filep;

	filep = debugfs_create_dir("pagetables", NULL);
	if (IS_ERR_OR_NULL(filep))
		goto error;
	else
		pagetables_dir = filep;

	filep = debugfs_create_file("pgd", 0400, pagetables_dir, NULL,
				&pgd_fops);
	if (IS_ERR_OR_NULL(filep))
		goto error;

	filep = debugfs_create_size_t("pgdindex", 0600, pagetables_dir,
				&pgdindex);
	if (IS_ERR_OR_NULL(filep))
		goto error;

	filep = debugfs_create_file("pud", 0400, pagetables_dir, NULL,
				&pud_fops);
	if (IS_ERR_OR_NULL(filep))
		goto error;

	filep = debugfs_create_size_t("pudindex", 0600, pagetables_dir,
				&pudindex);
	if (IS_ERR_OR_NULL(filep))
		goto error;

	filep = debugfs_create_file("pmd", 0400, pagetables_dir, NULL,
				&pmd_fops);
	if (IS_ERR_OR_NULL(filep))
		goto error;

	return 0;

error:
	debugfs_remove_recursive(pagetables_dir);
	return filep ? PTR_ERR(filep) : -ENOMEM;
}

static void __exit pagetables_exit(void)
{
	/* Does the right thing if pagetables_dir NULL. */
	debugfs_remove_recursive(pagetables_dir);
}

module_init(pagetables_init);
module_exit(pagetables_exit);
