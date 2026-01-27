# Multi Threads

여러 Thread를 생성하고 관리하는 프로그램

## 🖥️ 실행 화면

```bash
$ ./multi_threads
Main: Creating 5 threads
Main: Creating thread 0
Main: Creating thread 1
Main: Creating thread 2
Main: Creating thread 3
Main: Creating thread 4
Main: All threads created
Main: Waiting for threads to finish...
Thread 0: Starting
Thread 1: Starting
Thread 2: Starting
Thread 3: Starting
Thread 4: Starting
Thread 0: Finished after 0 seconds
Main: Thread 0 returned 0
Thread 1: Finished after 1 seconds
Main: Thread 1 returned 100
Thread 2: Finished after 2 seconds
Main: Thread 2 returned 200
Thread 3: Finished after 3 seconds
Main: Thread 3 returned 300
Thread 4: Finished after 4 seconds
Main: Thread 4 returned 400
Main: All threads finished
```

## 🚀 빌드 및 실행

```bash
# 컴파일
make

# 실행
make test

# Thread 수 변경 후 테스트
# multi_threads.c에서 NUM_THREADS 수정
```

## 📖 프로젝트 개요

### 목적
- 여러 Thread 동시 관리
- Thread 배열 사용법
- Thread 실행 순서 이해
- Join 순서의 중요성

### 학습 목표
- [x] Thread 배열 관리
- [x] 반복문으로 Thread 생성
- [x] Thread 실행 순서 관찰
- [x] 모든 Thread 대기
- [x] 반환값 수집

## 🔧 구현 내용

### 1. Thread 배열

```c
#define NUM_THREADS 5

pthread_t threads[NUM_THREADS];  // Thread ID 배열
```

**배열 사용 이유:**
- 여러 Thread를 체계적으로 관리
- 반복문으로 일괄 처리 가능
- Join 순서 제어 가능

### 2. 반복문으로 Thread 생성

```c
// Thread 생성 루프
for (i = 0; i < NUM_THREADS; i++) {
    if (pthread_create(&threads[i], NULL, thread_function, 
                      (void *)(long)i) != 0) {
        perror("pthread_create");
        return 1;
    }
}
```

**인자 전달:**
```c
// int → void* 캐스팅
pthread_create(&threads[i], NULL, func, (void *)(long)i);

// Thread 함수에서
void *func(void *arg) {
    long tid = (long)arg;  // void* → long
}
```

**주의사항:**
```c
// ❌ 잘못된 방법
for (i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, func, &i);
    // &i는 모든 thread가 같은 주소!
    // i 값이 변하면 모든 thread가 영향받음
}

// ✅ 올바른 방법
for (i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, func, (void *)(long)i);
    // 값 자체를 전달
}
```

### 3. Join 순서

```c
// 생성 순서대로 Join
for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], &result);
    printf("Thread %d returned %ld\n", i, (long)result);
}
```

**Join 순서의 영향:**

**순서대로 Join (현재 코드):**
```
Thread 0 종료 대기
   ↓ (대기)
Thread 0 종료 → 다음
   ↓
Thread 1 종료 대기
   ↓ (대기)
Thread 1 종료 → 다음
```

**무순서 Join (대안):**
```c
// 어떤 thread든 먼저 끝나면 처리
// → pthread_join은 순서 없이 대기 불가
// → 대안: 조건 변수 사용 (Day 4)
```

### 4. Thread 실행 순서

```
생성 순서 ≠ 실행 순서 ≠ 종료 순서
```

**생성 순서:**
```
Main: Thread 0 생성
Main: Thread 1 생성
Main: Thread 2 생성
...
```

**실행 순서 (스케줄러가 결정):**
```
가능한 경우 1:
Thread 0 시작
Thread 1 시작
Thread 2 시작

가능한 경우 2:
Thread 2 시작
Thread 0 시작
Thread 4 시작

→ 순서 보장 없음!
```

**종료 순서 (작업 시간에 따라):**
```c
sleep(tid);  // Thread 0: 0초, Thread 4: 4초

Thread 0 먼저 종료
Thread 1 그 다음
...
Thread 4 마지막
```

### 5. 반환값 처리

```c
void *result;

for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], &result);
    
    // void* → long 변환
    long ret_val = (long)result;
    printf("Thread %d returned %ld\n", i, ret_val);
}
```

**반환값 활용:**
```c
// 성공/실패 코드
return (void *)0;   // 성공
return (void *)-1;  // 실패

// 계산 결과
return (void *)(long)sum;

// 포인터 (Heap)
int *result = malloc(sizeof(int));
*result = 42;
return result;
```

## 💡 배운 점

### Thread 생성 패턴

**패턴 1: 즉시 생성, 나중에 Join**
```c
// 모든 thread 생성
for (i = 0; i < N; i++)
    pthread_create(&threads[i], ...);

// 모든 thread 대기
for (i = 0; i < N; i++)
    pthread_join(threads[i], ...);

장점: 모든 thread가 병렬로 실행
단점: N개의 thread가 동시 실행 (리소스 많이 사용)
```

**패턴 2: 생성 후 즉시 Join**
```c
// 생성과 대기를 번갈아
for (i = 0; i < N; i++) {
    pthread_create(&threads[i], ...);
    pthread_join(threads[i], ...);  // 즉시 대기
}

장점: 리소스 절약 (한 번에 1개만)
단점: 순차 실행 (병렬성 없음)
```

**패턴 3: Thread Pool (나중에 학습)**
```c
// 고정 개수의 worker thread
// Job queue로 작업 분배
// → Day 7에서 구현
```

### 스케줄러와 Thread 실행

**Linux CFS (Completely Fair Scheduler):**
```
모든 thread에게 공평한 CPU 시간 할당

가상 런타임 (vruntime):
- 각 thread의 누적 실행 시간
- vruntime이 작은 thread 우선 실행

Red-Black Tree:
- vruntime으로 정렬
- O(log n) 스케줄링
```

**실제 실행 예시:**
```
CPU Timeline:
|--T0--|--T1--|--T2--|--T0--|--T1--|--T3--|--T2--|

설명:
- T0, T1, T2가 거의 동시 생성
- CPU 시간을 번갈아 할당
- Context switch 발생
```

**Context Switch:**
```
Thread A 실행
   ↓
1. 레지스터 → 메모리 저장
2. Thread B의 레지스터 ← 메모리 복원
3. Thread B 실행 재개

비용: ~2 μs (마이크로초)
```

### Thread 생성 비용

```c
// 벤치마크
struct timeval start, end;

gettimeofday(&start, NULL);
for (i = 0; i < 1000; i++) {
    pthread_create(&t, NULL, dummy, NULL);
    pthread_join(t, NULL);
}
gettimeofday(&end, NULL);

// 결과: ~50 μs per thread
```

**비용 구성:**
```
1. 스택 할당: ~30 μs
2. task_struct 생성: ~10 μs
3. 스케줄러 등록: ~5 μs
4. 기타: ~5 μs
──────────────────────
총: ~50 μs
```

### Thread 수의 최적값

**CPU-bound 작업:**
```
최적 Thread 수 ≈ CPU 코어 수

이유:
- Thread > 코어 → Context switch 오버헤드
- Thread < 코어 → CPU 낭비
```

**I/O-bound 작업:**
```
최적 Thread 수 > CPU 코어 수

이유:
- I/O 대기 중에 다른 thread 실행
- Context switch보다 I/O 대기가 훨씬 김
```

**예시:**
```c
// CPU-bound (계산)
for (i = 0; i < num_cores; i++)
    pthread_create(...);

// I/O-bound (네트워크, 파일)
for (i = 0; i < num_cores * 2; i++)
    pthread_create(...);
```

### Thread ID vs Array Index

```c
pthread_t threads[5];  // Array index: 0-4

for (i = 0; i < 5; i++) {
    pthread_create(&threads[i], NULL, func, (void *)(long)i);
}
```

**Thread ID (pthread_t):**
- OS가 할당
- 큰 숫자 (예: 140123456789)
- 고유하지만 재사용 가능

**Array Index:**
- 프로그램이 관리
- 작은 숫자 (0, 1, 2, ...)
- 논리적 구분용

```c
void *func(void *arg) {
    long my_index = (long)arg;        // Array index
    pthread_t my_id = pthread_self(); // Thread ID
    
    printf("Index: %ld, ID: %lu\n", my_index, 
           (unsigned long)my_id);
}
```

### 메모리 오버헤드

```
1개 Thread:
- task_struct: ~8 KB
- Kernel stack: ~8 KB
- User stack: 8 MB (기본)
──────────────────────
총: ~8 MB

10개 Thread: ~80 MB
100개 Thread: ~800 MB

→ 무분별한 thread 생성 주의!
```

### Thread 생성 실패 처리

```c
for (i = 0; i < NUM_THREADS; i++) {
    int ret = pthread_create(&threads[i], NULL, func, 
                            (void *)(long)i);
    if (ret != 0) {
        fprintf(stderr, "pthread_create failed: %s\n", 
                strerror(ret));
        
        // 이미 생성된 thread들 정리
        for (int j = 0; j < i; j++) {
            pthread_cancel(threads[j]);
            pthread_join(threads[j], NULL);
        }
        
        return 1;
    }
}
```

**실패 원인:**
- 메모리 부족 (`EAGAIN`)
- Thread 수 제한 초과 (`EAGAIN`)
- 권한 부족 (`EPERM`)

**시스템 제한 확인:**
```bash
# Thread 수 제한
$ ulimit -u
15752

# Stack 크기 제한
$ ulimit -s
8192  # KB
```

## 📊 성능

### 실행 시간 측정

```c
#include <sys/time.h>

struct timeval start, end;

gettimeofday(&start, NULL);

// Thread 생성 & Join
for (i = 0; i < NUM_THREADS; i++)
    pthread_create(&threads[i], ...);
for (i = 0; i < NUM_THREADS; i++)
    pthread_join(threads[i], ...);

gettimeofday(&end, NULL);

long elapsed = (end.tv_sec - start.tv_sec) * 1000000 +
               (end.tv_usec - start.tv_usec);
printf("Time: %ld us\n", elapsed);
```

### 벤치마크 결과

```
Thread 수    생성 시간    총 시간
──────────────────────────────────
1            50 μs      50 μs
5            250 μs     4초 (sleep 때문)
10           500 μs     9초
100          5 ms       99초
1000         50 ms      999초
```

## 🔮 개선 가능 사항

- [ ] 에러 처리 강화 (부분 생성 실패)
- [ ] Thread 속성 설정 (stack 크기)
- [ ] 동적 Thread 수 (실행 시 입력)
- [ ] Thread 상태 추적
- [ ] CPU 코어 수에 맞춰 자동 조정
- [ ] Thread 풀 패턴 → [Day 7](../day7-thread-pool)

## 📚 참고 자료

### Man Pages
```bash
man pthread_create
man pthread_join
man pthread_cancel
man pthread_attr_setstacksize
```

### 시스템 제한 확인
```bash
# Thread 관련 제한
cat /proc/sys/kernel/threads-max

# User별 프로세스 제한
ulimit -u

# Stack 크기
ulimit -s
```

### 관련 함수
```c
// Thread 취소
int pthread_cancel(pthread_t thread);

// 속성 설정
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_destroy(pthread_attr_t *attr);
```

## 🎯 다음 단계

**학습 순서:**
1. ✅ [hello-thread](../hello-thread)
2. ✅ multi-threads (현재)
3. → [shared-counter](../shared-counter) - Race condition 관찰
4. → Day 4: Mutex로 동기화

## 📝 코드 구조

```
multi-threads/
├── multi_threads.c    # 메인 코드
├── Makefile           # 빌드 설정
└── README.md          # 이 문서
```

**실행 흐름:**
```
Main Thread:
  ├─ Thread 0 생성 ──→ 병렬 실행
  ├─ Thread 1 생성 ──→ 병렬 실행
  ├─ Thread 2 생성 ──→ 병렬 실행
  ├─ Thread 3 생성 ──→ 병렬 실행
  └─ Thread 4 생성 ──→ 병렬 실행
  
Main Thread:
  ├─ Thread 0 대기 (가장 먼저 종료)
  ├─ Thread 1 대기
  ├─ Thread 2 대기
  ├─ Thread 3 대기
  └─ Thread 4 대기 (가장 나중에 종료)
```

---

**Author:** OnepaperHoon\
**Date:** January 2025  
**Project:** Linux Kernel Study - Week 2, Day 3  
**Topic:** Thread Basics - Part 2