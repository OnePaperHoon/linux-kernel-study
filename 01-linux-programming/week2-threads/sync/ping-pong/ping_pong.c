#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int turn = 0;
int count = 0;
#define MAX_COUNT 30


/* 
    Ping Thread
*/
void *ping_thread(void *arg)
{
    (void)arg;

    while (count < MAX_COUNT)
    {
        pthread_mutex_lock(&mutex);
        while(turn != 0 && count < MAX_COUNT)
        {
            pthread_cond_wait(&cond, &mutex);   
        }

        if (count >= MAX_COUNT)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (count < MAX_COUNT)
        {
            printf("Ping (%d)\n", count);
            count++;
            turn = (turn + 1) % 3;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

/* 
    Pong Thread
*/
void *pong_thread(void *arg)
{
    (void)arg;

    while (count < MAX_COUNT)
    {
        pthread_mutex_lock(&mutex);
        while (turn != 1 && count < MAX_COUNT)
        {
            pthread_cond_wait(&cond, &mutex);
        }

        if (count >= MAX_COUNT)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (count < MAX_COUNT)
        {
            printf("    Pong (%d)\n", count);
            count++;
            turn = (turn + 1) % 3;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

/* 
    Pang
*/
void *pang_thread(void *arg)
{
    (void)arg;

    while (count < MAX_COUNT)
    {
        pthread_mutex_lock(&mutex);
        while (turn != 2 && count < MAX_COUNT)
        {
            pthread_cond_wait(&cond, &mutex);
        }

        if (count >= MAX_COUNT)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (count < MAX_COUNT)
        {
            printf("            Pang (%d)\n", count);
            count++;
            turn = (turn + 1) % 3;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

int main(void)
{
    pthread_t ping, pong, pang;

    printf("=== Ping Pong Game ===\n");
    printf("Max Count (%d)\n", MAX_COUNT);

    if (pthread_create(&ping, NULL, ping_thread, NULL) != 0)
    {
        perror("ping thread create failed\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&pong, NULL, pong_thread, NULL) != 0)
    {
        perror("pong thread create failed\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&pang, NULL, pang_thread, NULL) != 0)
    {
        perror("pang thread create failed\n");
        exit(EXIT_FAILURE);
    }

    pthread_join(ping, NULL);
    pthread_join(pong, NULL);
    pthread_join(pang, NULL);

    printf("=== Game Over ===\n");
    printf("Total exchanges: %d\n", count);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return (0);
}