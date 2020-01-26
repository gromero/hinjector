#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gustavo Romero");
MODULE_DESCRIPTION("injector");
MODULE_VERSION("v0.1");

#define I_SIZE  4  /* on PPC64 <= 9, instruction size is always 32-bit */

ssize_t write_operation(struct file *, const char __user*, size_t, loff_t *);
int open_operation(struct inode *, struct file *);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.write = write_operation,
	.open = open_operation,
};

#define MAX_CACHE_SIZE 1024
static char codecache[MAX_CACHE_SIZE]; /* 1KiB */

static int device_major_number;

// static int open_counter;

static int register_device(void)
{
	int r;
	r = register_chrdev(0, "injector", &fops);
	if (r < 0 ) {
		printk(KERN_WARNING "injector: can't register device\n");
		return r;
	}
	device_major_number = r;
	printk(KERN_NOTICE "injector: register with major device number %i\n", device_major_number);
	return 0;
}

static void unregister_device(void)
{
	printk(KERN_NOTICE "injector: device unregistered\n");
	if (device_major_number != 0)
		unregister_chrdev(device_major_number, "injector");
}

ssize_t write_operation(struct file *f, const char __user *dst, size_t count, loff_t *offp)
{
	int instr, i;
	size_t received;

	// stash number of received bytes before any truncation
	received = count;

	if (count < I_SIZE) {
		printk ("Nack, you should at least provide 1 instruction (32-bit) to execute\n");
		return count;
	} else if (count % I_SIZE) {
		printk("Nack, you should inject a number of bytes aligned to instruction size (32-bit)\n");
		count = count & ~(I_SIZE - 1);
		printk("Truncating to %ld byte(s)\n", count);
	} else if (count > MAX_CACHE_SIZE) {
		count = MAX_CACHE_SIZE;
	}

	printk("Dumping %ld bytes from userspace @%p\n", count, codecache);
	if (copy_from_user(codecache, dst, count)) {
		return -EFAULT;
	}

	/* dump instructions in codecache */
	for (instr = 0; instr < count; instr += I_SIZE) {
		printk("I[%d]: ", instr / I_SIZE);
		for (i = 0; i < I_SIZE && instr+i < count; i++)
			printk(KERN_CONT "%.2x", codecache[instr+i]);
		printk(KERN_CONT "\n");
	}
	return received;
}

int open_operation(struct inode *inode, struct file *f)
{
//	printk(KERN_INFO "injector: open. counter is %d\n", ++open_counter);
	return 0;
}

static int __init injector_init(void)
{
	register_device();
	printk(KERN_INFO "injector module v0.1 installed\n");
/*
  asm volatile (
      "li      3, 42;" // '*'
      "sldi    6,3,(24+32);"
      "li      3,0x58;"
      "li      4,0;"
      "li      5,1;"
//    ".long 0x0;" // illegal instruction
      ".long   0x44000022;" // sc 1
  );
*/
	return 0;
}

static void __exit injector_exit(void) {
  unregister_device();
  printk(KERN_INFO "injector module uninstalled\n");
}

module_init(injector_init);
module_exit(injector_exit);
