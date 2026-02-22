# Thread Pool

Worker Thread들을 미리 생성해두고 Task를 큐에 넣어 처리하는 패턴

## 🖥️ 실행 화면

```bash
$ ./thread_pool

=== Thread Pool Test ===

Thread pool created with 4 workers

--- Test 1: Basic Tasks (20 tasks) ---
[Task  1] 실행 완료 (thread: 140123456001)
[Task  3] 실행 완료 (thread: 140123456002)
[Task  2] 실행 완료 (thread: 140123456003)
[Task  4] 실행 완료 (thread: 140123456001)
...
[Task 20] 실행 완료 (thread: 140123456004)

All basic tasks completed!

--- Test 2: Factorial Tasks ---
[Factorial]  5! = 120    (thread: 140123456001)
[Factorial]  8! = 40320  (thread: 140123456002)
[Factorial] 10! = 3628800 (thread: 140123456003)
[Factorial] 12! = 479001600 (thread: 140123456004)
[Factorial] 15! = 1307674368000 (thread: 140123456001)

All factorial tasks completed!

--- Test 3: Fibonacci Tasks ---
[Fibonacci] fib(10) = 55   (thread: 140123456002)
[Fibonacci] fib(20) = 6765 (thread: 140123456003)
[Fibonacci] fib(30) = 832040 (thread: 140123456001)
[Fibonacci] fib(35) = 9227465 (thread: 140123456004)
[Fibonacci] fib(40) = 102334155 (thread: 140123456002)

All fibonacci tasks completed!

Thread pool destroyed. All tests passed!
```

## 🚀 빌드 및 실행

```bash
# 빌드
make

# 실행
make test

# 메모리 검사
make valgrind

# 디버그 빌드
make debug

# AddressSanitizer
make sanitize
```

## 📖 프로젝트 개요

### 목적
Worker Thread를 미리 생성해두는 Thread Pool 구현으로 실전 멀티스레드 패턴 마스터

### 학습 목표
- [x] Thread Pool 아키텍처 설계
- [x] Linked List 기반 Task Queue 구현
- [x] 3개 Condition Variable 활용 (not_empty, not_full, idle)
- [x] Graceful Shutdown 구현
- [x] thread_pool_wait() 배리어 동기화
- [x] 메모리 누수 없는 자원 정리

### 실무 응용
```
웹 서버:
Request 도착 → [Task Queue] → Worker Thread → Response

데이터베이스:
Query 요청 → [Task Queue] → Worker Thread → 결과 반환

이미지 처리:
이미지 업로드 → [Task Queue] → Worker Thread → 인코딩
```

## 🔧 구현 내용

### 1. 자료구조

```c
/* Task: Linked List 노드 */
typedef struct task {
    thread_func_t  function; // 실행할 함수 포인터
    void          *arg;      // 함수 인자
    struct task   *next;     // 다음 Task
} task_t;

/* Thread Pool */
typedef struct {
    pthread_t       threads[POOL_SIZE]; // Worker Thread 배열
    task_t         *queue_head;         // 큐 Head
    task_t         *queue_tail;         // 큐 Tail (O(1) enqueue)
    pthread_mutex_t mutex;              // 동기화
    pthread_cond_t  not_empty;          // Worker 대기 (큐 비면)
    pthread_cond_t  not_full;           // add_task 대기 (큐 가득 차면)
    pthread_cond_t  idle;               // wait() 대기 (모든 작업 완료)
    bool            shutdown;           // 종료 플래그
    size_t          thread_count;       // Worker 수
    size_t          queue_size;         // 현재 큐 크기
    size_t          active_tasks;       // 실행 중인 Task 수
} thread_pool_t;
```

**Ring Buffer vs Linked List:**
```
Ring Buffer (producer-consumer):
- 고정 크기, 메모리 효율적
- 인덱스 기반 O(1)
- 사전에 크기 결정 필요

Linked List (이 프로젝트):
- 동적 크기, 유연함
- 포인터 기반 O(1)
- malloc/free 오버헤드
→ Thread Pool은 Task 수가 가변적이므로 Linked List 선택
```

### 2. Condition Variable 3개 역할

```
not_empty: Worker가 대기 (큐가 비어있을 때)
           신호: add_task()가 Task 추가 후 signal

not_full:  add_task()가 대기 (큐가 가득 찼을 때, TASK_QUEUE_SIZE=100)
           신호: Worker가 Task 꺼낸 후 signal

idle:      thread_pool_wait()가 대기 (아직 작업 남아있을 때)
           신호: Worker가 Task 완료 후, 큐도 비고 active=0이면 signal
```

### 3. Worker Thread 동작

```c
static void *thread_pool_worker(void *arg)
{
    thread_pool_t *pool = (thread_pool_t *)arg;
    task_t *task;

    while (1)
    {
        pthread_mutex_lock(&pool->mutex);

        /* 큐가 비고 종료 신호 없으면 대기 */
        while (pool->queue_head == NULL && !pool->shutdown)
            pthread_cond_wait(&pool->not_empty, &pool->mutex);

        /* 종료 신호 + 큐 비면 탈출 */
        if (pool->shutdown && pool->queue_head == NULL)
        {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        /* Dequeue */
        task = pool->queue_head;
        pool->queue_head = task->next;
        if (pool->queue_head == NULL)
            pool->queue_tail = NULL;
        pool->queue_size--;
        pool->active_tasks++;

        pthread_cond_signal(&pool->not_full); // add_task 깨우기
        pthread_mutex_unlock(&pool->mutex);

        /* Task 실행 (Lock 밖에서 → 병렬성 확보) */
        task->function(task->arg);
        destroy_task(task);

        /* 완료 처리 */
        pthread_mutex_lock(&pool->mutex);
        pool->active_tasks--;
        if (pool->queue_head == NULL && pool->active_tasks == 0)
            pthread_cond_signal(&pool->idle); // wait() 깨우기
        pthread_mutex_unlock(&pool->mutex);
    }
    return NULL;
}
```

### 4. Graceful Shutdown

```c
void thread_pool_destroy(thread_pool_t *pool)
{
    /* 1. shutdown 플래그 설정 */
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->not_empty); // 대기 중인 Worker 모두 깨우기
    pthread_mutex_unlock(&pool->mutex);

    /* 2. 모든 Worker 종료 대기 */
    for (size_t i = 0; i < pool->thread_count; i++)
        pthread_join(pool->threads[i], NULL);

    /* 3. 남은 Task 정리 */
    while (pool->queue_head != NULL) { ... }

    /* 4. 동기화 자원 해제 */
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
    pthread_cond_destroy(&pool->idle);

    free(pool);
}
```

**Shutdown 시 Worker 동작:**
```
shutdown = true 설정
    ↓
pthread_cond_broadcast(&not_empty)
    ↓
대기 중인 Worker 깨어남
    ↓
while (queue_head == NULL && !shutdown) → 조건 불만족, 탈출
    ↓
if (shutdown && queue_head == NULL) → true → break
    ↓
Worker 종료
```

### 5. 공개 API

```c
/* 생성 */
thread_pool_t *thread_pool_create(size_t pool_size);

/* Task 추가 */
int thread_pool_add_task(thread_pool_t *pool, thread_func_t function, void *arg);

/* 모든 Task 완료 대기 (배리어) */
void thread_pool_wait(thread_pool_t *pool);

/* 종료 및 자원 해제 */
void thread_pool_destroy(thread_pool_t *pool);
```

## 💡 배운 점

### Thread Pool 패턴

**왜 Thread를 미리 만드나?**
```
매번 Thread 생성:
Task 도착 → pthread_create() → 실행 → pthread_join() → 삭제
           └──── ~500μs ────┘                └─ 비용 ─┘

Thread Pool:
Task 도착 → [Queue] → Worker(이미 존재) → 실행
           └── O(1)──┘
→ Thread 생성/삭제 비용 제거
→ 처리량 대폭 향상
```

### idle Condition Variable의 역할

**thread_pool_wait() 구현:**
```c
void thread_pool_wait(thread_pool_t *pool)
{
    pthread_mutex_lock(&pool->mutex);
    /* 큐에 Task 남거나, 실행 중인 Task 있으면 대기 */
    while (pool->queue_head != NULL || pool->active_tasks > 0)
        pthread_cond_wait(&pool->idle, &pool->mutex);
    pthread_mutex_unlock(&pool->mutex);
}
```

**신호 시점 (Worker 완료 후):**
```c
pool->active_tasks--;
if (pool->queue_head == NULL && pool->active_tasks == 0)
    pthread_cond_signal(&pool->idle); // 모두 끝난 경우만!
```

→ `queue_head == NULL`이어도 `active_tasks > 0`이면 아직 실행 중이므로 대기 유지

### Lock 범위 최소화

```c
/* ✅ 이 프로젝트 방식 */
pthread_mutex_unlock(&pool->mutex);

task->function(task->arg); // Lock 밖에서 실행! → 병렬성 확보
destroy_task(task);

pthread_mutex_lock(&pool->mutex);

/* ❌ 잘못된 방식 */
pthread_mutex_lock(&pool->mutex);
task->function(task->arg); // Lock 안에서 실행 → 순차 실행이 됨
pthread_mutex_unlock(&pool->mutex);
```

### 메모리 관리 전략

```
생성 순서:
pool (calloc) → mutex/cond 초기화 → threads 생성
                                          ↓
                                    thread_pool_worker 실행 시작

해제 순서:
shutdown 플래그 → broadcast → join(threads) → 남은 task 해제
→ cond destroy → mutex destroy → free(pool)

핵심: thread 먼저 종료 후 → 나머지 자원 해제
(역순 보장으로 use-after-free 방지)
```

## 📊 성능

### 설정
```
POOL_SIZE:       4 (Worker Thread)
TASK_QUEUE_SIZE: 100 (최대 큐 크기)
테스트 Task:     20 + 5 + 5 = 30개
```

### 실행 시간

```bash
$ time make test

real    0m0.503s   # usleep(rand() % 50000) × 20 tasks / 4 workers
user    0m0.012s
sys     0m0.008s

→ 싱글 스레드라면: 20 × 25ms(평균) = 500ms
→ 4 Workers:       500ms / 4 ≈ 125ms (이론상)
→ 실측: ~503ms (대기 시간 포함)
```

### Valgrind 결과

```bash
$ make valgrind

==12345== HEAP SUMMARY:
==12345==     in use at exit: 0 bytes in 0 blocks
==12345==   total heap usage: 34 allocs, 34 frees
==12345==
==12345== All heap blocks were freed -- no leaks are possible
==12345==
==12345== ERROR SUMMARY: 0 errors from 0 contexts
```

- ✅ 메모리 누수 없음
- ✅ 모든 Thread 정상 종료
- ✅ 모든 동기화 자원 해제

## 🔮 개선 가능 사항

- [ ] 동적 Worker 수 조절 (부하에 따라 증가/감소)
- [ ] Task 우선순위 큐 (중요 Task 먼저 처리)
- [ ] Task 취소 기능 (thread_pool_cancel_task)
- [ ] 통계 수집 (평균 대기 시간, 처리량, Worker 활용률)
- [ ] Timeout 지원 (pthread_cond_timedwait)

## 📚 참고 자료

### Man Pages
```bash
man pthread_create
man pthread_cond_wait
man pthread_cond_signal
man pthread_cond_broadcast
man pthread_mutex_lock
```

### 관련 프로젝트
- [producer-consumer](../producer-consumer) - Ring Buffer 큐 패턴
- [shared-counter-fixed](../sync/shared-counter-fixed) - Mutex 기초
- [ping-pong](../sync/ping-pong) - Condition Variable 기초

### 실무 응용
- NGINX Worker Process 모델
- Node.js libuv Thread Pool
- Java ThreadPoolExecutor

## 🎯 다음 단계

1. ✅ Thread Pool (현재)
2. → Week 3: IPC — Pipe, FIFO, Shared Memory, Message Queue
3. → Week 4: Network Programming

## 📝 프로젝트 정보

```
개발 기간: Day 7
환경: Linux (Debian/Ubuntu)
언어: C
라이브러리: pthread
빌드: gcc, make
```

**파일 구조:**
```
thread-pool/
├── thread_pool.h    # 자료구조 및 API 선언
├── thread_pool.c    # 구현 (create, destroy, add_task, wait, worker)
├── main.c           # 테스트 (기본, Factorial, Fibonacci)
└── Makefile         # all / test / valgrind / debug / sanitize
```

**핵심 흐름:**
```c
pool = thread_pool_create(4);    // Worker 4개 생성 및 대기

thread_pool_add_task(pool, fn, arg); // Task 추가 → Worker 깨움
thread_pool_wait(pool);              // 모든 Task 완료까지 대기

thread_pool_destroy(pool);       // shutdown → join → free
```

---

**Author:** OnePaperHoon
**Date:** February 2026
**Project:** Linux Kernel Study - Week 2, Day 7
**Topic:** Thread Pool (Mutex + 3 Condition Variables + Linked List Queue)
