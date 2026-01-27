#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_THREADS 10
#define INCREMENTS 100000

long global_counter = 0;

void *increment_counter(void *arg)
{
    long tid = (long)arg;
    int i;

    printf("Thread %ld: Staring\n", tid);


    // 100,000번 증가
    for (i = 0; i < INCREMENTS; i++)
    {
        global_counter++; // <= Race Condition
    }

    printf("Thread %ld: Finished\n", tid);

    return NULL;
}

int main(void)
{
    pthread_t threads[MAX_THREADS];
    int i;

    printf("Initial counter: %ld\n", global_counter);
    printf("Expected final counter: %d\n",MAX_THREADS * INCREMENTS);
    printf("\n");

    // Create threads
    for (i = 0; i < MAX_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, increment_counter, (void*)(long)i);
    }


    // Waiting Threads
    for (i = 0; i < MAX_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("\n");
    printf("Final counter: %ld\n", global_counter);
    printf("Expected counter: %d\n",MAX_THREADS * INCREMENTS);
    printf("Difference: %ld\n", (MAX_THREADS * INCREMENTS) - global_counter);

    if (global_counter == MAX_THREADS * INCREMENTS)
        printf("✅ Correct\n");
    else
        printf("❌ Race condition detected\n");

    return 0;
}