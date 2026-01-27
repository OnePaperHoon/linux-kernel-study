# Hello Thread

가장 기본적인 pthread 프로그램 - Thread 생성과 종료

## 🖥️ 실행 화면

```bash
$ ./hello_thread
Main: Creating thread
Main: Thread created with ID 140123456789
Main: Waiting for thread to finish
Hello from thread 42!
Thread 42: My pthread_t is 140123456789
Thread 42: Exiting
Main: Thread returned 42
Main: Done
```

## 🚀 빌드 및 실행

```bash
# 컴파일
make

# 실행
make test

# 또는
./hello_thread
```

## 📖 프로젝트 개요

### 목적
- pthread API 기초 학습
- Thread 생성과 종료 흐름 이해
- 가장 단순한 멀티스레딩 프로그램

### 학습 목표
- [x] pthread_create 사용법
- [x] pthread_join 사용법
- [x] Thread 함수 작성법
- [x] 인자 전달 방법

## 🔧 구현 내용

### 1. Thread 생성: pthread_create()

```c
pthread_t thread;

int pthread_create(
    pthread_t *thread,              // Thread ID 저장할 변수
    const pthread_attr_t *attr,     // 속성 (NULL = 기본값)
    void *(*start_routine)(void *), // Thread가 실행할 함수
    void *arg                       // 함수에 전달할 인자
);
```

**예제:**
```c
// Thread 생성
if (pthread_create(&thread, NULL, thread_function, (void *)42) != 0) {
    perror("pthread_create");
    return 1;
}
```

**주요 파라미터:**
- `thread`: Thread ID가 저장될 변수의 포인터
- `attr`: NULL이면 기본 속성 사용
  - 스택 크기: 보통 8MB
  - 스케줄링 정책: 상속
  - 분리 상태: Joinable
- `start_routine`: Thread가 실행할 함수
  - 반환 타입: `void *`
  - 파라미터: `void *`
- `arg`: Thread 함수에 전달할 인자

**반환값:**
- 성공: 0
- 실패: 에러 번호 (errno가 아님!)

### 2. Thread 함수

```c
void *thread_function(void *arg)
{
    long thread_id = (long)arg;  // void* → long 캐스팅
    
    // Thread 작업 수행
    printf("Hello from thread %ld!\n", thread_id);
    
    // 반환값 (void* 타입)
    return (void *)thread_id;
}
```

**Thread 함수 규칙:**
- 반환 타입: `void *`
- 파라미터: `void *` 하나만
- 모든 타입을 `void *`로 캐스팅해서 전달/반환

**인자 전달 방법:**

```c
// 정수 전달
pthread_create(&t, NULL, func, (void *)42);

// 포인터 전달
int *ptr = malloc(sizeof(int));
*ptr = 100;
pthread_create(&t, NULL, func, ptr);

// 구조체 전달
struct data {
    int id;
    char name[50];
};
struct data *d = malloc(sizeof(struct data));
pthread_create(&t, NULL, func, d);
```

### 3. Thread 대기: pthread_join()

```c
void *result;

int pthread_join(
    pthread_t thread,    // 대기할 Thread ID
    void **retval        // 반환값을 저장할 포인터 (NULL 가능)
);
```

**예제:**
```c
void *result;

// Thread 종료 대기
if (pthread_join(thread, &result) != 0) {
    perror("pthread_join");
    return 1;
}

printf("Thread returned %ld\n", (long)result);
```

**동작:**
- 호출한 thread는 **블로킹**됨
- 대상 thread가 종료될 때까지 대기
- Thread의 반환값을 가져옴
- Thread 리소스 정리 (detach 상태가 아닌 경우 필수!)

**주의사항:**
- Join하지 않으면 → 좀비 thread (메모리 누수)
- 같은 thread를 두 번 join → Undefined behavior
- Detached thread를 join → 에러

### 4. Thread ID 확인

```c
// Thread 자신의 ID 확인
pthread_t self_id = pthread_self();

printf("My ID: %lu\n", (unsigned long)pthread_self());
```

**pthread_t 타입:**
- 구현마다 다름 (Linux: unsigned long)
- 직접 비교 대신 `pthread_equal()` 사용 권장
- 출력 시 캐스팅 필요

### 5. 컴파일 옵션

```bash
# pthread 라이브러리 링크 필수!
gcc -pthread hello_thread.c -o hello_thread

# 또는
gcc -lpthread hello_thread.c -o hello_thread
```

**-pthread vs -lpthread:**
- `-pthread`: 컴파일러 옵션 + 링커 옵션 모두 설정 (권장)
- `-lpthread`: 링커 옵션만 설정

## 💡 배운 점

### Thread vs Process

**Thread가 Process보다 가벼운 이유:**

```
Process 생성 (fork):
1. 새 프로세스 생성
2. 메모리 공간 복사 (COW)
3. 페이지 테이블 생성
4. 파일 디스크립터 복사
→ 비용: 높음

Thread 생성 (pthread_create):
1. 같은 프로세스 내에서 실행 단위 추가
2. Stack만 새로 할당
3. 나머지는 공유
→ 비용: 낮음 (약 10배 빠름)
```

**메모리 공유:**
```
Process:                    Thread:
┌──────────┐               ┌──────────┐
│  Code    │ 복사           │  Code    │ 공유
│  Data    │ 복사           │  Data    │ 공유
│  Heap    │ 복사           │  Heap    │ 공유
│  Stack   │ 독립           ├──────────┤
└──────────┘               │ Stack T1 │ 독립
                           │ Stack T2 │ 독립
                           └──────────┘
```

### pthread_create 내부 동작

```
User Space                  Kernel Space
──────────                  ────────────

pthread_create()
    ↓
1. 스택 할당 (기본 8MB)
    ↓
2. Thread 구조체 준비
    ↓
3. clone() syscall ────→ sys_clone()
                            ↓
                         task_struct 생성
                         (CLONE_VM | CLONE_FILES | ...)
                            ↓
                         Scheduler 등록
                            ↓
pthread_create 리턴 ←── Thread 실행 시작
```

**clone() flags:**
```c
CLONE_VM        // 메모리 공유
CLONE_FS        // 파일시스템 정보 공유
CLONE_FILES     // 파일 디스크립터 공유
CLONE_SIGHAND   // 시그널 핸들러 공유
CLONE_THREAD    // 같은 thread 그룹
```

### pthread_join의 역할

**Thread Lifecycle:**
```
생성         실행          종료
 ↓           ↓            ↓
NEW → READY → RUNNING → TERMINATED
                            ↓
                        (join 대기)
                            ↓
                         리소스 정리
```

**Join하지 않으면:**
```c
pthread_create(&thread, NULL, func, NULL);
// join 안함 → Thread 종료해도 리소스 정리 안됨
// → 좀비 thread (메모리 누수)
```

**Detached thread:**
```c
// 방법 1: 생성 시
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
pthread_create(&thread, &attr, func, NULL);
// join 불필요, 자동 정리

// 방법 2: 생성 후
pthread_detach(thread);
```

### void* 캐스팅

**정수 전달 시 주의:**
```c
// ❌ 위험 (64bit 시스템에서 문제)
int value = 42;
pthread_create(&t, NULL, func, &value);
// value가 stack에 있고, main이 끝나면 사라짐!

// ✅ 안전
pthread_create(&t, NULL, func, (void *)42);
// 값 자체를 전달 (포인터가 아님)

// Thread 함수에서
void *func(void *arg) {
    long value = (long)arg;  // 값 복원
}
```

**포인터 전달 시:**
```c
// ✅ Heap 사용
int *ptr = malloc(sizeof(int));
*ptr = 42;
pthread_create(&t, NULL, func, ptr);

// Thread에서
void *func(void *arg) {
    int *ptr = (int *)arg;
    printf("%d\n", *ptr);
    free(ptr);  // 사용 후 해제
}
```

### Thread 실행 순서

```c
pthread_create(&t1, NULL, func1, NULL);
printf("After create\n");  // 언제 실행?
```

**Thread는 비동기적으로 실행:**
- `pthread_create()` 리턴 != Thread 시작 완료
- OS 스케줄러가 결정
- 실행 순서 보장 없음!

```
가능한 순서 1:
Main: pthread_create 호출
Main: "After create" 출력
Thread: 실행 시작

가능한 순서 2:
Main: pthread_create 호출
Thread: 실행 시작 (즉시!)
Thread: 작업 수행
Main: "After create" 출력
```

### 컴파일 시 -pthread의 중요성

```bash
# ❌ 링크 에러
gcc hello_thread.c -o hello_thread
# undefined reference to 'pthread_create'

# ✅ 정상
gcc -pthread hello_thread.c -o hello_thread
```

**-pthread가 하는 일:**
1. 매크로 정의 (`_REENTRANT`)
2. pthread 라이브러리 링크 (`-lpthread`)
3. Thread-safe 코드 생성

## 📊 성능

```
Thread 생성 시간: ~50 μs
메모리 오버헤드: ~8MB (stack)
Context switch: ~2 μs
```

**vs Process:**
```
Process 생성 (fork): ~500 μs (10배 느림)
메모리 오버헤드: 수십 MB
Context switch: ~10 μs (5배 느림)
```

## 🔮 개선 가능 사항

- [ ] 여러 thread 생성 → [multi-threads](../multi-threads)
- [ ] 반환값 활용
- [ ] 에러 처리 강화
- [ ] Thread 속성 설정 (stack 크기 등)
- [ ] Detached thread 예제

## 📚 참고 자료

### Man Pages
```bash
man pthread_create
man pthread_join
man pthread_self
man pthread_detach
man pthread_attr_init
```

### 함수 원형
```c
#include <pthread.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
pthread_t pthread_self(void);
int pthread_detach(pthread_t thread);
int pthread_equal(pthread_t t1, pthread_t t2);
void pthread_exit(void *retval);
```

### 관련 문서
- TLPI Chapter 29: Threads - Introduction
- POSIX Threads Programming: https://computing.llnl.gov/tutorials/pthreads/
- Linux man-pages: https://man7.org/linux/man-pages/man3/pthread_create.3.html

## 🎯 다음 단계

**학습 순서:**
1. ✅ hello-thread (현재)
2. → [multi-threads](../multi-threads) - 여러 thread 관리
3. → [shared-counter](../shared-counter) - Race condition
4. → Day 4: Mutex로 동기화

## 📝 코드 구조

```
hello-thread/
├── hello_thread.c    # 메인 코드
├── Makefile          # 빌드 설정
└── README.md         # 이 문서
```

**핵심 흐름:**
```c
main()
  ↓
pthread_create() → thread_function() 실행 시작
  ↓                     ↓
printf()            printf() (병렬 실행)
  ↓                     ↓
pthread_join()      return (종료)
  ↓
printf() (thread 종료 후)
```

---

**Author:** OnepaperHoon
**Date:** January 2025  
**Project:** Linux Kernel Study - Week 2, Day 3  
**Topic:** Thread Basics - Part 1