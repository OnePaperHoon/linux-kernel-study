/*
 * Thread Pool Test Program
 */

#include "thread_pool.h"
#include <time.h>

#define NUM_TASKS 20

// 테스트용 뮤텍스 (출력 순서 보호)
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// 테스트 1: 간단한 작업 - 숫자 출력
void print_task(void *arg)
{
	int num = *(int *)arg;

	usleep(rand() % 50000); // 0~50ms 랜덤 딜레이
	pthread_mutex_lock(&print_mutex);
	printf("[Task %2d] 실행 완료 (thread: %lu)\n", num, (unsigned long)pthread_self());
	pthread_mutex_unlock(&print_mutex);
}

// 테스트 2: 계산 작업 - 팩토리얼 계산
void factorial_task(void *arg)
{
	int n = *(int *)arg;
	long result = 1;
	int i;

	for (i = 1; i <= n; i++)
		result *= i;

	pthread_mutex_lock(&print_mutex);
	printf("[Factorial] %d! = %ld (thread: %lu)\n", n, result, (unsigned long)pthread_self());
	pthread_mutex_unlock(&print_mutex);
}

// 테스트 3: 피보나치 계산
void fibonacci_task(void *arg)
{
	int n = *(int *)arg;
	int a = 0;
	int b = 1;
	int temp;
	int i;

	for (i = 2; i <= n; i++)
	{
		temp = a + b;
		a = b;
		b = temp;
	}

	pthread_mutex_lock(&print_mutex);
	printf("[Fibonacci] fib(%d) = %d (thread: %lu)\n", n, (n <= 1) ? n : b, (unsigned long)pthread_self());
	pthread_mutex_unlock(&print_mutex);
}

int main(void)
{
	thread_pool_t *pool;
	int task_args[NUM_TASKS];
	int fib_args[] = {10, 20, 30, 35, 40};
	int fact_args[] = {5, 8, 10, 12, 15};
	int i;

	srand(time(NULL));

	printf("=== Thread Pool Test ===\n\n");

	// 스레드 풀 생성 (4개의 워커 스레드)
	pool = thread_pool_create(POOL_SIZE);
	if (pool == NULL)
	{
		fprintf(stderr, "Failed to create thread pool\n");
		return 1;
	}
	printf("Thread pool created with %d workers\n\n", POOL_SIZE);

	// 테스트 1: 기본 작업 20개 추가
	printf("--- Test 1: Basic Tasks (%d tasks) ---\n", NUM_TASKS);
	for (i = 0; i < NUM_TASKS; i++)
	{
		task_args[i] = i + 1;
		thread_pool_add_task(pool, print_task, &task_args[i]);
	}

	// 모든 작업 완료 대기
	thread_pool_wait(pool);
	printf("\nAll basic tasks completed!\n\n");

	// 테스트 2: 팩토리얼 계산 작업
	printf("--- Test 2: Factorial Tasks ---\n");
	for (i = 0; i < 5; i++)
		thread_pool_add_task(pool, factorial_task, &fact_args[i]);

	thread_pool_wait(pool);
	printf("\nAll factorial tasks completed!\n\n");

	// 테스트 3: 피보나치 계산 작업
	printf("--- Test 3: Fibonacci Tasks ---\n");
	for (i = 0; i < 5; i++)
		thread_pool_add_task(pool, fibonacci_task, &fib_args[i]);

	thread_pool_wait(pool);
	printf("\nAll fibonacci tasks completed!\n\n");

	// 스레드 풀 종료
	thread_pool_destroy(pool);
	printf("Thread pool destroyed. All tests passed!\n");

	pthread_mutex_destroy(&print_mutex);
	return 0;
}
