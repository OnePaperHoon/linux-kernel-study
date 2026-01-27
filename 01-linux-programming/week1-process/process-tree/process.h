#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct process {
    pid_t pid;                  // 프로세스 아이디
    pid_t ppid;                 // 부모 프로세스 아이디
    char state;                 // 상태 값
    char comm[256];             // 명령어 이름
    struct process *parent;     // 부모 프로세스
    struct process **children;  // 자식 프로세스 배열
    int num_children;
    int children_capacity;
} process_t;

// 프로세스 생성 및 삭제
process_t       *process_create(pid_t pid);
void            process_free(process_t *proc);

// 프로세스 정보 읽기
int             process_read_stat(process_t *proc);

// 자식 추가
void            process_add_child(process_t *parent, process_t *child);

#endif
