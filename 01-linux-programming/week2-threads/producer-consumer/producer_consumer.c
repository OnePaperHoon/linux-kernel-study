#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define PRODUCERS 2
#define CONSUMERS 3
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
			pthread_cond_wait(&not_empty, &mutex);
		}
		// 버퍼에 아이템 추가
		buffer[in] = item;
		in = (in + 1) % BUFFER_SIZE;
		count++;
		produced_count++;
		printf("Producer %ld produced %d (Total produced: %d)\n", (
			long)arg, item, produced_count);
		// 컨슈머에게 알림
		pthread_cond_broadcast(&not_empty);
		pthread_mutex_unlock(&mutex);
		// 생산 속도 조절
		usleep(rand() % 100000);
		if (produced_count >= TOTAL_ITEMS)
			break;
	}
	return NULL;
}

void consume_item(int item)
{
	// 소비 속도 조절
	(void)item; // 사용하지 않는 변수 경고 방지
	usleep(rand() % 150000);
}

void *consumer_thread(void *arg)
{
	(void)arg;

	while (1)
	{
		pthread_mutex_lock(&mutex);

		// 버퍼가 비어있으면 대기
		while (count == 0)
		{
			pthread_cond_wait(&not_full, &mutex);
		}

		// 버퍼에서 아이템 제거
		int item = buffer[out];
		out = (out + 1) % BUFFER_SIZE;
		count--;
		consumed_count++;
		printf("Consumer %ld consumed %d (Total consumed: %d)\n", (
			long)arg, item, consumed_count);
		
		// 프로듀서에게 알림
		pthread_cond_signal(&not_full);
		pthread_mutex_unlock(&mutex);
		consume_item(item);
	}
	return NULL;
}



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
		pthread_cancel(consumers[i]);
		pthread_join(consumers[i], NULL);
	}
	return 0;
}