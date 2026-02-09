/* 
 * Thread Pool Implementation in C
 * 
 */

#include "thread_pool.h"



// Task Cre/Des
static task_t *create_task(thread_func_t function, void *arg)
{
    task_t *new_task = (task_t *)malloc(sizeof(task_t));
    if (new_task == NULL)
    {
        perror("Failed to allocate memory for new task");
        return NULL;
    }

    new_task->function = function;
    new_task->arg = arg;
    new_task->next = NULL;

    return new_task;
}

static void destroy_task(task_t *task)
{
    if (task == NULL)
        return;
    free(task);
}

// Thread Pool Worker
static void *thread_pool_worker(void *arg)
{
    // 워커는 결국 무한 루프를 돌면서 테스크 큐에서 테스크를 꺼내어 실행

    
}



// Thread Pool Cre/Des

thread_pool_t *thread_pool_create(size_t pool_size)
{

    thread_pool_t   *pool;
    pthread_t       thread;
    size_t          i;

    if (pool_size == 0 || pool_size > POOL_SIZE)
        pool_size = POOL_SIZE;

    pool = calloc(1, sizeof(*pool));
    if (pool == NULL)
    {
        perror("Failed to allocate memory for thread pool");
        return NULL;
    }

    pool->thread_count = pool_size;

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);

    pool->queue_head = NULL;
    pool->queue_tail = NULL;

    for (i = 0; i < pool_size; i++)
    {
        if (pthread_create(&thread, NULL, thread_pool_worker, (void *)pool) != 0)
        {
            perror("Failed to create thread");
            thread_pool_destroy(pool);
            return NULL;
        }
        pool->threads[i] = thread;
    }

    return pool;
}

void thread_pool_destroy(thread_pool_t *pool)
{
    // 쓰레드 풀의 자원을 해제 및 쓰레드 종료

    pool->shutdown = true;
    // 쓰레드를 깨워야함
    pthread_cond_broadcast(&pool->not_empty);



}

