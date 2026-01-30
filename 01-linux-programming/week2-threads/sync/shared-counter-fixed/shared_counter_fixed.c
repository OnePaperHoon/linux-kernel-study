/* 
    Day3의 Race Condition을 Mutex로 해결
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_THREADS 10
#define INCREMENTS 100000

/* 공유 리소스 */
long global_counter = 0;

/* Mutex: Critical Section 보호 */
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 
 * Thread 함수 : Counter 증가
 */

void *increment_counter(void *arg)
{
    long tid = (long)arg;

    printf("Thread %ld: Starting\n", tid);

    for (int i = 0; i < INCREMENTS; i++)
    {
        /* === Critical Section Start === */
        pthread_mutex_lock(&counter_mutex);
        global_counter++;
        pthread_mutex_unlock(&counter_mutex);
        /* === Critical Section End === */
    }

    printf("Thread %ld: Finished\n", tid);

    return NULL;
}

int main(void)
{
    pthread_t threads[MAX_THREADS];
    long expected = MAX_THREADS * INCREMENTS;
    
    printf("=== Shared Counter with Mutex ===\n\n");
    printf("Initial counter: %ld\n", global_counter);
    printf("Expected final:  %ld\n", expected);
    printf("Threads:         %d\n", MAX_THREADS);
    printf("Increments each: %d\n", INCREMENTS);
    printf("\n");
    
    /* Thread 생성 */
    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, increment_counter, 
                          (void *)(long)i) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    /* Thread 대기 */
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* 결과 출력 */
    printf("\n");
    printf("=== Results ===\n");
    printf("Final counter:   %ld\n", global_counter);
    printf("Expected:        %ld\n", expected);
    printf("Difference:      %ld\n", expected - global_counter);
    printf("\n");
    
    /* 검증 */
    if (global_counter == expected) {
        printf("✅ Success! No race condition!\n");
        printf("   Mutex protected the critical section.\n");
    } else {
        printf("❌ Still has issues (difference: %ld)\n", 
               expected - global_counter);
        printf("   This should not happen with proper mutex!\n");
    }
    
    /* Mutex 정리 */
    pthread_mutex_destroy(&counter_mutex);
    
    return 0;
}