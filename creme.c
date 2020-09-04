#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michio Honda");
MODULE_DESCRIPTION("Connection removal notification module.");
MODULE_VERSION("0.01");

static int__init
creme_init(void)
{
	 printk(KERN_INFO "loaded creme\n");
	 return 0;
}

static void__exit
creme_exit(void)
{
	 printk(KERN_INFO "unloaded creme\n");
}

module_init(creme_init);
module_exit(creme_exit);
