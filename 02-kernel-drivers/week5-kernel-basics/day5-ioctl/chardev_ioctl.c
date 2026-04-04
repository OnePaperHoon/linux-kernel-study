/*
 * chardev_ioctl.c - Character Device with ioctl
 * Week 5 Day 5: ioctl
 */

#include "ioctl_cmd.h"
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "ioctl_dev"
#define MSG_SIZE 256

/* 디바이스 상태 */
static int major_number;
static int access_count = 0;
static char message[MSG_SIZE] = "Hello from kernel!";

/*
 * device_open - 디바이스 열기
 */
static int device_open(struct inode *inode, struct file *file) {
  access_count++;
  printk(KERN_INFO "ioctl_dev: Device opened (count: %d)\n", access_count);

  try_module_get(THIS_MODULE);
  return 0;
}

/*
 * device_release - 디바이스 닫기
 */
static int device_release(struct inode *inode, struct file *file) {
  printk(KERN_INFO "ioctl_dev: Device closed\n");

  module_put(THIS_MODULE);
  return 0;
}

/*
 * device_read - 읽기
 */
static ssize_t device_read(struct file *file, char __user *buffer, size_t len,
                           loff_t *offset) {
  size_t msg_len = strlen(message);
  size_t to_read;

  if (*offset >= msg_len)
    return 0;

  to_read = min(len, msg_len - (size_t)*offset);

  if (copy_to_user(buffer, message + *offset, to_read))
    return -EFAULT;

  *offset += to_read;
  printk(KERN_INFO "ioctl_dev: Read %zu bytes\n", to_read);

  return to_read;
}

/*
 * device_write - 쓰기
 */
static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t len, loff_t *offset) {
  size_t to_write = min(len, (size_t)(MSG_SIZE - 1));

  if (copy_from_user(message, buffer, to_write))
    return -EFAULT;

  message[to_write] = '\0';
  printk(KERN_INFO "ioctl_dev: Wrote %zu bytes: %s\n", to_write, message);

  return to_write;
}

/*
 * device_ioctl - ioctl 핸들러
 */
static long device_ioctl(struct file *file, unsigned int cmd,
                         unsigned long arg) {
  int ret = 0;
  int value;
  char user_msg[MSG_SIZE];

  /* Magic number 확인 */
  if (_IOC_TYPE(cmd) != IOCTL_MAGIC) {
    printk(KERN_WARNING "ioctl_dev: Invalid magic number\n");
    return -ENOTTY;
  }

  /* 명령어 번호 확인 */
  if (_IOC_NR(cmd) > IOCTL_MAXNR) {
    printk(KERN_WARNING "ioctl_dev: Invalid command number\n");
    return -ENOTTY;
  }

  switch (cmd) {
  case IOCTL_RESET:
    /* 카운터 리셋 */
    access_count = 0;
    strcpy(message, "Reset!");
    printk(KERN_INFO "ioctl_dev: RESET executed\n");
    break;

  case IOCTL_GET_COUNT:
    /* 카운터 읽기 */
    if (copy_to_user((int __user *)arg, &access_count, sizeof(int))) {
      return -EFAULT;
    }
    printk(KERN_INFO "ioctl_dev: GET_COUNT = %d\n", access_count);
    break;

  case IOCTL_SET_MSG:
    /* 메시지 설정 */
    if (copy_from_user(user_msg, (char __user *)arg, MSG_SIZE)) {
      return -EFAULT;
    }
    user_msg[MSG_SIZE - 1] = '\0';
    strcpy(message, user_msg);
    printk(KERN_INFO "ioctl_dev: SET_MSG = %s\n", message);
    break;

  case IOCTL_GET_MSG:
    /* 메시지 읽기 */
    if (copy_to_user((char __user *)arg, message, MSG_SIZE)) {
      return -EFAULT;
    }
    printk(KERN_INFO "ioctl_dev: GET_MSG\n");
    break;

  case IOCTL_EXCHANGE:
    /* 값 교환 (읽기+쓰기) */
    if (copy_from_user(&value, (int __user *)arg, sizeof(int))) {
      return -EFAULT;
    }
    printk(KERN_INFO "ioctl_dev: EXCHANGE: received %d, returning %d\n", value,
           access_count);

    /* 기존 카운터 반환 */
    if (copy_to_user((int __user *)arg, &access_count, sizeof(int))) {
      return -EFAULT;
    }

    /* 새 값으로 설정 */
    access_count = value;
    break;

  default:
    printk(KERN_WARNING "ioctl_dev: Unknown command: 0x%x\n", cmd);
    return -ENOTTY;
  }

  return ret;
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
    .unlocked_ioctl = device_ioctl,
};

/*
 * 모듈 초기화
 */
static int __init ioctl_dev_init(void) {
  printk(KERN_INFO "====================================\n");
  printk(KERN_INFO "ioctl_dev: Initializing\n");

  /* 문자 디바이스 등록 */
  major_number = register_chrdev(0, DEVICE_NAME, &fops);
  if (major_number < 0) {
    printk(KERN_ERR "ioctl_dev: Failed to register\n");
    return major_number;
  }

  printk(KERN_INFO "ioctl_dev: Registered with major %d\n", major_number);
  printk(KERN_INFO "ioctl_dev: Create device:\n");
  printk(KERN_INFO "  sudo mknod /dev/%s c %d 0\n", DEVICE_NAME, major_number);
  printk(KERN_INFO "  sudo chmod 666 /dev/%s\n", DEVICE_NAME);
  printk(KERN_INFO "====================================\n");

  return 0;
}

/*
 * 모듈 종료
 */
static void __exit ioctl_dev_exit(void) {
  unregister_chrdev(major_number, DEVICE_NAME);
  printk(KERN_INFO "ioctl_dev: Unregistered\n");
}

module_init(ioctl_dev_init);
module_exit(ioctl_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OnePaperHoon");
MODULE_DESCRIPTION("Character Device with ioctl - Week 5 Day 5");
MODULE_VERSION("1.0");
