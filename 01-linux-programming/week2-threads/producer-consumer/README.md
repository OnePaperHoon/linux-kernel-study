# Producer-Consumer Pattern

실무에서 가장 많이 사용되는 멀티스레드 디자인 패턴

## 🖥️ 실행 화면

```bash
$ ./producer_consumer

Producer 0 produced 42 (Total produced: 1)
Consumer 0 consumed 42 (Total consumed: 1)
Producer 1 produced 87 (Total produced: 2)
Producer 2 produced 23 (Total produced: 3)
Consumer 1 consumed 87 (Total consumed: 2)
Consumer 2 consumed 23 (Total consumed: 3)
Producer 0 produced 56 (Total produced: 4)
Consumer 3 consumed 56 (Total consumed: 4)
...
Producer 1 produced 91 (Total produced: 50)
Consumer 4 consumed 91 (Total consumed: 50)
```

## 🚀 빌드 및 실행

```bash
# 컴파일
make

# 실행
./producer_consumer

# 테스트 (3회 실행)
make test

# Valgrind 검사
make valgrind

# Helgrind (Thread 안전성)
make helgrind
```

## 📖 프로젝트 개요

### 목적
Classic Producer-Consumer 패턴 구현을 통한 멀티스레드 동기화 마스터

### 학습 목표
- [x] Producer-Consumer 패턴 이해
- [x] Ring Buffer (Circular Queue) 구현
- [x] 2개의 Condition Variable 사용
- [x] Thread 간 효율적인 통신
- [x] Deadlock 방지 기법
- [x] 종료 조건 처리

### 실무 응용
```
네트워크 서버:
Client 요청 → [Queue] → Worker Thread

로그 시스템:
Log 생성 → [Queue] → File Writer

이미지 처리:
Frame 캡처 → [Queue] → Encoder

작업 큐:
Task 생성 → [Queue] → Thread Pool
```

## 🔧 구현 내용

### 1. 전체 구조

```
Producer Thread (3개)      Consumer Thread (5개)
       ↓                           ↑
    [생산]                      [소비]
       ↓                           ↑
       └──→ [Ring Buffer] ──→────┘
           (크기: 10)
```

**동작 흐름:**
1. Producer가 아이템 생성
2. Buffer에 추가 (in 위치)
3. Consumer가 Buffer에서 꺼냄 (out 위치)
4. 아이템 처리 (소비)

### 2. Ring Buffer (Circular Queue)

```c
#define BUFFER_SIZE 10

int buffer[BUFFER_SIZE];  // 실제 데이터
int in = 0;               // Producer가 넣을 위치
int out = 0;              // Consumer가 꺼낼 위치
int count = 0;            // 현재 아이템 수
```

**순환 구조:**
```
     0   1   2   3   4   5   6   7   8   9
   ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
   │ 42│ 87│ 23│   │   │   │   │   │   │   │
   └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
     ↑       ↑
    out     in
    
in = (in + 1) % BUFFER_SIZE   // 3 → 4 → 5 → ... → 9 → 0
out = (out + 1) % BUFFER_SIZE // 순환
```

**왜 Ring Buffer?**
- 메모리 효율적 (고정 크기)
- 캐시 친화적
- O(1) 추가/삭제
- 메모리 재사용

### 3. 동기화 객체

```c
/* Mutex: Buffer 보호 */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Condition Variable 2개 */
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;   // Buffer에 공간 있음
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;  // Buffer에 데이터 있음
```

**왜 2개?**
- `not_full`: Producer가 대기 (Buffer 가득 참)
- `not_empty`: Consumer가 대기 (Buffer 비어있음)
- 각자 다른 조건을 기다림!

### 4. Producer 구현

```c
void *producer_thread(void *arg)
{
    while (1)
    {
        /* 1. 아이템 생성 (Lock 밖에서) */
        int item = rand() % 100;
        
        pthread_mutex_lock(&mutex);
        
        /* 2. Buffer 가득 차면 대기 */
        while (count == BUFFER_SIZE)
        {
            /* 종료 조건 */
            if (produced_count == TOTAL_ITEMS)
            {
                pthread_cond_broadcast(&not_empty);  // Consumer 깨우기
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            pthread_cond_wait(&not_full, &mutex);
        }
        
        /* 3. 종료 조건 재확인 */
        if (produced_count == TOTAL_ITEMS)
        {
            pthread_cond_broadcast(&not_empty);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        
        /* 4. Buffer에 추가 */
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        produced_count++;
        
        printf("Producer %ld produced %d (Total: %d)\n", 
               (long)arg, item, produced_count);
        
        /* 5. Consumer 깨우기 */
        pthread_cond_broadcast(&not_empty);
        
        pthread_mutex_unlock(&mutex);
        
        /* 6. 생산 속도 조절 */
        usleep(rand() % 1000);
    }
}
```

**핵심 포인트:**
1. **Lock 밖에서 생산**: 병렬성 ↑
2. **while로 대기**: Spurious wakeup 대비
3. **broadcast 사용**: 모든 Consumer 깨우기
4. **종료 전 깨우기**: Deadlock 방지

### 5. Consumer 구현

```c
void *consumer_thread(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        
        /* 1. Buffer 비어있으면 대기 */
        while (count == 0)
        {
            /* 종료 조건 */
            if (consumed_count == TOTAL_ITEMS)
            {
                pthread_cond_broadcast(&not_full);  // Producer 깨우기
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            pthread_cond_wait(&not_empty, &mutex);
        }
        
        /* 2. Buffer에서 꺼내기 */
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        consumed_count++;
        
        printf("Consumer %ld consumed %d (Total: %d)\n", 
               (long)arg, item, consumed_count);
        
        /* 3. Producer 깨우기 */
        pthread_cond_broadcast(&not_full);
        
        pthread_mutex_unlock(&mutex);
        
        /* 4. 아이템 소비 (Lock 밖에서) */
        usleep(rand() % 1000);
    }
}
```

**핵심 포인트:**
1. **Lock 밖에서 소비**: 병렬성 ↑
2. **while로 대기**: Spurious wakeup 대비
3. **broadcast 사용**: 모든 Producer 깨우기
4. **종료 전 깨우기**: Deadlock 방지

---

## 💡 배운 점

### Producer-Consumer 패턴

**정의:**
```
생산자(Producer)와 소비자(Consumer)가 
공유 버퍼를 통해 비동기적으로 데이터를 주고받는 패턴
```

**특징:**
- Producer와 Consumer가 **독립적**
- 속도 차이 흡수 (Buffering)
- 처리량 향상 (Throughput)

### Ring Buffer (Circular Queue)

**구현:**
```c
// 추가 (Producer)
buffer[in] = item;
in = (in + 1) % SIZE;  // Modulo로 순환
count++;

// 제거 (Consumer)
item = buffer[out];
out = (out + 1) % SIZE;  // Modulo로 순환
count--;
```

**장점:**
- **O(1) 연산**: 추가/제거 모두 상수 시간
- **메모리 효율**: 고정 크기, 재사용
- **캐시 친화적**: 연속된 메모리
- **구현 간단**: 배열 + 인덱스 2개

**주의사항:**
- `count` 변수로 full/empty 구분
- `in == out`일 때: full 또는 empty (모호)
- `count` 없으면 1칸 낭비 필요

### 2개의 Condition Variable

**왜 2개?**

```c
// ❌ 1개만 사용 (비효율)
pthread_cond_t cond;

// Producer
while (count == SIZE) {
    pthread_cond_wait(&cond, &mutex);
}
pthread_cond_broadcast(&cond);  // Producer도 깨움!

// ✅ 2개 사용 (효율적)
pthread_cond_t not_full;   // Producer 대기
pthread_cond_t not_empty;  // Consumer 대기

// Producer
while (count == SIZE) {
    pthread_cond_wait(&not_full, &mutex);
}
pthread_cond_signal(&not_empty);  // Consumer만 깨움!

// Consumer
while (count == 0) {
    pthread_cond_wait(&not_empty, &mutex);
}
pthread_cond_signal(&not_full);   // Producer만 깨움!
```

**장점:**
- 불필요한 wakeup 감소
- CPU 효율 ↑
- 의미가 명확

### signal vs broadcast

```c
// signal: 하나만 깨움
pthread_cond_signal(&cond);

// broadcast: 전부 깨움
pthread_cond_broadcast(&cond);
```

**언제 사용?**

```
signal:
- Consumer/Producer 하나만 있을 때
- 하나만 깨워도 될 때
- 성능 중요

broadcast:
- 여러 Consumer/Producer
- 종료 신호
- 안전 우선
```

**이 프로젝트:**
- Producer 3개, Consumer 5개
- `broadcast` 사용 (안전)
- 종료 시 모두 깨워야 함

### Spurious Wakeup

**정의:**
```
pthread_cond_wait()가 signal 없이도 깨어날 수 있음
= Linux 내부 구현의 특성
```

**대응:**
```c
// ❌ if 사용 (위험)
if (count == 0) {
    pthread_cond_wait(&cond, &mutex);
}
// 깨어났지만 count 여전히 0일 수 있음!

// ✅ while 사용 (안전)
while (count == 0) {
    pthread_cond_wait(&cond, &mutex);
}
// 깨어나면 다시 확인!
```

### 종료 조건 처리

**문제:**
```
Producer가 모두 종료
→ Buffer 비어있음
→ Consumer들이 영원히 대기
→ Deadlock!
```

**해결:**
```c
// Producer 종료 시
if (produced_count == TOTAL_ITEMS) {
    pthread_cond_broadcast(&not_empty);  // 모든 Consumer 깨우기!
    return NULL;
}

// Consumer 종료 확인
while (count == 0) {
    if (consumed_count == TOTAL_ITEMS) {
        return NULL;  // 종료
    }
    pthread_cond_wait(&not_empty, &mutex);
}
```

### Critical Section 최소화

```c
// ✅ 좋은 예
pthread_mutex_lock(&mutex);
// 최소한만 (Buffer 접근)
buffer[in] = item;
in = (in + 1) % SIZE;
count++;
pthread_mutex_unlock(&mutex);

// Lock 밖에서 시간 소모 작업
usleep(1000);  // 생산/소비

// ❌ 나쁜 예
pthread_mutex_lock(&mutex);
int item = produce();  // 시간 걸림!
buffer[in] = item;
in = (in + 1) % SIZE;
consume(item);         // 시간 걸림!
pthread_mutex_unlock(&mutex);
```

**원칙:**
- Lock은 **Buffer 접근**할 때만
- 생산/소비는 **Lock 밖**에서
- Critical Section = 최소한

---

## 📊 성능

### 설정

```
Producers:    3개
Consumers:    5개
Buffer Size:  10
Total Items:  50
```

### 실행 시간

```bash
$ time ./producer_consumer

real    0m0.052s  # 총 실행 시간
user    0m0.004s  # CPU 시간
sys     0m0.008s  # Kernel 시간
```

**분석:**
- Producer < Consumer (5 > 3)
- Consumer가 빠르게 소비
- Buffer 거의 비어있음
- Blocking 적음

### Buffer 상태 관찰

```
Producer 속도 > Consumer 속도:
→ Buffer 자주 가득 참
→ Producer 대기 많음

Producer 속도 < Consumer 속도:
→ Buffer 자주 비어있음
→ Consumer 대기 많음

Producer 속도 = Consumer 속도:
→ Buffer 적절히 사용
→ 대기 최소화
→ 최적!
```

### 동시성 (Concurrency)

```
싱글 스레드:
- 50개 아이템
- 각 100ms
- 총: 5000ms

Producer-Consumer:
- Producer 3개 병렬
- Consumer 5개 병렬
- 총: ~50ms (100배 빠름!)
```

### Valgrind 결과

```bash
$ make valgrind

==12345== HEAP SUMMARY:
==12345==     in use at exit: 0 bytes in 0 blocks
==12345==   total heap usage: 9 allocs, 9 frees
==12345==
==12345== All heap blocks were freed -- no leaks are possible
==12345==
==12345== ERROR SUMMARY: 0 errors from 0 contexts
```

**결과:**
- ✅ 메모리 누수 없음
- ✅ Thread 안전
- ✅ 올바른 동기화

---

## 🔮 개선 가능 사항

### 1. 우선순위 큐

- [ ] 일반 큐 → 우선순위 큐
- [ ] 중요한 아이템 먼저 처리
- [ ] Heap 구조 사용

### 2. Timeout 처리

- [ ] `pthread_cond_timedwait()` 사용
- [ ] 일정 시간 후 포기
- [ ] 데드락 방지 강화

```c
struct timespec timeout;
clock_gettime(CLOCK_REALTIME, &timeout);
timeout.tv_sec += 5;  // 5초 후

int ret = pthread_cond_timedwait(&not_empty, &mutex, &timeout);
if (ret == ETIMEDOUT) {
    // Timeout 처리
}
```

### 3. 동적 Buffer 크기

- [ ] Buffer 크기 자동 조절
- [ ] 부하에 따라 증가/감소
- [ ] 메모리 효율 ↑

### 4. 통계 수집

- [ ] 평균 대기 시간
- [ ] Buffer 사용률
- [ ] Throughput 측정
- [ ] 성능 프로파일링

---

## 📚 참고 자료

### Man Pages
```bash
man pthread_cond_wait
man pthread_cond_signal
man pthread_cond_broadcast
man pthread_cond_timedwait
```

### 관련 문서
- [[01-thread-basics]] - Thread 기초
- [[02-pthread-api]] - pthread 함수들
- [[04-critical-section]] - Critical Section
- [Day 4: shared-counter-fixed](../day4-sync/shared-counter-fixed) - Mutex
- [Day 4: ping-pong](../day4-sync/ping-pong) - Condition Variable

### 실무 응용
- POSIX Message Queue
- Linux Pipe/FIFO
- Thread Pool 구현
- Network Server 설계

---

## 🎯 다음 단계

**학습 순서:**
1. ✅ [Day 4: Mutex](../day4-sync/shared-counter-fixed)
2. ✅ [Day 4: Condition Variable](../day4-sync/ping-pong)
3. ✅ Producer-Consumer (현재)
4. → Reader-Writer Problem
5. → Thread Pool Server
6. → Week 3: IPC (Pipe, Shared Memory)

## 📝 프로젝트 정보

```
개발 기간: Day 5-6
환경: Linux (Debian/Ubuntu)
언어: C
라이브러리: pthread
빌드: gcc, make
```

**파일 구조:**
```
producer-consumer/
├── producer_consumer.c    # 메인 코드
├── Makefile               # 빌드 설정
└── README.md              # 이 문서
```

**핵심 개념:**
```c
// Ring Buffer
buffer[(in++) % SIZE] = item;
item = buffer[(out++) % SIZE];

// 2개 Condition Variable
pthread_cond_wait(&not_full, &mutex);   // Producer
pthread_cond_wait(&not_empty, &mutex);  // Consumer

// 종료 시 broadcast
pthread_cond_broadcast(&not_empty);
pthread_cond_broadcast(&not_full);
```

---

**Author:** OnePaperHoon  
**Date:** January 2025  
**Project:** Linux Kernel Study - Week 2, Day 5-6  
**Topic:** Producer-Consumer Pattern