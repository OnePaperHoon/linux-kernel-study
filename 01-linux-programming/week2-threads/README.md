# Week 2: Threads & Synchronization

## 학습 목표
- pthread API 마스터
- Race Condition 이해 및 해결
- 동기화 패턴 (Mutex, Condition Variable) 실습
- 실전 멀티스레드 패턴 구현

## 프로젝트 목록

### 📁 basics/ — Thread 기초
| 프로젝트 | 주제 | 상태 |
|----------|------|------|
| [hello-thread](./basics/hello-thread) | pthread_create / join 기초 | ✅ |
| [multi-threads](./basics/multi-threads) | 여러 Thread 관리, 반환값 | ✅ |
| [shared-counter](./basics/shared-counter) | Race Condition 관찰 | ✅ |

### 📁 sync/ — 동기화
| 프로젝트 | 주제 | 상태 |
|----------|------|------|
| [shared-counter-fixed](./sync/shared-counter-fixed) | Mutex로 Race Condition 해결 | ✅ |
| [ping-pong](./sync/ping-pong) | Condition Variable로 순서 제어 | ✅ |

### 📁 고급 패턴
| 프로젝트 | 주제 | 상태 |
|----------|------|------|
| [producer-consumer](./producer-consumer) | Ring Buffer + Condition Variable | ✅ |
| [thread-pool](./thread-pool) | Linked List Queue + Worker 관리 | ✅ |

## 학습 흐름

```
hello-thread          → Thread 기본 생성/종료
  ↓
multi-threads         → 여러 Thread, 반환값 처리
  ↓
shared-counter        → Race Condition 문제 직접 경험
  ↓
shared-counter-fixed  → Mutex로 해결
  ↓
ping-pong             → Condition Variable로 순서 제어
  ↓
producer-consumer     → 복합 패턴 (Mutex + Cond 2개)
  ↓
thread-pool           → 실전 응용 (Worker + Queue 관리)
```

## 핵심 개념 요약

### Mutex
```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_lock(&mutex);   // Critical Section 진입
// ... 공유 자원 접근 ...
pthread_mutex_unlock(&mutex); // 나올 때 해제
```

### Condition Variable
```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
// 대기
while (!condition)
    pthread_cond_wait(&cond, &mutex); // Spurious wakeup 대비 while!
// 신호
pthread_cond_signal(&cond);      // 하나 깨우기
pthread_cond_broadcast(&cond);   // 모두 깨우기
```

## 면접 예상 질문

Q: Thread와 Process의 차이는?
A: Thread는 같은 프로세스 내에서 메모리(Code, Data, Heap)를 공유하고
   Stack만 독립적입니다. 생성 비용이 약 10배 저렴합니다.

Q: Race Condition이 발생하는 조건 3가지는?
A: ① 공유 자원 존재 ② 동시 접근 ③ 최소 하나의 Write

Q: Mutex와 Semaphore의 차이는?
A: Mutex는 소유 개념이 있어 잠근 Thread만 해제 가능합니다.
   Semaphore는 카운팅 기반으로 여러 Thread가 접근 가능합니다.

## 다음 주 계획
- Week 3: IPC (Pipe, FIFO, Shared Memory, Message Queue, Signal)
