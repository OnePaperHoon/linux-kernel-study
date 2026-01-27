# Shared Counter

Race Condition을 관찰하는 프로그램 - 동기화의 필요성

## 🖥️ 실행 화면

```bash
$ ./shared_counter
Initial counter: 0
Expected final counter: 1000000

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

Final counter: 987,654
Expected: 1,000,000
Difference: 12,346
❌ Race condition detected!
```

## 🚀 빌드 및 실행

```bash
# 컴파일
make

# 실행
make test

# 여러 번 실행 (매번 다른 결과!)
$ for i in {1..5}; do ./shared_counter | grep "Final\|Difference"; done
```

## 📖 프로젝트 개요

### 목적
- Race Condition 현상 관찰
- 동기화 없는 공유 메모리 접근의 위험성
- 내일 Mutex로 해결 예정

### 학습 목표
- [x] Race Condition 이해
- [x] Critical Section 개념
- [x] Atomic Operation의 중요성
- [x] 재현 가능한 버그 경험
- [ ] Mutex로 해결 (Day 4)

## 🔧 구현 내용

### 1. 전역 변수 (공유 리소스)

```c
// 전역 변수 - 모든 thread가 공유
long global_counter = 0;
```

**메모리 레이아웃:**
```
     Data Segment
┌────────────────────┐
│ global_counter     │ ← 모든 thread가 같은 주소
└────────────────────┘
     ↑    ↑    ↑
     T0   T1   T2  (모든 thread)
```

### 2. Counter 증가

```c
void *increment_counter(void *arg)
{
    for (int i = 0; i < INCREMENTS; i++) {
        global_counter++;  // ← Race condition!
    }
    return NULL;
}
```

**문제의 코드:**
```c
global_counter++;
```

이것은 **atomic operation이 아닙니다!**

### 3. Race Condition 발생

**어셈블리 레벨 분석:**
```c
global_counter++;

// 실제로는 3단계:
1. LOAD:  레지스터 ← global_counter 값 읽기
2. INC:   레지스터 값 + 1
3. STORE: global_counter ← 레지스터 값 쓰기
```

**Race Condition 시나리오:**
```
시간    Thread 0              Thread 1              global_counter
────────────────────────────────────────────────────────────────────
T0     LOAD (0)                                       0
T1     INC  (1)                                       0
T2                            LOAD (0)                0
T3     STORE(1)                                       1
T4                            INC  (1)                1
T5                            STORE(1)                1
                                                      ↑
                                            예상: 2, 실제: 1
                                            → 1회 손실!
```

### 4. 실험 결과 분석

```bash
$ make test

Run 1:
Final counter: 987,654
Difference: 12,346

Run 2:
Final counter: 991,234
Difference: 8,766

Run 3:
Final counter: 985,123
Difference: 14,877

→ 매번 다른 결과!
→ 항상 1,000,000보다 작음
```

**왜 매번 다를까?**
- Thread 실행 순서가 매번 다름
- Context switch 시점이 매번 다름
- Race condition은 타이밍에 민감

## 💡 배운 점

### Critical Section (임계 영역)

**정의:**
```
여러 thread가 동시에 접근하면 안 되는 코드 영역
```

**예시:**
```c
// Critical Section 시작
global_counter++;
// Critical Section 끝
```

**조건:**
1. **Mutual Exclusion (상호 배제)**
   - 한 번에 하나의 thread만 진입
   
2. **Progress (진행)**
   - 아무도 안 쓰면 진입 가능
   
3. **Bounded Waiting (한정 대기)**
   - 무한히 기다리지 않음

### Atomic Operation

**Atomic이란?**
```
중간에 끊을 수 없는 연산
- 전부 실행되거나
- 전혀 실행 안되거나
```

**Non-Atomic:**
```c
global_counter++;

// 3단계로 나뉨
LOAD
INC
STORE
```

**Atomic (하드웨어 지원):**
```c
__atomic_add_fetch(&global_counter, 1, __ATOMIC_SEQ_CST);

// 또는 GCC built-in
__sync_fetch_and_add(&global_counter, 1);

// 단일 CPU 명령어로 실행
LOCK INC [memory]
```

### Race Condition 발생 조건

**3가지 조건 모두 만족 시 발생:**

1. **공유 리소스**
   ```c
   long global_counter;  // 전역 변수
   ```

2. **동시 접근**
   ```c
   // 여러 thread가 동시 실행
   pthread_create(&t1, NULL, func, NULL);
   pthread_create(&t2, NULL, func, NULL);
   ```

3. **최소 하나의 Write**
   ```c
   global_counter++;  // 수정 연산
   ```

**하나라도 없으면 안전:**
```c
// 1. 공유 안함 (각자 지역 변수)
void *func(void *arg) {
    int local_counter = 0;  // Stack, 독립적
    local_counter++;        // 안전
}

// 2. 순차 접근 (동시 실행 안함)
func1();
func2();  // func1 끝난 후 실행

// 3. Read만 (수정 없음)
printf("%ld", global_counter);  // 읽기만, 안전
```

### Context Switch의 영향

**Context Switch란?**
```
CPU가 한 thread에서 다른 thread로 전환하는 것
```

**발생 시점:**
- Time slice 소진 (보통 1-10ms)
- I/O 대기
- Sleep, yield 호출
- 우선순위 높은 thread 도착

**Race Condition과의 관계:**
```
Context Switch 없으면:
T0: counter++ × 100,000 (완료)
T1: counter++ × 100,000 (완료)
→ 200,000 (정확!)

Context Switch 있으면:
T0: counter++ × 50,000
[Switch]
T1: counter++ × 60,000
[Switch]
T0: counter++ × 50,000
[Switch]
T1: counter++ × 40,000
→ ~195,000 (손실!)
```

### 멀티코어 CPU의 영향

**싱글 코어:**
```
CPU
 ↓
T0 → T1 → T0 → T1
(번갈아 실행, Context switch)

Race condition: 발생 가능
빈도: 중간
```

**멀티 코어:**
```
CPU 0         CPU 1
  ↓            ↓
  T0          T1
(진짜 동시 실행!)

Race condition: 발생 확률 ↑
빈도: 높음
```

**실험:**
```bash
# 단일 코어로 제한
$ taskset -c 0 ./shared_counter

# 모든 코어 사용
$ ./shared_counter

→ 멀티코어에서 손실 더 많음
```

### Memory Ordering

**CPU 최적화:**
```c
// 원본 코드
global_counter++;

// CPU가 재배치 가능 (성능 향상)
temp = global_counter;  // LOAD
temp = temp + 1;        // INC
// ... 다른 명령어 ...
global_counter = temp;  // STORE (나중에)
```

**멀티코어에서 문제:**
```
CPU 0                   CPU 1
LOAD (0)               
                        LOAD (0)
INC (1)                
                        INC (1)
STORE (1)              
                        STORE (1)
→ 결과: 1 (예상: 2)
```

**해결책:**
- Memory barrier (fence)
- Mutex (Day 4)
- Atomic operations

### 디버깅의 어려움

**Heisenbug:**
```
관찰하려고 하면 사라지는 버그
```

**예시:**
```c
// 디버깅용 printf 추가
global_counter++;
printf("Counter: %ld\n", global_counter);

→ printf가 시간을 소비
→ Race condition 확률 감소
→ 버그가 안 나타남!
```

**gdb에서도:**
```bash
$ gdb ./shared_counter
(gdb) break increment_counter
(gdb) run

→ Breakpoint에서 멈춤
→ 다른 thread도 멈춤
→ Race condition 재현 안됨
```

### 손실률 계산

```
손실 = 1,000,000 - actual_counter
손실률 = 손실 / 1,000,000 × 100%

예시:
actual = 987,654
손실 = 12,346
손실률 = 1.2%
```

**손실률에 영향을 주는 요인:**
- Thread 수 ↑ → 손실률 ↑
- CPU 코어 수 ↑ → 손실률 ↑
- Context switch 빈도 ↑ → 손실률 ↑
- Critical section 길이 ↑ → 손실률 ↑

### 실험: 다양한 조건

**Thread 수 변화:**
```c
NUM_THREADS = 2:  손실률 ~0.5%
NUM_THREADS = 10: 손실률 ~1.5%
NUM_THREADS = 20: 손실률 ~3.0%
```

**증가 횟수 변화:**
```c
INCREMENTS = 1,000:     손실률 ~0.1%
INCREMENTS = 100,000:   손실률 ~1.0%
INCREMENTS = 1,000,000: 손실률 ~2.0%
```

## 📊 성능

### 실행 시간

```bash
$ time ./shared_counter

real    0m0.234s
user    0m1.456s  ← CPU 시간 (10 threads)
sys     0m0.008s
```

**분석:**
- real < user: 멀티코어 병렬 실행
- user ≈ real × threads: CPU 효율적 사용

### Race Condition 빈도

```
10회 실행 결과:

Run  | Final Counter | Loss
─────────────────────────────
1    | 987,654      | 12,346
2    | 991,234      | 8,766
3    | 985,123      | 14,877
4    | 993,456      | 6,544
5    | 982,001      | 17,999
6    | 988,765      | 11,235
7    | 990,123      | 9,877
8    | 986,543      | 13,457
9    | 989,012      | 10,988
10   | 984,567      | 15,433
─────────────────────────────
평균 | 987,848      | 12,152 (1.2%)
```

## 🔮 개선 가능 사항

- [ ] **Mutex 추가** → [Day 4](../day4-sync/shared-counter-fixed)
- [ ] Atomic operations 사용
- [ ] Read-Write Lock
- [ ] Lock-free 알고리즘
- [ ] 손실률 그래프 생성

## 📚 참고 자료

### Man Pages
```bash
man pthread_mutex_init
man pthread_mutex_lock
man __atomic_add_fetch
```

### 관련 개념
```c
// Atomic operations (C11)
#include <stdatomic.h>
atomic_long counter = 0;
atomic_fetch_add(&counter, 1);

// GCC built-in
__sync_fetch_and_add(&counter, 1);
__atomic_add_fetch(&counter, 1, __ATOMIC_SEQ_CST);

// Memory barrier
__sync_synchronize();  // Full barrier
```

### 디버깅 도구

**Helgrind (Valgrind tool):**
```bash
$ valgrind --tool=helgrind ./shared_counter

==12345== Possible data race during write
==12345==    at 0x401234: increment_counter
==12345==    by 0x402345: start_thread
==12345==  This conflicts with a previous write
==12345==    at 0x401234: increment_counter
```

**Thread Sanitizer (TSan):**
```bash
$ gcc -fsanitize=thread shared_counter.c -o shared_counter
$ ./shared_counter

WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 8 at 0x... by thread T1:
    #0 increment_counter
  Previous write of size 8 at 0x... by thread T0:
    #0 increment_counter
```

## 🎯 다음 단계

**해결 방법 학습 순서:**

1. ✅ shared-counter (현재) - 문제 인식
2. → [Day 4](../day4-sync/shared-counter-fixed) - Mutex로 해결
3. → Atomic operations
4. → Lock-free 알고리즘
5. → Performance 비교

**Day 4에서 배울 내용:**
```c
// Mutex 사용
pthread_mutex_t mutex;

pthread_mutex_lock(&mutex);
global_counter++;  // 이제 안전!
pthread_mutex_unlock(&mutex);

// 결과: 정확히 1,000,000
```

## 📝 코드 구조

```
shared-counter/
├── shared_counter.c    # Race condition 데모
├── Makefile            # 빌드 설정
└── README.md           # 이 문서
```

**메모리 구조:**
```
Global Memory
┌─────────────────┐
│ global_counter  │ ← 모든 thread 접근
└─────────────────┘
   ↑  ↑  ↑  ↑  ↑
   T0 T1 T2 ... T9

각 Thread의 Stack
┌──────────┐  ┌──────────┐  ┌──────────┐
│ local i  │  │ local i  │  │ local i  │
└──────────┘  └──────────┘  └──────────┘
    T0            T1            T2
```

**문제 발생 구조:**
```
Thread 0        Memory        Thread 1
────────        ──────        ────────
LOAD (0)    ←  [0]
INC (1)        [0]
                [0]       →  LOAD (0)
                [0]          INC (1)
STORE (1)   →  [1]
                [1]       →  STORE (1)
                ↑
            값 손실!
```

---

**Author:** OnePaperHoon  
**Date:** January 2025  
**Project:** Linux Kernel Study - Week 2, Day 3  
**Topic:** Thread Basics - Part 3 (Race Condition)  
**Status:** 🚨 문제 확인, Day 4에서 해결 예정