#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lorenzo Stoakes <lstoakes@gmail.com>");
MODULE_DESCRIPTION("Simple experimental tool for extracting page tables.");

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

static int __init pagetables_init(void)
{
	struct dentry *pfile;

	pfile = debugfs_create_dir("pagetables", NULL);
	if (IS_ERR_OR_NULL(pfile))
		goto error_dir;
	else
		pagetables_dir = pfile;

	pfile = debugfs_create_file("pgd", 0400, pagetables_dir, NULL,
				&pgd_fops);
	if (IS_ERR_OR_NULL(pfile))
		goto error;

	return 0;

 error:
	debugfs_remove_recursive(pagetables_dir);
 error_dir:
	return pfile ? PTR_ERR(pfile) : -ENOMEM;
}

static void __exit pagetables_exit(void)
{
	/* Does the right thing if pagetables_dir NULL. */
	debugfs_remove_recursive(pagetables_dir);
}

module_init(pagetables_init);
module_exit(pagetables_exit);
