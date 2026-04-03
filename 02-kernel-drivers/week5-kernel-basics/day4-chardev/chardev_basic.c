#include "linux/gfp_types.h"
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "onepaperhoon_dev"
#define BUFFER_SIZE 1024

/* 디바이스 메이저 번호 */
static int major_number;

// 디바이서 버퍼
static char *device_buffer;
static size_t buffer_size = 0;

// 오픈 카운터
static int device_open_count = 0;

/*
 * device_open - 디바이스 파일을 열 때 호출
 */
static int device_open(struct inode *inode, struct file *file) {
  device_open_count++;
  printk(KERN_INFO "chardev: Device opened %d time(s)\n", device_open_count);

  try_module_get(THIS_MODULE);
  return 0;
}

/*
 * device_release - 디바이스 파일을 닫을 때 호출
 */
static int device_release(struct inode *inode, struct file *file) {
  printk(KERN_INFO "chardev: Device closed\n");

  module_put(THIS_MODULE);
  return 0;
}

/*
 * device_read - 디바이스에서 읽기
 */
static ssize_t device_read(struct file *file, char __user *user_buffer,
                           size_t count, loff_t *offset) {
  size_t bytes_to_read;

  // 읽을 데이터가 없으면
  if (*offset >= buffer_size) {
    return 0; // EOF
  }

  // 읽을 바이트 수 계산
  bytes_to_read = min(count, buffer_size - (size_t)*offset);

  // 커널 버퍼 -> 유저 버퍼
  if (copy_to_user(user_buffer, device_buffer + *offset, bytes_to_read)) {
    return -EFAULT;
  }

  *offset += bytes_to_read;

  printk(KERN_INFO "chardev: Read %zu bytes\n", bytes_to_read);
  return bytes_to_read;
}

/*
 * device_write - 디바이스에 쓰기
 */
static ssize_t device_write(struct file *file, const char __user *user_buffer,
                            size_t count, loff_t *offset) {
  size_t bytes_to_write;

  // 버퍼 오버플로우 방지
  if (*offset >= BUFFER_SIZE) {
    return -ENOSPC;
  }

  // 쓸 바이트 수 계산
  bytes_to_write = min(count, BUFFER_SIZE - (size_t)*offset);

  // 유저 버퍼 -> 커널 버퍼
  if (copy_to_user(device_buffer + *offset, user_buffer, bytes_to_write)) {
    return -EFAULT;
  }

  *offset += bytes_to_write;
  buffer_size = max(buffer_size, (size_t)*offset);

  printk(KERN_INFO "chardev: Wrote %zu bytes\n", bytes_to_write);
  return bytes_to_write;
}

/*
 * file_operations 구조체
 */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

/*
 * 모듈 초기화
 */
static int __init chardev_init(void) {
  printk(KERN_INFO "=========================================\n");
  printk(KERN_INFO "chardev: Initializing character device\n");

  // 디바이스 버퍼 할당
  device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
  if (!device_buffer) {
    printk(KERN_ERR "chardev: Failed to allocate buffer\n");
    return -ENOMEM;
  }
  memset(device_buffer, 0, BUFFER_SIZE);

  // 문자 디바이스 등록
  major_number = register_chrdev(0, DEVICE_NAME, &fops);
  if (major_number < 0) {
    printk(KERN_ERR "chardev: Failed to register device\n");
    kfree(device_buffer);
    return major_number;
  }

  printk(KERN_INFO "chardev: Register with major number %d\n", major_number);
  printk(KERN_INFO "chardev: Create deivce with:\n");
  printk(KERN_INFO " sudo mknod /dev/%s c %d 0\n", DEVICE_NAME, major_number);
  printk(KERN_INFO " sudo chmode 666 /dev/%s\n", DEVICE_NAME);
  printk(KERN_INFO "========================================\n");

  return 0;
}

/*
 * 모듈 종료
 */
static void __exit chardev_exit(void) {
  // 문자 디바이스 헤제
  unregister_chrdev(major_number, DEVICE_NAME);

  // 버퍼 해제
  kfree(device_buffer);

  printk(KERN_INFO "chardev: Device unregistered\n");
  printk(KERN_INFO "chardev: Remove device with:\n");
  printk(KERN_INFO " sudo rm /dev/%s\n", DEVICE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OnepaperHoon");
MODULE_DESCRIPTION("Basic character Device Driver - Week 5 Day 4");
MODULE_VERSION("1.0");
