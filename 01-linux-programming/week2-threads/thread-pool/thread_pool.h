#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <error.h>
#include <errno.h>
#include <stdbool.h>


#define POOL_SIZE 4 // 스레드 풀의 크기 정의
#define MAX_TASKS 100 // 최대 테스크 수 정의
#define TASK_QUEUE_SIZE 100 // 테스크 큐의 크기 정의

typedef void (*thread_func_t)(void *arg); // 스레드 함수 타입 정의



/**
 * Task structure
 */
typedef struct task {
    // 테스크에 대한 정보를 담는 구조체
    thread_func_t function; // 테스크가 수행할 함수 포인터
    void *arg;                   // 함수에 전달할 인자 포인터
    struct task *next;           // 다음 테스크를 가리키는 포인터
} task_t;

/**
 * Thread Pool structure
 */
typedef struct {
    pthread_t threads[POOL_SIZE];   // 스레드 풀을 구성하는 스레드 배열
    task_t *queue_head;             // Task Queue
    task_t *queue_tail;             // Task Tail
    pthread_mutex_t mutex;          // 동기화
    pthread_cond_t not_empty;       // Queue에 Task가 있을 때 신호
    pthread_cond_t not_full;        // Queue에 빈 공간이 있을 때 신호 
    bool shutdown;                  // 종료 플래그    
    size_t thread_count;            // 스레드 수
}   thread_pool_t;

/**
 * Thread Pool Cre/Des
 *
*/
thread_pool_t *thread_pool_create(size_t pool_size);
void thread_pool_destroy(thread_pool_t *pool);

/**
 * Thread Pool Task Management
 *
*/
int thread_pool_add_task(thread_pool_t *pool, thread_func_t function, void *arg);
void thread_pool_wait(thread_pool_t *pool);




#endif // THREAD_POOL_H