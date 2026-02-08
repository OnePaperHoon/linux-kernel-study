#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define PRODUCERS 3
#define CONSUMERS 5
#define BUFFER_SIZE 10
#define TOTAL_ITEMS 50

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

int buffer[BUFFER_SIZE];
int in = 0;
int out = 0;
int count = 0;

int produced_count = 0;
int consumed_count = 0;

void *producer_thread(void *arg)
{
	while (1)
	{
		// 생산할 아이템 생성
		int item = rand() % 100;
		pthread_mutex_lock(&mutex);
		// 버퍼가 가득 찼으면 대기
		while (count == BUFFER_SIZE)
		{
			if (produced_count == TOTAL_ITEMS)
			{
				pthread_cond_broadcast(&not_empty);
				pthread_mutex_unlock(&mutex);
				return NULL;
			}
			pthread_cond_wait(&not_full, &mutex);
		}
		if (produced_count == TOTAL_ITEMS)
		{
			pthread_cond_broadcast(&not_empty);
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
		// 버퍼에 아이템 추가
		buffer[in] = item;
		in = (in + 1) % BUFFER_SIZE;
		count++;
		produced_count++;
		printf("Producer %ld produced %d (Total produced: %d)\n", (
			long)arg, item, produced_count);
		// 컨슈머에게 알림
		pthread_cond_broadcast(&not_full);
		pthread_mutex_unlock(&mutex);
		// 생산 속도 조절
		usleep(rand() % 1000);
		
	}
	return NULL;
}

void consume_item(void)
{
	// 소비 속도 조절
	usleep(rand() % 1000);
}

void *consumer_thread(void *arg)
{
	(void)arg;

	while (1)
	{
		pthread_mutex_lock(&mutex);
		while (count == 0)
		{
			if (consumed_count == TOTAL_ITEMS)
			{
				pthread_cond_broadcast(&not_full);
				pthread_mutex_unlock(&mutex);
				return NULL;
			}
			pthread_cond_wait(&not_empty, &mutex);
		}

		// 버퍼에서 아이템 제거
		int item = buffer[out];
		out = (out + 1) % BUFFER_SIZE;
		count--;
		consumed_count++;
		printf("Consumer %ld consumed %d (Total consumed: %d)\n", (
			long)arg, item, consumed_count);
		

		pthread_cond_broadcast(&not_full);
		pthread_mutex_unlock(&mutex);
		consume_item();
	}
	return NULL;
}

/* 
	프로듀서가 3이고 컨슈머가 3일때 종료 되지 않고 블록킹 현상이 있음
	이때 프로듀서가 모두 종료 되어도 컨슈머는 계속 대기 상태에 빠지게 됨

*/

/* 
	프로 듀서는 총 TOTAL_ITEMS 개의 아이템을 생산하고,
	컨슈며는 총 TOTAL_ITEMS 개의 아이템을 소비함.

	프로듀서가 생산후 버퍼에 아이템을 넣고,
	컨슈머가 버퍼에서 아이템을 꺼내 소비하는 방식으로 구현

*/
int main()
{
	pthread_t producers[PRODUCERS], consumers[CONSUMERS];

	srand(time(NULL));

	// 프로듀서 스레드 생성
	for (long i = 0; i < PRODUCERS; i++)
	{
		pthread_create(&producers[i], NULL, producer_thread, (void *)i);
	}

	// 컨슈머 스레드 생성
	for (long i = 0; i < CONSUMERS; i++)
	{
		pthread_create(&consumers[i], NULL, consumer_thread, (void *)i);
	}

	// 프로듀서 스레드 종료 대기
	for (int i = 0; i < PRODUCERS; i++)
	{
		pthread_join(producers[i], NULL);
	}
	// 컨슈머 스레드 종료 대기
	for (int i = 0; i < CONSUMERS; i++)
	{
		//pthread_cancel(consumers[i]);
		pthread_join(consumers[i], NULL);
	}
	return 0;
}