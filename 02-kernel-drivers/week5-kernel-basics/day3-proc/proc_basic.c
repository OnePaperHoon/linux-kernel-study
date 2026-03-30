/*
 * proc_basic.c - /proc filesystem basic example
 * Week 5 Day 3: proc filesystem
 */

#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#define PROC_NAME "onepaperhoon_info"

/* 전역 카운터 */
static unsigned long access_count = 0;

/*
 * seq_show - /proc 파일을 읽을 때 호출
 */
static int proc_show(struct seq_file *m, void *v) {
  access_count++;

  seq_printf(m, "====================================\n");
  seq_printf(m, "OnePaperHoon Kernel Module Info\n");
  seq_printf(m, "====================================\n");
  seq_printf(m, "Access count: %lu\n", access_count);
  seq_printf(m, "Current jiffies: %lu\n", jiffies);
  seq_printf(m, "Module: %s\n", THIS_MODULE->name);
  seq_printf(m, "====================================\n");

  return 0;
}

/*
 * proc_open - /proc 파일을 열 때 호출
 */
static int proc_open(struct inode *inode, struct file *file) {
  return single_open(file, proc_show, NULL);
}

/*
 * proc_write - /proc 파일에 쓸 때 호출
 */
static ssize_t proc_write(struct file *file, const char __user *buffer,
                          size_t count, loff_t *pos) {
  char cmd[10];

  if (count > sizeof(cmd) - 1)
    return -EINVAL;

  if (copy_from_user(cmd, buffer, count))
    return -EFAULT;

  cmd[count] = '\0';

  /* "reset" 명령어로 카운터 리셋 */
  if (strncmp(cmd, "reset", 5) == 0) {
    access_count = 0;
    printk(KERN_INFO "proc_basic: Counter reset!\n");
  }

  return count;
}

/*
 * proc_ops 구조체 (Kernel 5.6+)
 */
static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_write = proc_write,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* proc entry pointer */
static struct proc_dir_entry *proc_entry;

/*
 * 모듈 초기화
 */
static int __init proc_basic_init(void) {
  printk(KERN_INFO "====================================\n");
  printk(KERN_INFO "proc_basic: Module loading...\n");

  /* /proc 파일 생성 */
  proc_entry = proc_create_data(PROC_NAME, 0666, NULL, &proc_fops, NULL);
  if (!proc_entry) {
    printk(KERN_ERR "proc_basic: Failed to create /proc/%s\n", PROC_NAME);
    return -ENOMEM;
  }

  printk(KERN_INFO "proc_basic: Created /proc/%s\n", PROC_NAME);
  printk(KERN_INFO "proc_basic: Read with: cat /proc/%s\n", PROC_NAME);
  printk(KERN_INFO "proc_basic: Reset with: echo reset > /proc/%s\n",
         PROC_NAME);
  printk(KERN_INFO "====================================\n");

  return 0;
}

/*
 * 모듈 종료
 */
static void __exit proc_basic_exit(void) {
  if (proc_entry) {
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "proc_basic: Removed /proc/%s\n", PROC_NAME);
  }

  printk(KERN_INFO "proc_basic: Module unloaded. Total accesses: %lu\n",
         access_count);
}

module_init(proc_basic_init);
module_exit(proc_basic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OnePaperHoon");
MODULE_DESCRIPTION("Basic /proc filesystem example - Week 5 Day 3");
MODULE_VERSION("1.0");
