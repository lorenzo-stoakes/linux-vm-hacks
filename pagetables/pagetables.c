#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/sched.h>
#include <asm/pgtable.h>

/*
 * TODO: Is possible page reads could be non-atomic, check.
 * TODO: Probably needs some locking, check.
 */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lorenzo Stoakes <lstoakes@gmail.com>");
MODULE_DESCRIPTION("Simple experimental tool for extracting page tables.");

static u64 vaddr;

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
	pgd_t *pgdp, pgd;
	pud_t *pudp;

	pgdp = pgd_offset(current->mm, vaddr);
	pgd = *pgdp;
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
	pgd_t *pgdp, pgd;
	pud_t *pudp, pud;
	pmd_t *pmdp;

	pgdp = pgd_offset(current->mm, vaddr);
	pgd = *pgdp;
	if (pgd_none(pgd) || pgd_bad(pgd))
		return -EINVAL;

	pudp = pud_offset(pgdp, vaddr);
	pud = *pudp;
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

static ssize_t pte_read(struct file *file, char __user *out, size_t size,
			loff_t *off)
{
	pgd_t *pgdp, pgd;
	pud_t *pudp, pud;
	pmd_t *pmdp, pmd;
	pte_t *ptep;

	pgdp = pgd_offset(current->mm, vaddr);
	pgd = *pgdp;
	if (pgd_none(pgd) || pgd_bad(pgd))
		return -EINVAL;

	pudp = pud_offset(pgdp, vaddr);
	pud = *pudp;
	if (pud_none(pud) || pud_bad(pud))
		return -EINVAL;

	pmdp = pmd_offset(pudp, vaddr);
	pmd = *pmdp;
	if (pmd_none(pmd) || pmd_bad(pmd))
		return -EINVAL;

	/* TODO: Perhaps we should enforce a lock here. */
	ptep = (pte_t *)pmd_page_vaddr(pmd);

	return simple_read_from_buffer(out, size, off, ptep,
				sizeof(unsigned long) * PTRS_PER_PTE);
}

static const struct file_operations pte_fops = {
	.owner = THIS_MODULE,
	.read = pte_read
};

static int __init pagetables_init(void)
{
	struct dentry *filep;

	filep = debugfs_create_dir("pagetables", NULL);
	if (IS_ERR_OR_NULL(filep))
		goto error;
	else
		pagetables_dir = filep;


	filep = debugfs_create_x64("vaddr", 0600, pagetables_dir,
				&vaddr);
	if (IS_ERR_OR_NULL(filep))
		goto error;

	filep = debugfs_create_file("pgd", 0400, pagetables_dir, NULL,
				&pgd_fops);
	if (IS_ERR_OR_NULL(filep))
		goto error;

	filep = debugfs_create_file("pud", 0400, pagetables_dir, NULL,
				&pud_fops);
	if (IS_ERR_OR_NULL(filep))
		goto error;

	filep = debugfs_create_file("pmd", 0400, pagetables_dir, NULL,
				&pmd_fops);
	if (IS_ERR_OR_NULL(filep))
		goto error;

	filep = debugfs_create_file("pte", 0400, pagetables_dir, NULL,
				&pte_fops);
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
