/*
 *
 * params.c - Kernel Module with Parameters
 * Week 5 Day 2: Module Parameters
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

/*
 * 모듈 파라미터 선언
 */
static int count = 1;
static char *name = "OnepaperHoon";
static bool debug = false;

/*
 * module_param(변수명, 타입, 권한)
 * 권한:
 *   0000 - /sys에 나타나지 않음
 *   0444 - 읽기 전용
 *   0644 - 읽기 / 쓰기
 */

module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of greetings (default=1)");

module_param(name, charp, 0644);
MODULE_PARM_DESC(name, "Name to greet (default=OnePaperGoon)");

module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Enable debug mode (default=false)");

/*
 * 모듈 초기화
 */
static int __init params_init(void) {
  int i;

  printk(KERN_INFO "=======================================\n");
  printk(KERN_INFO "Module Parameters Demo\n");
  printk(KERN_INFO "=======================================\n");

  if (debug) {
    printk(KERN_DEBUG "[DEBUG] Module loaded with:\n");
    printk(KERN_DEBUG " count = %d\n", count);
    printk(KERN_DEBUG " name = %s\n", name);
    printk(KERN_DEBUG " debug = %s\n", debug ? "ture" : "false");
  }

  // count만큼 인사 출력
  for (i = 0; i < count; i++) {
    printk(KERN_INFO "Hello, %s", name);
  }
  printk("====================================\n");
  return 0;
}

static void __exit params_exit(void) {
  printk(KERN_INFO "======================================\n");
  printk(KERN_INFO "GoodBye, %s!\n", name);
  printk(KERN_INFO "Module unloaded\n");
  printk(KERN_INFO "======================================\n");
}

module_init(params_init);
module_exit(params_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OnePaperHoon");
MODULE_DESCRIPTION("Kernel Module with Parameters - Week 5 Day 2");
MODULE_VERSION("1.0");
