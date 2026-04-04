/*
 * ioctl_cmd.h - ioctl command definitions
 * Week 5 Day 5: ioctl
 */

#ifndef IOCTL_CMD_H
#define IOCTL_CMD_H

#include <linux/ioctl.h>

/* Magic number */
#define IOCTL_MAGIC 'k'

/* ioctl 명령어 정의 */
#define IOCTL_RESET _IO(IOCTL_MAGIC, 0)               // 데이터 없음
#define IOCTL_GET_COUNT _IOR(IOCTL_MAGIC, 1, int)     // 읽기
#define IOCTL_SET_MSG _IOW(IOCTL_MAGIC, 2, char[256]) // 쓰기
#define IOCTL_GET_MSG _IOR(IOCTL_MAGIC, 3, char[256]) // 읽기
#define IOCTL_EXCHANGE _IOWR(IOCTL_MAGIC, 4, int)     // 읽기+쓰기

#define IOCTL_MAXNR 4

#endif /* IOCTL_CMD_H */
