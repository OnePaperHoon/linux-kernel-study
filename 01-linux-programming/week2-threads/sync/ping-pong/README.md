# Ping-Pong-Pang

Condition Variable로 3개 Thread의 실행 순서를 정확히 제어하는 프로그램

## 🖥️ 실행 화면

```bash
$ ./ping_pong

=== Ping Pong Game ===
Max Count (30)
Ping (0)
    Pong (1)
            Pang (2)
Ping (3)
    Pong (4)
            Pang (5)
Ping (6)
    Pong (7)
            Pang (8)
...
Ping (27)
    Pong (28)
            Pang (29)
=== Game Over ===
Total exchanges: 30
```

## 🚀 빌드 및 실행

```bash
# 빌드
make

# 실행
./ping_pong

# 테스트
make test
```

## 📖 프로젝트 개요

### 목적
Condition Variable의 `wait` / `broadcast`를 활용하여 3개 Thread가 정해진 순서(Ping → Pong → Pang)로 실행되도록 제어

### 학습 목표
- [x] Condition Variable 기본 사용법
- [x] turn 변수로 순서 제어
- [x] Spurious Wakeup 대비 (`while` 루프)
- [x] 단일 Condition Variable로 다중 Thread 제어
- [x] 종료 조건 처리

## 🔧 구현 내용

### 1. 공유 상태

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

int turn  = 0;         // 현재 차례 (0=Ping, 1=Pong, 2=Pang)
int count = 0;         // 총 실행 횟수
#define MAX_COUNT 30   // 종료 조건
```

**turn 순환 구조:**
```
turn = (turn + 1) % 3

0 → 1 → 2 → 0 → 1 → 2 → ...
↑   ↑   ↑
Ping Pong Pang
```

### 2. 각 Thread 구조 (Ping 예시)

```c
void *ping_thread(void *arg)
{
    while (count < MAX_COUNT)
    {
        pthread_mutex_lock(&mutex);

        /* 내 차례가 아니면 대기 (turn != 0) */
        while (turn != 0 && count < MAX_COUNT)
            pthread_cond_wait(&cond, &mutex);

        /* 종료 확인 */
        if (count >= MAX_COUNT)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        /* 실행 */
        printf("Ping (%d)\n", count);
        count++;
        turn = (turn + 1) % 3; // Pong 차례로 넘김

        /* 모든 Thread 깨우기 */
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
```

**3개 Thread 비교:**
```
ping_thread:  while (turn != 0 ...) 대기 → turn = 1로 넘김
pong_thread:  while (turn != 1 ...) 대기 → turn = 2로 넘김
pang_thread:  while (turn != 2 ...) 대기 → turn = 0으로 넘김
```

### 3. 실행 흐름

```
시간   turn   Ping                 Pong                 Pang
────────────────────────────────────────────────────────────
T0      0    lock → 내 차례!       lock → 대기           lock → 대기
T1      0    "Ping(0)", turn=1
             broadcast
T2      1    unlock → 대기         깨어남: 내 차례!
T3      1                          "Pong(1)", turn=2
                                   broadcast
T4      2    대기(turn!=0)          unlock → 대기         깨어남: 내 차례!
T5      2                                               "Pang(2)", turn=0
                                                        broadcast
T6      0    깨어남: 내 차례!       대기(turn!=1)         unlock → 대기
...
```

### 4. broadcast vs signal

```c
/* 이 프로젝트: broadcast 사용 */
pthread_cond_broadcast(&cond);

/* signal을 쓰면 안 되는 이유: */
pthread_cond_signal(&cond);
// → 대기 중인 Thread 중 하나만 깨움
// → Pong이 대기 중인데 Pang이 깨어날 수 있음
// → Pang은 turn != 2이므로 다시 대기 → 아무도 실행 안 함 → Deadlock!

/* broadcast는 모두 깨워서 각자 조건 확인하게 함 */
```

## 💡 배운 점

### Condition Variable 핵심 패턴

```c
/* 대기 패턴 */
pthread_mutex_lock(&mutex);
while (!my_condition)           // ← if가 아닌 while! (Spurious wakeup 대비)
    pthread_cond_wait(&cond, &mutex);
// ... 작업 수행 ...
pthread_mutex_unlock(&mutex);

/* 신호 패턴 */
pthread_mutex_lock(&mutex);
// ... 상태 변경 ...
pthread_cond_broadcast(&cond);  // 또는 signal
pthread_mutex_unlock(&mutex);
```

### Spurious Wakeup

**정의:**
```
pthread_cond_wait()가 signal/broadcast 없이도 깨어나는 현상
Linux POSIX 구현의 특성
```

**이 프로젝트에서 if 쓰면?**
```c
// ❌ if 사용 시
if (turn != 0)
    pthread_cond_wait(&cond, &mutex);
// Spurious wakeup → turn 여전히 != 0 → 잘못 실행!

// ✅ while 사용 시
while (turn != 0 && count < MAX_COUNT)
    pthread_cond_wait(&cond, &mutex);
// 깨어나면 반드시 조건 재확인!
```

### 단일 Condition Variable로 다중 Thread 제어

```
하나의 cond로 3개 Thread를 제어하는 방법:
→ turn 변수로 "누구 차례인지" 구분
→ broadcast로 모두 깨우고
→ 각자 while 조건으로 자신의 차례인지 확인

vs 2개 이상의 Condition Variable:
→ producer-consumer: not_full, not_empty 분리
→ 불필요한 wakeup 감소 가능
→ 하지만 이 패턴은 단순히 turn 확인이라 1개로 충분
```

### 종료 조건의 중요성

```c
/* 대기 중에도 count 확인 */
while (turn != 0 && count < MAX_COUNT)
    pthread_cond_wait(&cond, &mutex);

/* 깨어난 후에도 확인 */
if (count >= MAX_COUNT)
{
    pthread_mutex_unlock(&mutex);
    break;
}
```

**왜 두 곳에서 확인?**
```
- while: Spurious wakeup 대비 + count 한계 도달 시 탈출
- if: 대기 탈출 후, 정말 종료 조건인지 재확인
  (내 차례가 아니라 종료 때문에 깨어난 경우)
```

## 📊 성능

```
Thread 수:   3 (Ping, Pong, Pang)
MAX_COUNT:   30
순서 보장:   100% (매번 동일한 출력)

실행 시간: < 1ms (sleep 없음)
Context Switch: MAX_COUNT × 3 = 90회
```

**순서 일관성 확인:**
```bash
$ for i in {1..5}; do ./ping_pong | head -6; echo "---"; done
Ping (0)
    Pong (1)
            Pang (2)
Ping (3)
    Pong (4)
            Pang (5)
---
(매번 동일!)
```

## 🔮 개선 가능 사항

- [ ] N개 Thread로 일반화 (turn % N)
- [ ] MAX_COUNT를 인자로 받기
- [ ] signal 사용으로 최적화 (각 Thread마다 별도 cond)
- [ ] 실행 간격 조절 (usleep 추가)

## 📚 참고 자료

### Man Pages
```bash
man pthread_cond_wait
man pthread_cond_broadcast
man pthread_cond_signal
```

### 관련 프로젝트
- [shared-counter-fixed](../shared-counter-fixed) - Mutex 기초
- [producer-consumer](../../producer-consumer) - Cond 2개 활용

## 🎯 다음 단계

1. ✅ [shared-counter-fixed](../shared-counter-fixed) - Mutex
2. ✅ ping-pong (현재) - Condition Variable
3. → [producer-consumer](../../producer-consumer) - Cond + Ring Buffer
4. → [thread-pool](../../thread-pool) - 실전 패턴

## 📝 프로젝트 정보

```
개발 기간: Day 4 (sync)
환경: Linux (Debian/Ubuntu)
언어: C
라이브러리: pthread
빌드: gcc -pthread
```

**파일 구조:**
```
ping-pong/
├── ping_pong.c    # Ping/Pong/Pang Thread 구현
└── Makefile       # 빌드 설정
```

**핵심 코드:**
```c
/* 공유 상태 */
int turn = 0; // 0=Ping, 1=Pong, 2=Pang

/* 각 Thread */
while (turn != MY_TURN && count < MAX_COUNT)
    pthread_cond_wait(&cond, &mutex);

count++;
turn = (turn + 1) % 3;
pthread_cond_broadcast(&cond);
```

---

**Author:** OnePaperHoon
**Date:** February 2026
**Project:** Linux Kernel Study - Week 2, Day 4 (Sync)
**Topic:** Condition Variable — Turn-based Thread Ordering
