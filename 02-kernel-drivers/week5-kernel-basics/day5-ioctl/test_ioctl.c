/*
 * test_ioctl.c - ioctl test program
 */

#include "ioctl_cmd.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/ioctl_dev"

int main(int argc, char *argv[]) {
  int fd, ret, count, value;
  char message[256];

  /* 디바이스 열기 */
  fd = open(DEVICE_PATH, O_RDWR);
  if (fd < 0) {
    perror("Failed to open device");
    return 1;
  }

  printf("====================================\n");
  printf("ioctl Test Program\n");
  printf("====================================\n\n");

  /* 1. IOCTL_GET_COUNT */
  printf("1. Getting count...\n");
  ret = ioctl(fd, IOCTL_GET_COUNT, &count);
  if (ret < 0) {
    perror("IOCTL_GET_COUNT failed");
  } else {
    printf("   Current count: %d\n\n", count);
  }

  /* 2. IOCTL_SET_MSG */
  printf("2. Setting message...\n");
  strcpy(message, "Hello from OnePaperHoon!");
  ret = ioctl(fd, IOCTL_SET_MSG, message);
  if (ret < 0) {
    perror("IOCTL_SET_MSG failed");
  } else {
    printf("   Message set: %s\n\n", message);
  }

  /* 3. IOCTL_GET_MSG */
  printf("3. Getting message...\n");
  memset(message, 0, sizeof(message));
  ret = ioctl(fd, IOCTL_GET_MSG, message);
  if (ret < 0) {
    perror("IOCTL_GET_MSG failed");
  } else {
    printf("   Message: %s\n\n", message);
  }

  /* 4. IOCTL_EXCHANGE */
  printf("4. Exchanging value...\n");
  value = 999;
  printf("   Sending: %d\n", value);
  ret = ioctl(fd, IOCTL_EXCHANGE, &value);
  if (ret < 0) {
    perror("IOCTL_EXCHANGE failed");
  } else {
    printf("   Received: %d\n\n", value);
  }

  /* 5. IOCTL_GET_COUNT (확인) */
  printf("5. Getting count (after exchange)...\n");
  ret = ioctl(fd, IOCTL_GET_COUNT, &count);
  if (ret < 0) {
    perror("IOCTL_GET_COUNT failed");
  } else {
    printf("   Current count: %d\n\n", count);
  }

  /* 6. IOCTL_RESET */
  printf("6. Resetting...\n");
  ret = ioctl(fd, IOCTL_RESET, NULL);
  if (ret < 0) {
    perror("IOCTL_RESET failed");
  } else {
    printf("   Reset done!\n\n");
  }

  /* 7. IOCTL_GET_COUNT (확인) */
  printf("7. Getting count (after reset)...\n");
  ret = ioctl(fd, IOCTL_GET_COUNT, &count);
  if (ret < 0) {
    perror("IOCTL_GET_COUNT failed");
  } else {
    printf("   Current count: %d\n\n", count);
  }

  printf("====================================\n");
  printf("Test Complete!\n");
  printf("====================================\n");

  close(fd);
  return 0;
}
