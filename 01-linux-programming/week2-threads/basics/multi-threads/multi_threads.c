#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 10

void *thread_function(void *arg)
{
    long tid = (long)arg;

    printf("Thread %ld: Starting\n", tid);

    // 각 thread가 다른 시간 동안 작업
    sleep(NUM_THREADS - tid);

    printf("Thread %ld: Fnished after %ld seconds\n", tid, tid);

    return (void*)(tid * 100); // 반환값
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    void *result;
    int i;

    printf("Main: Creating %d threads\n", NUM_THREADS);

    // Thread 생성
    for (i = 0; i < NUM_THREADS; i++)
    {
        printf("Main: Creating thread %d\n", i);

        if (pthread_create(&threads[i], NULL, thread_function, (void *)(long)i) != 0)
        {
            perror("pthread_create");
            return 1;
        }
    }

    printf("Main: All thread created\n");
    printf("Main: Waiting for threads to finish...\n");

    // Thread 종료 대기 (순서대로)
    for (i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_join(threads[i], &result) != 0)
        {
            perror("pthread_join");
            return 1;
        }
        printf("Main: Thread %d returned %ld\n", i, (long)result);
    }

    printf("Main: All Threads finished\n");
    return 0;
}