/*
 * uart_stm32.c - UART driver for STM32 communication
 * Week 6 Day 1: UART Communication
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/serial_core.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "stm32_uart"
#define UART_DEVICE "/dev/ttyAMAO"
#define BUFFER_SIZE 1024

static int major_number;
static char tx_buffer[BUFFER_SIZE];
static char rx_buffer[BUFFER_SIZE];
static size_t rx_count = 0;

static struct file *uart_file = NULL;

/*
 * UART 열기
 */
static int uart_open_port(void) {
  uart_file = filp_open(UART_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);

  if (IS_ERR(uart_file)) {
    printk(KERN_ERR "uart_stm32: Failed to open %s\n", UART_DEVICE);
    return PTR_ERR(uart_file);
  }

  printk(KERN_INFO "uart_stm32: UART port opened\n");
  return 0;
}

/*
 * UART 닫기
 */
static void uart_close_port(void) {
  if (uart_file && !IS_ERR(uart_file)) {
    filp_close(uart_file, NULL);
    uart_file = NULL;
  }
}

/*
 * UART로 데이터 전송
 */
static ssize_t uart_send(const char *data, size_t len) {
  ssize_t ret;

  if (!uart_file || IS_ERR(uart_file))
    return -ENODEV;

  ret = kernel_write(uart_file, data, len, &uart_file->f_pos);

  return ret;
}

/*
 * UART에서 데이터 수신
 */
static ssize_t uart_receive(char *data, size_t len) {
  ssize_t ret;

  if (!uart_file || IS_ERR(uart_file))
    return -ENODEV;

  ret = kernel_read(uart_file, data, len, &uart_file->f_pos);

  return ret;
}

/*
 * 디바이스 열기
 */
static int device_open(struct inode *inode, struct file *file) {
  printk(KERN_INFO "uart_stm32: Device opened\n");

  try_module_get(THIS_MODULE);
  return 0;
}

/*
 * 디바이스 닫기
 */
static int device_release(struct inode *inode, struct file *file) {
  printk(KERN_INFO "uart_stm32: Device closed\n");

  module_put(THIS_MODULE);
  return 0;
}

/*
 * 디바이스 읽기
 */
static ssize_t device_read(struct file *file, char __user *buffer, size_t len,
                           loff_t *offset) {
  ssize_t bytes_read;
  ssize_t ret;

  // UART 에서 읽기
  bytes_read = uart_receive(rx_buffer, min(len, (size_t)BUFFER_SIZE));

  if (bytes_read < 0) {
    printk(KERN_ERR "uart_stm32: UART read failed\n");
    return bytes_read;
  }

  if (bytes_read == 0)
    return 0;

  // 유저 공간으로 복사
  ret = copy_to_user(buffer, rx_buffer, bytes_read);
  if (ret) {
    return -EFAULT;
  }

  printk(KERN_INFO "uart_stm32: Read %zd bytes\n", bytes_read);

  return bytes_read;
}

/*
 * 디바이스 쓰기
 */
static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t len, loff_t *offset) {
  ssize_t to_write = min(len, (size_t)BUFFER_SIZE);
  ssize_t ret;

  // 유저 공간에서 복사
  if (copy_from_user(tx_buffer, buffer, to_write)) {
    return -EFAULT;
  }

  // UART로 전송
  ret = uart_send(tx_buffer, to_write);

  if (ret < 0) {
    printk(KERN_ERR "uart_stm32: UART Write failed\n");
    return ret;
  }

  printk(KERN_INFO "uart_stm32: Wrote %zd bytes\n", ret);

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
};

/*
 * 모듈 초기화
 */
static int __init uart_stm32_init(void) {
  int ret;

  printk(KERN_INFO "====================================\n");
  printk(KERN_INFO "uart_stm32: Initializing\n");

  // Character device 등록
  major_number = register_chrdev(0, DEVICE_NAME, &fops);
  if (major_number < 0) {
    printk(KERN_ERR "uart_stm32: Failed to register\n");
    return major_number;
  }

  printk(KERN_INFO "uart_stm32: Registered with major %d\n", major_number);

  // UART 포트 열기
  ret = uart_open_port();
  if (ret < 0) {
    unregister_chrdev(major_number, DEVICE_NAME);
    return ret;
  }

  printk(KERN_INFO "uart_stm32: Create Device:\n");
  printk(KERN_INFO " sudo mknod /dev/%s c %d 0\n", DEVICE_NAME, major_number);
  printk(KERN_INFO " sudo chmod 666 /dev/%s\n", DEVICE_NAME);
  printk(KERN_INFO "====================================\n");

  return 0;
}

/*
 * 모듈 종료
 */
static void __exit uart_stm32_exit(void) {
  uart_close_port();
  unregister_chrdev(major_number, DEVICE_NAME);

  printk(KERN_INFO "uart_stm32: Unregistered\n");
}

module_init(uart_stm32_init);
module_exit(uart_stm32_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OnepaperHoon");
MODULE_DESCRIPTION("UART Drive for STM32 Communication - Week 6 Day 1");
MODULE_VERSION("1.0");
