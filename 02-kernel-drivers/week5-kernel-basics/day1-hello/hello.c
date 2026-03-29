#include "linux/printk.h"
#include <linux/init.h>   // __init, __exit
#include <linux/kernel.h> // printk
#include <linux/module.h> // module_init, module_exit

/*
        모듈 초기화 함수
        - insmod 실행 시 자동 호출
        - 성공 0반환
        - 실패 음수 반환
*/
static int __init hello_init(void) {
  printk(KERN_INFO "===================================\n");
  printk(KERN_INFO "Hello, Kernel Module!\n");
  printk(KERN_INFO "Week 5 Day 1: Module Basics\n");
  printk(KERN_INFO "Author: OnePaperHoon\n");
  printk(KERN_INFO "===================================\n");
  return 0;
}

/*
        모듈 종료 함수
        -rmmod 실행 시 자동 호출
        -리소스 정리
*/
static void __exit hello_exit(void) {
  printk(KERN_INFO "====================================\n");
  printk(KERN_INFO "Goodbye, Kernel Module!\n");
  printk(KERN_INFO "Module unloaded successfully.\n");
  printk(KERN_INFO "====================================\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OnePaperHoon");
MODULE_DESCRIPTION("Simple Hello World Kernel Module - Week 5 Day 1");
MODULE_VERSION("1.0");
