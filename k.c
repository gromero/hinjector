#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gustavo Romero");
MODULE_DESCRIPTION("Trepanner");
MODULE_VERSION("v0.1");

static int __init trepanner_init(void) {
  printk("KERN_INFO Trepanner module v0.1 init()\n");
  asm volatile (
      "li      3, 42;" /* '*' */
      "sldi    6,3,(24+32);"
      "li      3,0x58;"
      "li      4,0;"
      "li      5,1;"
//    ".long 0x0;"
      ".long   0x44000022;"
   );
  return 0;
}

static void __exit trepanner_exit(void) {
  printk("Trepanner module exit()\n");
}

module_init(trepanner_init);
module_exit(trepanner_exit);

