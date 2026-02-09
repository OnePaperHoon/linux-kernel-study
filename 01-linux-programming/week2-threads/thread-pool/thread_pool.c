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
    thread_pool_t *pool = (thread_pool_t *)arg;
    task_t *task;

    while (1)
    {
        pthread_mutex_lock(&pool->mutex);

        // 큐가 비어있고 종료 신호가 없으면 대기
        while (pool->queue_head == NULL && !pool->shutdown)
            pthread_cond_wait(&pool->not_empty, &pool->mutex);

        // 종료 신호가 오고 큐가 비어있으면 종료
        if (pool->shutdown && pool->queue_head == NULL)
        {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        // 큐에서 테스크를 꺼냄 (dequeue)
        task = pool->queue_head;
        pool->queue_head = task->next;
        if (pool->queue_head == NULL)
            pool->queue_tail = NULL;
        pool->queue_size--;
        pool->active_tasks++;

        // 큐에 빈 공간이 생겼으므로 add_task에서 대기 중인 스레드를 깨움
        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->mutex);

        // 테스크 실행 (lock 밖에서 실행하여 병렬성 확보)
        task->function(task->arg);
        destroy_task(task);

        // 실행 완료 처리
        pthread_mutex_lock(&pool->mutex);
        pool->active_tasks--;
        // 큐가 비고 실행 중인 테스크도 없으면 wait 중인 스레드를 깨움
        if (pool->queue_head == NULL && pool->active_tasks == 0)
            pthread_cond_signal(&pool->idle);
        pthread_mutex_unlock(&pool->mutex);
    }

    return NULL;
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
    pthread_cond_init(&pool->idle, NULL);

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
    task_t *task;
    size_t i;

    if (pool == NULL)
        return;

    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = true;
    // 모든 워커 쓰레드를 깨워야함
    pthread_cond_broadcast(&pool->not_empty);
    pthread_mutex_unlock(&pool->mutex);

    // 모든 워커 쓰레드 종료 대기
    for (i = 0; i < pool->thread_count; i++)
        pthread_join(pool->threads[i], NULL);

    // 남은 테스크 큐 정리
    while (pool->queue_head != NULL)
    {
        task = pool->queue_head;
        pool->queue_head = task->next;
        destroy_task(task);
    }

    // 동기화 자원 해제
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
    pthread_cond_destroy(&pool->idle);

    free(pool);
}

// Thread Pool Task Management

int thread_pool_add_task(thread_pool_t *pool, thread_func_t function, void *arg)
{
    task_t *new_task;

    if (pool == NULL || function == NULL)
        return -1;

    new_task = create_task(function, arg);
    if (new_task == NULL)
        return -1;

    pthread_mutex_lock(&pool->mutex);

    // 큐가 가득 찼으면 빈 공간이 생길 때까지 대기
    while (pool->queue_size >= TASK_QUEUE_SIZE && !pool->shutdown)
        pthread_cond_wait(&pool->not_full, &pool->mutex);

    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutex);
        destroy_task(new_task);
        return -1;
    }

    // 큐에 테스크 추가 (enqueue)
    if (pool->queue_tail == NULL)
    {
        // 큐가 비어있는 경우
        pool->queue_head = new_task;
        pool->queue_tail = new_task;
    }
    else
    {
        pool->queue_tail->next = new_task;
        pool->queue_tail = new_task;
    }
    pool->queue_size++;

    // 워커 스레드를 깨움
    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->mutex);

    return 0;
}

void thread_pool_wait(thread_pool_t *pool)
{
    if (pool == NULL)
        return;

    pthread_mutex_lock(&pool->mutex);
    // 큐에 테스크가 남아있거나 실행 중인 테스크가 있으면 대기
    while (pool->queue_head != NULL || pool->active_tasks > 0)
        pthread_cond_wait(&pool->idle, &pool->mutex);
    pthread_mutex_unlock(&pool->mutex);
}
