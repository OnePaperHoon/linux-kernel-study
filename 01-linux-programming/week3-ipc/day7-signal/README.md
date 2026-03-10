# Signal

비동기 이벤트 알림 메커니즘 — 커널이 프로세스에 전달하는 소프트웨어 인터럽트

## 🖥️ 실행 화면

### signal_basic

```bash
$ ./signal_basic

=== Signal Basic Example ===
[Main] PID: 12345

[setup] SIGINT  registered via sigaction()
[setup] SIGALRM registered via sigaction()
[setup] SIGUSR1 registered via signal()

[Main] Running... (Ctrl+C 3번으로 종료)
[Main] 다른 터미널에서: kill -USR1 12345

[SIGALRM] Timer expired!
[Main] SIGALRM 총 1회 발생 (3초마다)
^C
[SIGINT] Ctrl+C received!
[Main] SIGINT 1/3 수신 (앞으로 2번 더)
[SIGUSR1] User-defined signal received!
[Main] SIGUSR1 총 1회 수신
[SIGALRM] Timer expired!
[Main] SIGALRM 총 2회 발생 (3초마다)
^C
[SIGINT] Ctrl+C received!
[Main] SIGINT 2/3 수신 (앞으로 1번 더)
^C
[SIGINT] Ctrl+C received!
[SIGINT] 3번 수신 → 프로그램 종료

[Main] Exiting (SIGINT 3회, SIGUSR1 1회, SIGALRM 2회)
```

### signal_parent_child

```bash
$ ./signal_parent_child

=== Signal Parent-Child Communication ===
    부모 ──SIGUSR1──▶ 자식
    자식 ──SIGUSR2──▶ 부모

[Child]  Started (PID: 12346, Parent PID: 12345)
[Child]  Round 1: Waiting for SIGUSR1...
[Parent] Started (PID: 12345, Child PID: 12346)
[Parent] Round 1: Sending SIGUSR1 to child
[Parent] Round 1: Waiting for SIGUSR2...
[Child]  Round 1: SIGUSR1 received! Working...
[Child]  Round 1: Sending SIGUSR2 to parent
[Child]  Round 2: Waiting for SIGUSR1...
[Parent] Round 1: SIGUSR2 received! ✓

[Parent] Round 2: Sending SIGUSR1 to child
...
[Parent] All rounds complete. Sending SIGTERM to child
[Child]  SIGTERM received. Exiting cleanly.
[Parent] Child exited. Done.
```

## 🚀 빌드 및 실행

```bash
# 전체 빌드
make

# signal_basic 실행 (인터랙티브)
./signal_basic
# → Ctrl+C 3번, 또는 다른 터미널에서 kill -USR1 <PID>

# signal_parent_child 실행
make test-pc

# 메모리 검사
make valgrind
```

## 📖 프로젝트 개요

### 목적
`sigaction()` 기반 시그널 핸들링과 `kill()`을 이용한 프로세스 간 시그널 통신 구현

### 학습 목표
- [x] `signal()` vs `sigaction()` 차이 이해
- [x] `volatile sig_atomic_t` 로 핸들러-main 간 안전한 데이터 공유
- [x] `async-signal-safe` 함수와 그렇지 않은 함수 구분
- [x] `pause()` 로 CPU 낭비 없는 시그널 대기
- [x] `kill()` + `SIGUSR1/SIGUSR2` 로 프로세스 간 통신
- [x] `alarm()` + `SIGALRM` 타이머

### 주요 시그널

```
SIGINT  (2)  : Ctrl+C → 프로세스 인터럽트 (기본: 종료)
SIGTERM (15) : kill <pid> 기본 → 정상 종료 요청
SIGKILL (9)  : kill -9 → 강제 종료 (핸들링/무시 불가)
SIGUSR1 (30) : 사용자 정의 1
SIGUSR2 (31) : 사용자 정의 2
SIGALRM (14) : alarm() 타이머 만료
SIGCHLD (20) : 자식 프로세스 상태 변경 (종료/중지)
SIGSEGV (11) : 잘못된 메모리 접근 (기본: 코어 덤프)
SIGPIPE (13) : 수신자 없는 파이프에 쓰기 (기본: 종료)
```

## 🔧 구현 내용

### 1. signal() vs sigaction()

```c
/* ❌ signal() - 이식성 낮음 */
signal(SIGINT, handler);
/* 문제점:
 *   - 핸들러 실행 중 같은 시그널 블로킹 여부: 플랫폼마다 다름
 *   - 핸들러 실행 후 SIG_DFL로 리셋 여부: 플랫폼마다 다름
 *   - POSIX에서 동작 "implementation-defined"
 */

/* ✅ sigaction() - POSIX 표준, 정밀 제어 */
struct sigaction sa;
sa.sa_handler = handler;
sigemptyset(&sa.sa_mask);       /* 핸들러 실행 중 추가 블로킹 없음 */
sa.sa_flags = SA_RESTART;       /* 인터럽트된 syscall 자동 재시작 */
sigaction(SIGINT, &sa, NULL);
```

### 2. 핸들러에서 안전한 변수 타입

```c
/* volatile sig_atomic_t:
 *   volatile    → 컴파일러가 레지스터 캐싱 안 함 (항상 메모리에서 읽기)
 *   sig_atomic_t → 읽기/쓰기가 원자적으로 보장되는 정수형
 */
static volatile sig_atomic_t g_flag = 0;

static void handler(int sig) {
    g_flag = 1;     /* ✅ 안전 */
    (void)sig;
}
```

### 3. async-signal-safe 함수

```
✅ 핸들러에서 사용 가능 (async-signal-safe):
  write(), read(), _exit(), kill(), signal(), alarm()
  send(), recv(), open(), close(), fork()

❌ 핸들러에서 사용 불가:
  printf()  → 내부에서 mutex 사용 → 데드락 가능
  malloc()  → 내부 상태 변경 중 인터럽트 시 corruption
  exit()    → atexit 핸들러 실행 → 비안전
```

```c
/* ✅ 올바른 방식: 핸들러에서는 flag만 설정, 출력은 main에서 */
static void handler(int sig) {
    g_flag = 1;                         /* flag 설정 */
    write(STDOUT_FILENO, "!\n", 2);     /* write는 safe */
    (void)sig;
}

int main(void) {
    while (1) {
        pause();
        if (g_flag) {
            printf("Signal received\n");  /* main에서 printf */
            g_flag = 0;
        }
    }
}
```

### 4. pause() vs sleep() vs busy-wait

```c
/* ❌ busy-wait: CPU 100% 소비 */
while (!g_flag) {}

/* ❌ sleep: 고정 대기, 응답 지연 */
while (!g_flag) sleep(1);

/* ✅ pause(): 시그널이 올 때까지 프로세스 중단, CPU 0% */
while (!g_flag) pause();
```

### 5. kill()로 프로세스 간 통신

```c
/* 부모 → 자식에게 시그널 전송 */
kill(child_pid, SIGUSR1);

/* 자식 → 부모에게 응답 */
kill(getppid(), SIGUSR2);

/* 자기 자신에게 시그널 */
raise(SIGUSR1);       /* = kill(getpid(), SIGUSR1) */

/* 같은 프로세스 그룹 전체에 전송 */
kill(0, SIGTERM);
```

### 6. SA_RESTART 플래그

```c
sa.sa_flags = SA_RESTART;

/* SA_RESTART 없을 때:
 *   read() 블로킹 중 SIGINT → read() returns -1, errno=EINTR
 *   → 직접 EINTR 처리 필요
 *
 * SA_RESTART 있을 때:
 *   read() 블로킹 중 SIGINT → 핸들러 실행 후 read() 자동 재시작
 *   → EINTR 처리 불필요 (편리)
 *
 * 주의: connect(), poll(), select() 등 일부 syscall은
 *       SA_RESTART로도 재시작되지 않음 (플랫폼마다 다름)
 */
```

## 💡 배운 점

### 시그널 vs 다른 IPC

```
Pipe/MQ:    데이터 전송 (payload 있음)
Signal:     이벤트 통보 (payload 없음, 번호만)

Signal의 장점:
  - 어느 프로세스에서든 전송 가능 (PID만 알면)
  - 커널이 직접 전달 → 비동기, 낮은 오버헤드
  - 프로세스 생명주기 관리에 적합 (SIGTERM, SIGKILL)

Signal의 단점:
  - 데이터 전달 불가 (payload = 0)
  - 핸들러에서 사용 가능한 함수 제한 (async-signal-safe만)
  - 경쟁 조건에 취약 (처리 전 같은 시그널 또 도착하면 소실)
```

### 시그널의 실무 활용

```
프로세스 종료 처리:
  SIGTERM → 정상 종료 로직 (파일 닫기, 리소스 해제)
  SIGINT  → Ctrl+C 처리

데몬 프로세스 설정 리로드:
  SIGHUP → 설정 파일 재읽기 (nginx: kill -HUP <pid>)

자식 프로세스 감시:
  SIGCHLD → waitpid() 호출로 좀비 방지

타이머:
  SIGALRM + alarm() → 단순 타이머 구현
  (정밀도 필요시: timer_create + SIGRTMIN 사용)
```

### 실무 패턴: 안전한 종료

```c
static volatile sig_atomic_t g_running = 1;

static void handle_term(int sig) {
    g_running = 0;
    (void)sig;
}

int main(void) {
    struct sigaction sa = {.sa_handler = handle_term};
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    while (g_running) {
        /* 작업 수행 */
    }

    /* 정리 작업 */
    cleanup();
    return 0;
}
```

## 🔮 개선 가능 사항

- [ ] `sigqueue()` + `siginfo_t` 로 시그널과 함께 정수 데이터 전달
- [ ] `sigprocmask()` 로 특정 구간에서 시그널 블로킹
- [ ] `sigwaitinfo()` / `signalfd()` (Linux) 로 동기식 시그널 처리
- [ ] Real-time signal (`SIGRTMIN` ~ `SIGRTMAX`): 큐잉 + 데이터 전달
- [ ] `timer_create()` + POSIX timer로 고정밀 타이머 구현

## 📚 참고 자료

### Man Pages

```bash
man signal
man sigaction
man kill
man raise
man alarm
man pause
man sigprocmask
man sigsuspend
```

### 관련 프로젝트

- [week1-process/mini-shell](../../week1-process/mini-shell) - SIGINT/SIGPIPE 처리 실전
- [day1-pipe_basic](../day1-pipe_basic) - SIGPIPE와 파이프
- [week2-threads/producer-consumer](../../week2-threads/producer-consumer) - 스레드 기반 비동기

## 📝 프로젝트 정보

```
개발 기간: Day 7
환경: macOS / Linux
언어: C
빌드: cc, make
```

**파일 구조:**

```
day7-signal/
├── signal_basic.c         # sigaction + SIGINT/SIGUSR1/SIGALRM
├── signal_parent_child.c  # kill() 로 부모↔자식 SIGUSR1/SIGUSR2 통신
├── Makefile               # all / test / test-pc / valgrind / debug
└── README.md
```

**핵심 패턴:**

```c
/* 등록 */
struct sigaction sa = {.sa_handler = handler, .sa_flags = SA_RESTART};
sigemptyset(&sa.sa_mask);
sigaction(SIGUSR1, &sa, NULL);

/* 대기 (CPU 0%) */
while (!g_flag) pause();

/* 다른 프로세스에 전송 */
kill(target_pid, SIGUSR1);
```

---

**Author:** OnePaperHoon
**Date:** February 2026
**Project:** Linux Kernel Study - Week 3, Day 7
**Topic:** Signal (sigaction + kill + pause)
