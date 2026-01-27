#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Thread 함수
void *thread_function(void *arg)
{
    long thread_id = (long)arg;

    printf("Hello from thread %ld!\n", thread_id);
    printf("Thread %ld: My pthread_t is %lu\n",
        thread_id, (unsigned long)pthread_self());

    sleep(10);

    printf("Thread %ld: Exiting\n", thread_id);

    return (void *)thread_id;
}

int main(void)
{
    pthread_t thread;
    void *result;

    printf("Main: Creating thread\n");

    // Thread 생성
    if (pthread_create(&thread, NULL, thread_function, (void *)42) != 0) {
        perror("pthread_create");
        return 1;
    }

    sleep(1);
    printf("Main: Thread create with ID %lu\n",
        (unsigned long)thread);

    // Thread 종료 대기
    printf("Main: Waiting for thread to finish\n");
    if (pthread_join(thread, &result) != 0)
    {
        perror("pthread_join");
        return 1;
    }

    printf("Main: Thread returned %ld\n", (long)result);
    printf("Main: Done\n");

    return 0;
}