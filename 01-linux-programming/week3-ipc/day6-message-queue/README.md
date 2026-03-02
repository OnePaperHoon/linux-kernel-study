# System V Message Queue

메시지 타입(mtype)으로 **우선순위 수신**이 가능한 IPC — Pipe와 달리 도착 순서에 무관하게 원하는 타입만 먼저 꺼낼 수 있다.

## 🖥️ 실행 화면

### mq_basic (fork 기반 데모)

```bash
$ ./mq_basic

=== System V Message Queue Basic Example ===
    (큐에 type=2,2,1,2,1 순서로 쌓아도
     mtype=-2 수신 시 type=1 먼저 나옴)

[Main] Message queue created (msqid: 131072)

[Sender] Starting (PID: 12345)
[Sender] Sending 5 messages (순서대로):

[Sender] Sent  [type=2]: [LOW]  B 업무 처리 요청
[Sender] Sent  [type=2]: [LOW]  C 업무 처리 요청
[Sender] Sent  [type=1]: [HIGH] A 긴급 처리 요청
[Sender] Sent  [type=2]: [LOW]  D 업무 처리 요청
[Sender] Sent  [type=1]: [HIGH] E 긴급 처리 요청

[Sender] Done

[Receiver] Starting (PID: 12346)
[Receiver] Receiving with mtype=-2 (우선순위: type 1 먼저):

[Receiver] Got [type=1]: [HIGH] A 긴급 처리 요청   ← 3번째로 들어왔지만 먼저 나옴
[Receiver] Got [type=1]: [HIGH] E 긴급 처리 요청   ← 5번째로 들어왔지만 2번째로 나옴
[Receiver] Got [type=2]: [LOW]  B 업무 처리 요청
[Receiver] Got [type=2]: [LOW]  C 업무 처리 요청
[Receiver] Got [type=2]: [LOW]  D 업무 처리 요청

[Receiver] Done

[Main] Removing message queue (msqid: 131072)

=== Message Queue Communication Complete ===
```

### mq_sender / mq_receiver (독립 프로세스)

```bash
# Terminal 1
$ ./mq_receiver
=== Message Queue Receiver ===
[Receiver] PID: 22222
[Receiver] Queue key: 0x4d012345
[Receiver] Waiting for queue...
[Receiver] Queue found (msqid: 131073)

[Receiver] Receiving with mtype=-2 (긴급 메시지 우선):

[Receiver] Got  [type=1]: 긴급 작업 X
[Receiver] Got  [type=1]: 긴급 작업 Y
[Receiver] Got  [type=2]: 일반 작업 A
[Receiver] Got  [type=2]: 일반 작업 B
[Receiver] Got  [type=2]: 일반 작업 C
[Receiver] Got  [type=2]: 일반 작업 D

[Receiver] All messages received. Exiting.

# Terminal 2
$ ./mq_sender
=== Message Queue Sender ===
[Sender] PID: 11111
[Sender] Queue key: 0x4d012345
[Sender] Queue created (msqid: 131073)
[Sender] Receiver가 준비될 시간... (1s)

[Sender] Sending 6 messages:
[Sender] Sent  [type=2]: 일반 작업 A
[Sender] Sent  [type=2]: 일반 작업 B
[Sender] Sent  [type=1]: 긴급 작업 X
...
[Sender] Queue removed
```

## 🚀 빌드 및 실행

```bash
# 전체 빌드
make

# fork 기반 데모 실행
make test

# 메모리 검사
make valgrind

# 독립 프로세스 IPC 데모 (터미널 2개)
./mq_receiver       # Terminal 1 먼저
./mq_sender         # Terminal 2

# System IPC 상태 확인
ipcs -q

# 남은 큐 수동 삭제
ipcrm -q <msqid>
```

## 📖 프로젝트 개요

### 목적
System V Message Queue의 **mtype 기반 우선순위 수신** 메커니즘 이해

### 학습 목표
- [x] `msgget` / `msgsnd` / `msgrcv` / `msgctl` API 이해
- [x] mtype 우선순위 수신 (`mtype=-N`)
- [x] `IPC_PRIVATE` (fork 기반) vs `ftok` (독립 프로세스)
- [x] System V IPC 수명 관리 (`ipcs`, `ipcrm`)
- [x] POSIX MQ vs System V MQ 차이

### IPC 방식 비교

```
Pipe:
  - 바이트 스트림 (FIFO, 순서 변경 불가)
  - fork 또는 FIFO 파일로만 공유

FIFO (Named Pipe):
  - 바이트 스트림 (FIFO)
  - 파일 시스템 경로로 공유

Message Queue (이 프로젝트):
  - 개별 메시지 단위
  - mtype으로 우선순위/선택적 수신
  - 커널이 큐 관리 (프로세스 종료 후에도 큐 유지!)
```

> **macOS 참고:** POSIX MQ (`mq_open`, `<mqueue.h>`)는 macOS에서 미지원.
> 이 프로젝트는 macOS/Linux 모두 지원하는 System V MQ를 사용.

## 🔧 구현 내용

### 1. 메시지 구조체

```c
typedef struct {
    long    mtype;              /* 반드시 첫 번째, 값은 1 이상 */
    char    mtext[MSG_TEXT_SIZE];
} t_msg;

/* msgsnd/msgrcv의 size 인자 = mtext 크기만 (mtype 제외) */
msgsnd(msqid, &msg, sizeof(msg.mtext), 0);
msgrcv(msqid, &msg, sizeof(msg.mtext), mtype, 0);
```

### 2. 큐 생성 방식

```c
/* fork 기반: IPC_PRIVATE → 자식이 msqid를 상속 */
msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

/* 독립 프로세스: ftok으로 공유 키 생성 */
key_t key = ftok(".", 'M');  /* 경로 + proj_id → 동일한 key */
msqid = msgget(key, IPC_CREAT | 0666);
```

### 3. mtype으로 우선순위 수신 — 핵심 기능

```
큐 상태 (도착 순서):
  [type=2] → [type=2] → [type=1] → [type=2] → [type=1]

msgrcv(msqid, &msg, size, 0, 0):   /* FIFO */
  꺼내는 순서: type=2, type=2, type=1, type=2, type=1

msgrcv(msqid, &msg, size, 1, 0):   /* type=1만 */
  꺼내는 순서: type=1, type=1

msgrcv(msqid, &msg, size, -2, 0):  /* type≤2 중 낮은 것 먼저 */
  꺼내는 순서: type=1, type=1, type=2, type=2, type=2
```

→ **`mtype=-N` 패턴이 실무 우선순위 큐를 구현하는 핵심**

### 4. 큐 수명 (Lifetime)

```
Pipe:          프로세스 종료 → 자동 소멸
Message Queue: 프로세스 종료 후에도 커널에 남아있음!
               → msgctl(msqid, IPC_RMID, NULL) 로 명시적 삭제 필요
               → 안 지우면 ipcs -q 로 계속 보임
```

```bash
# 남아있는 큐 확인
ipcs -q

# 수동 삭제
ipcrm -q <msqid>
```

### 5. msgsnd / msgrcv 플래그

```c
/* msgsnd flags */
msgsnd(msqid, &msg, size, 0);        /* 블로킹: 큐 가득 차면 대기 */
msgsnd(msqid, &msg, size, IPC_NOWAIT); /* 비블로킹: 가득 차면 EAGAIN */

/* msgrcv flags */
msgrcv(msqid, &msg, size, type, 0);         /* 블로킹 */
msgrcv(msqid, &msg, size, type, IPC_NOWAIT); /* 비블로킹: 없으면 ENOMSG */
msgrcv(msqid, &msg, size, type, MSG_NOERROR); /* 메시지 잘려도 에러 X */
```

## 💡 배운 점

### Message Queue vs Pipe

```
Pipe (byte stream):
  write("ABCDE") → read(3) → "ABC", read(2) → "DE"
  → 경계 없음, 파서 필요

Message Queue (message unit):
  msgsnd("Hello") → msgrcv() → "Hello"  (항상 메시지 단위)
  → 자동으로 경계 구분
```

### mtype = 자체 우선순위 큐

```c
/* 실무 패턴: mtype으로 우선순위 표현 */
#define PRIORITY_HIGH   1
#define PRIORITY_NORMAL 2
#define PRIORITY_LOW    3

/* 전송 */
msg.mtype = PRIORITY_HIGH;
msgsnd(msqid, &msg, size, 0);

/* 수신: 우선순위 높은 것(type=1)부터 */
msgrcv(msqid, &msg, size, -3, 0);
```

→ `mq_basic.c`에서 type2가 먼저 도착해도 type1이 먼저 나오는 이유

### IPC 자원 누수 주의

```c
/* ❌ 잊으면 ipcs -q 에 계속 남음 */
exit(EXIT_FAILURE);

/* ✅ 항상 종료 전 삭제 */
msgctl(msqid, IPC_RMID, NULL);
exit(EXIT_FAILURE);
```

### 실무 응용

```
로그 시스템:
  LOG_ERROR (type=1) → LOG_WARN (type=2) → LOG_INFO (type=3)
  에러 로그를 먼저 처리

작업 큐:
  긴급 작업(type=1)과 일반 작업(type=2) 분리

프로세스 간 이벤트 전달:
  각 이벤트 종류를 mtype으로 구분
```

## 🔮 개선 가능 사항

- [ ] 다중 타입 우선순위 큐 (type=1,2,3,...,N)
- [ ] `IPC_NOWAIT` + 폴링 기반 수신
- [ ] 양방향 통신 (큐 2개: request queue + reply queue)
- [ ] POSIX MQ (`mq_open`)로 동일 기능 구현 (Linux only)

## 📚 참고 자료

### Man Pages

```bash
man msgget
man msgsnd
man msgrcv
man msgctl
man ftok
man ipcs
man ipcrm
```

### 관련 프로젝트

- [day1-pipe_basic](../day1-pipe_basic) - Anonymous Pipe
- [day3-fifo](../day3-fifo) - Named Pipe
- [day4-5-shared-memory](../day4-5-shared-memory) - Shared Memory

## 📝 프로젝트 정보

```
개발 기간: Day 6
환경: macOS / Linux (System V IPC)
언어: C
빌드: cc, make
```

**파일 구조:**

```
day6-message-queue/
├── mq_basic.c      # fork 기반 데모 (IPC_PRIVATE, mtype 우선순위)
├── mq_sender.c     # 독립 Sender (ftok 키 공유)
├── mq_receiver.c   # 독립 Receiver (mtype=-2 우선순위 수신)
├── Makefile        # all / test / valgrind / debug / sanitize
└── README.md
```

**핵심 흐름:**

```c
/* 생성 */
key_t key = ftok(".", 'M');
int msqid = msgget(key, IPC_CREAT | 0666);

/* 전송 */
t_msg msg = {.mtype = 1};
strcpy(msg.mtext, "hello");
msgsnd(msqid, &msg, sizeof(msg.mtext), 0);

/* 수신 (우선순위: type≤2 중 낮은 것 먼저) */
msgrcv(msqid, &msg, sizeof(msg.mtext), -2, 0);

/* 삭제 */
msgctl(msqid, IPC_RMID, NULL);
```

---

**Author:** OnePaperHoon
**Date:** February 2026
**Project:** Linux Kernel Study - Week 3, Day 6
**Topic:** System V Message Queue (msgget + msgsnd + msgrcv, mtype 우선순위)
