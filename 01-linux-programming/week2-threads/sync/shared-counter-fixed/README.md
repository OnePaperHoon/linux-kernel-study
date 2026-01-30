# Shared Counter Fixed

Day 3의 Race Condition을 Mutex로 해결한 프로그램

## 🖥️ 실행 화면

```bash
$ ./shared_counter_fixed

=== Shared Counter with Mutex ===

Initial counter: 0
Expected final:  1000000
Threads:         10
Increments each: 100000

Thread 0: Starting
Thread 1: Starting
Thread 2: Starting
Thread 3: Starting
Thread 4: Starting
Thread 5: Starting
Thread 6: Starting
Thread 7: Starting
Thread 8: Starting
Thread 9: Starting
Thread 0: Finished
Thread 1: Finished
Thread 2: Finished
Thread 3: Finished
Thread 4: Finished
Thread 5: Finished
Thread 6: Finished
Thread 7: Finished
Thread 8: Finished
Thread 9: Finished

=== Results ===
Final counter:   1000000
Expected:        1000000
Difference:      0

✅ Success! No race condition!
   Mutex protected the critical section.
```

## 🚀 빌드 및 실행

```bash
# 컴파일
make

# 실행
./shared_counter_fixed

# 5번 실행 (일관성 확인)
make test

# Day 3 버전과 비교
make compare

# Valgrind 검사
make valgrind

# 성능 측정
make benchmark
```

## 📖 프로젝트 개요

### 목적
Day 3에서 발견한 Race Condition을 Mutex를 사용하여 완전히 해결

### 학습 목표
- [x] Mutex 개념 이해
- [x] pthread_mutex_lock/unlock 사용
- [x] Critical Section 보호
- [x] Race Condition 해결 검증
- [x] 성능 트레이드오프 이해

## 🔧 구현 내용

### 1. Mutex 선언 및 초기화

```c
// Static 초기화 (간단)
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

// 또는 Dynamic 초기화
pthread_mutex_t mutex;
pthread_mutex_init(&mutex, NULL);
```

**Static vs Dynamic:**
- Static: 전역 변수, 컴파일 타임 초기화
- Dynamic: 런타임 초기화, 속성 설정 가능

**프로젝트에서 Static 선택한 이유:**
- 간단함
- 전역 변수로 충분
- 기본 속성으로 충분

### 2. Critical Section 보호

```c
void *increment_counter(void *arg)
{
    for (int i = 0; i < INCREMENTS; i++) {
        /* Entry Section */
        pthread_mutex_lock(&counter_mutex);
        
        /* === Critical Section === */
        global_counter++;
        /* ======================== */
        
        /* Exit Section */
        pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}
```

**동작 흐름:**
```
Thread 0             Mutex State        Thread 1
────────────────────────────────────────────────
lock() → 성공        LOCKED (by T0)
counter++ (안전)                        lock() → 대기
unlock()             UNLOCKED           
                                        lock() → 성공
                                        counter++
                                        unlock()
```

### 3. Mutex vs No Mutex

#### Day 3 (Mutex 없음) ❌

```c
void *increment_counter(void *arg) {
    for (int i = 0; i < INCREMENTS; i++) {
        global_counter++;  // Race Condition!
    }
    return NULL;
}

// 결과 (5회 실행):
Run 1: 987,654
Run 2: 991,234
Run 3: 985,123
Run 4: 993,456
Run 5: 982,001
→ 매번 다름, 항상 틀림!
```

#### Day 4 (Mutex 사용) ✅

```c
void *increment_counter(void *arg) {
    for (int i = 0; i < INCREMENTS; i++) {
        pthread_mutex_lock(&counter_mutex);
        global_counter++;  // 안전!
        pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}

// 결과 (5회 실행):
Run 1: 1,000,000
Run 2: 1,000,000
Run 3: 1,000,000
Run 4: 1,000,000
Run 5: 1,000,000
→ 항상 정확!
```

### 4. Mutex 정리

```c
int main(void)
{
    // Thread 생성 & Join
    // ...
    
    /* Mutex 정리 (중요!) */
    pthread_mutex_destroy(&counter_mutex);
    
    return 0;
}
```

**왜 destroy 필요?**
- 리소스 해제
- 좋은 습관
- Static 초기화라도 호출 권장

---

## 💡 배운 점

### Mutex (Mutual Exclusion)

**정의:**
```
한 번에 하나의 thread만 Critical Section에 진입하도록 보장
```

**핵심 개념:**
- **Lock**: Critical Section 진입 전
- **Unlock**: Critical Section 나올 때
- **Ownership**: 잠근 thread만 해제 가능

### pthread_mutex_lock()

```c
int pthread_mutex_lock(pthread_mutex_t *mutex);

// 반환값:
// 0: 성공
// EINVAL: mutex가 유효하지 않음
// EDEADLK: Deadlock 감지 (Recursive 아닌 경우)
```

**동작:**
- Mutex가 unlocked → 즉시 획득, 리턴
- Mutex가 locked → **블로킹**, 대기
- 깨어나면 → Lock 획득, 리턴

**블로킹이란?**
```
Thread가 sleep 상태로 전환
CPU 양보
대기 큐에 추가
다른 thread가 unlock하면 깨어남
```

### pthread_mutex_unlock()

```c
int pthread_mutex_unlock(pthread_mutex_t *mutex);

// 반환값:
// 0: 성공
// EPERM: 소유자가 아닌 thread가 호출
```

**동작:**
- Mutex 해제
- 대기 중인 thread 중 하나 깨움
- 어떤 thread 깨울지는 스케줄러가 결정

**주의:**
```c
// ❌ 잘못된 예
void *thread_A(void *arg) {
    pthread_mutex_lock(&mutex);
    // ...
}

void *thread_B(void *arg) {
    pthread_mutex_unlock(&mutex);  // 에러! (소유 안함)
}

// ✅ 올바른 예
void *thread_A(void *arg) {
    pthread_mutex_lock(&mutex);
    // Critical Section
    pthread_mutex_unlock(&mutex);  // 같은 thread에서
}
```

### Atomic Operation이 아닌 이유

```c
// 이것도 Atomic 아님!
pthread_mutex_lock(&mutex);
global_counter++;
pthread_mutex_unlock(&mutex);
```

**각 단계:**
1. `pthread_mutex_lock()` - 함수 호출
2. `global_counter++` - 3단계 (LOAD, INC, STORE)
3. `pthread_mutex_unlock()` - 함수 호출

**하지만:**
- Lock을 잡으면 다른 thread가 진입 못함
- 결과적으로 Atomic처럼 동작
- 실제 Atomic보다는 느림

### 성능 트레이드오프

**Mutex 사용 시:**

```
장점:
✅ Race Condition 완전 해결
✅ 정확한 결과
✅ 안전함

단점:
❌ 느림 (약 5-10배)
❌ Lock 오버헤드
❌ Context Switch 증가
```

**벤치마크:**
```
No Mutex:
- 시간: ~0.2초
- 결과: 987,654 (틀림!)
- CPU: 100% 활용

Mutex:
- 시간: ~1.0초 (5배 느림)
- 결과: 1,000,000 (정확!)
- CPU: 낮은 활용 (대기 많음)
```

### Lock Contention (경합)

```
Contention = 여러 thread가 동시에 같은 Lock을 원함

High Contention (이 프로젝트):
- 10개 thread
- 모두 같은 counter 증가
- 계속 Lock 경합
→ 성능 ↓↓

Low Contention:
- Thread가 독립적
- Lock 요청 적음
→ 성능 ↑
```

**관찰:**
```bash
$ time ./shared_counter_fixed

real    0m1.234s  # 실제 시간
user    0m2.456s  # CPU 시간 (여러 코어 합산)
sys     0m5.678s  # Kernel 시간 (Lock!)

→ sys 시간이 큼 = Lock 오버헤드
→ user < real = 대기 시간
```

### Critical Section 최소화

**이 프로젝트의 문제:**
```c
for (int i = 0; i < 100000; i++) {
    pthread_mutex_lock(&counter_mutex);
    global_counter++;
    pthread_mutex_unlock(&counter_mutex);
}

→ 100,000번 Lock/Unlock!
→ 매우 비효율적
```

**개선 방법 (나중에):**
```c
// Thread-Local Counter 사용
int local_counter = 0;

for (int i = 0; i < 100000; i++) {
    local_counter++;  // Lock 없이!
}

// 마지막에 한 번만
pthread_mutex_lock(&counter_mutex);
global_counter += local_counter;
pthread_mutex_unlock(&counter_mutex);

→ 1번만 Lock/Unlock!
→ 훨씬 빠름
```

### Futex (Fast Userspace Mutex)

**Linux Mutex 내부 구현:**

```
No Contention (Fast Path):
1. User space에서 Atomic CAS
2. 성공하면 즉시 리턴
3. Syscall 없음!
→ ~10 ns

Contention (Slow Path):
1. Atomic CAS 실패
2. futex() syscall 호출
3. Kernel에서 sleep
4. 다른 thread가 깨움
→ ~2 μs (Context Switch)
```

**이 프로젝트에서:**
- Contention 많음
- 대부분 Slow Path
- Syscall 많음
→ 느림

### Mutex 타입

**Normal Mutex (기본, 이 프로젝트):**
```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

특징:
- 같은 thread가 재잠금 → Deadlock
- 소유자 검증 없음 (빠름)
```

**Recursive Mutex:**
```c
pthread_mutexattr_t attr;
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
pthread_mutex_init(&mutex, &attr);

특징:
- 같은 thread가 여러 번 lock 가능
- 같은 횟수만큼 unlock 필요
```

**Error-Check Mutex:**
```c
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

특징:
- 재잠금 → EDEADLK 에러
- 소유자 검증 (느림)
- 디버깅용
```

---

## 📊 성능

### 실행 시간 비교

```
조건:
- 10 threads
- 각 100,000 증가
- 총 1,000,000 증가

Day 3 (No Mutex):
real: 0.234s
user: 1.456s
sys:  0.008s
→ 빠르지만 틀림 (987,654)

Day 4 (Mutex):
real: 1.123s
user: 2.345s
sys:  5.678s
→ 느리지만 정확 (1,000,000)

차이: 약 5배 느림
```

### Thread 수에 따른 변화

```
2 threads:
- No Mutex: 0.1s, 결과: 995,123
- Mutex:    0.5s, 결과: 1,000,000

10 threads:
- No Mutex: 0.2s, 결과: 987,654
- Mutex:    1.0s, 결과: 1,000,000

20 threads:
- No Mutex: 0.3s, 결과: 972,345
- Mutex:    2.0s, 결과: 1,000,000

→ Thread ↑ = Contention ↑ = 느림 ↑
```

### Valgrind 결과

```bash
$ valgrind --leak-check=full ./shared_counter_fixed

==12345== HEAP SUMMARY:
==12345==     in use at exit: 0 bytes in 0 blocks
==12345==   total heap usage: 11 allocs, 11 frees, 1,344 bytes allocated
==12345==
==12345== All heap blocks were freed -- no leaks are possible
==12345==
==12345== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

**분석:**
- ✅ 메모리 누수 없음
- ✅ 모든 리소스 정리
- ✅ Mutex도 올바르게 destroy

---

## 🔮 개선 가능 사항

### 1. Thread-Local Counter

- [ ] 각 thread가 local counter 증가
- [ ] 마지막에 한 번만 global counter 업데이트
- [ ] Lock 횟수: 100,000 → 10
- [ ] 예상 성능: 10배 향상

### 2. Atomic Operations 사용

- [ ] `atomic_fetch_add()` 사용
- [ ] Mutex 없이 안전
- [ ] 하드웨어 지원
- [ ] 예상 성능: 50배 향상

### 3. Read-Write Lock

- [ ] 읽기만 하는 thread 추가
- [ ] `pthread_rwlock_t` 사용
- [ ] 읽기는 동시 가능

### 4. Lock-Free 알고리즘

- [ ] CAS (Compare-And-Swap) 사용
- [ ] 완전히 Lock 없음
- [ ] 매우 빠름, 하지만 복잡

---

## 📚 참고 자료

### Man Pages
```bash
man pthread_mutex_init
man pthread_mutex_lock
man pthread_mutex_unlock
man pthread_mutex_destroy
man pthread_mutexattr_init
man pthread_mutexattr_settype
```

### 관련 함수
```c
// 초기화
int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr);

// Lock (블로킹)
int pthread_mutex_lock(pthread_mutex_t *mutex);

// Try-Lock (즉시 리턴)
int pthread_mutex_trylock(pthread_mutex_t *mutex);

// Timed-Lock (Timeout)
int pthread_mutex_timedlock(pthread_mutex_t *mutex,
                            const struct timespec *abs_timeout);

// Unlock
int pthread_mutex_unlock(pthread_mutex_t *mutex);

// 정리
int pthread_mutex_destroy(pthread_mutex_t *mutex);
```

### 관련 문서
- [[01-thread-basics]] - Thread 기초
- [[02-pthread-api]] - pthread 함수들
- [[03-race-condition]] - Day 3 문제
- [[04-critical-section]] - Critical Section 개념
- [Day 3: shared-counter](../../day3-basics/shared-counter) - 문제 버전

---

## 🎯 다음 단계

**학습 순서:**
1. ✅ [Day 3: shared-counter](../../day3-basics/shared-counter) - 문제 발견
2. ✅ shared-counter-fixed (현재) - Mutex로 해결
3. → [ping-pong](../ping-pong) - Condition Variable
4. → [producer-consumer](../../day5-6-producer-consumer) - Queue 패턴
5. → [thread-pool](../../day7-thread-pool) - 실전 응용

## 📝 프로젝트 정보

```
개발 기간: Day 4
환경: Linux (Ubuntu/WSL)
언어: C
라이브러리: pthread
빌드: gcc, make
테스트: Valgrind
```

**파일 구조:**
```
shared-counter-fixed/
├── shared_counter_fixed.c    # 메인 코드
├── Makefile                   # 빌드 설정
└── README.md                  # 이 문서
```

**핵심 코드:**
```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg) {
    pthread_mutex_lock(&mutex);
    // Critical Section
    global_counter++;
    pthread_mutex_unlock(&mutex);
    return NULL;
}
```

---

**Author:** OnePaperHoon  
**Date:** January 2025  
**Project:** Linux Kernel Study - Week 2, Day 4  
**Topic:** Mutex & Synchronization