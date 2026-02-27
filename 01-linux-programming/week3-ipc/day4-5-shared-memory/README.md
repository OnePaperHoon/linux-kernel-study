# POSIX Shared Memory

프로세스 간 가장 빠른 IPC 방식 — 커널 복사 없이 물리 메모리를 직접 공유

## 🖥️ 실행 화면

### shm_basic (fork 기반 데모)

```bash
$ ./shm_basic

=== POSIX Shared Memory Basic Example ===

[Main] Creating shared memory: /shm_basic_example
[Main] Shared memory mapped (size: 136 bytes)
[Main] Address: 0x1045f8000

[Reader] Starting (PID: 12346)
[Writer] Starting (PID: 12345)
[Writer] Sent [1/3]: "Hello from writer!"
[Reader] Received: "Hello from writer!"
[Writer] Sent [2/3]: "Shared memory: no kernel copy"
[Reader] Received: "Shared memory: no kernel copy"
[Writer] Sent [3/3]: "IPC via mmap + shm_open"
[Reader] Received: "IPC via mmap + shm_open"
[Writer] Done
[Reader] Done

[Main] Cleaning up...

=== Shared Memory Communication Complete ===
```

### shm_writer / shm_reader (독립 프로세스 IPC)

```bash
# Terminal 1 (먼저 실행)
$ ./shm_reader
=== Shared Memory Reader ===
[Reader] PID: 22222

[Reader] Waiting for shared memory '/day4_shm_ipc'...
[Reader] Connected to shared memory

[Reader] Received: "Message 1: Hello, Reader!"
[Reader] Received: "Message 2: Shared memory is fast"
[Reader] Received: "Message 3: No kernel copy overhead"
[Reader] Received: "Message 4: Direct memory access"
[Reader] Received: "Message 5: IPC complete"

[Reader] All messages received. Exiting.

# Terminal 2
$ ./shm_writer
=== Shared Memory Writer ===
[Writer] PID: 11111

[Writer] Shared memory '/day4_shm_ipc' created
[Writer] Waiting for reader to connect... (2s)

[Writer] Sent [1/5]: "Message 1: Hello, Reader!"
[Writer] Sent [2/5]: "Message 2: Shared memory is fast"
...
[Writer] Sent [5/5]: "Message 5: IPC complete"

[Writer] All messages sent. Reader will exit.
[Writer] Shared memory removed
```

## 🚀 빌드 및 실행

```bash
# 전체 빌드 (shm_basic, shm_writer, shm_reader)
make

# fork 기반 데모 실행
make test

# 메모리 검사
make valgrind

# 독립 프로세스 IPC 데모 (터미널 2개)
./shm_reader       # Terminal 1 먼저
./shm_writer       # Terminal 2

# 디버그 빌드
make debug

# AddressSanitizer
make sanitize
```

## 📖 프로젝트 개요

### 목적
POSIX Shared Memory(`shm_open` + `mmap`)를 통해 프로세스 간 제로 카피(Zero-Copy) IPC 구현

### 학습 목표
- [x] `shm_open` / `ftruncate` / `mmap` API 이해
- [x] `MAP_SHARED` vs `MAP_PRIVATE` 차이
- [x] volatile 플래그 기반 동기화 (Spin-wait)
- [x] fork 기반 공유 vs 독립 프로세스 간 공유
- [x] 공유 메모리 수명 관리 (`shm_unlink`)

### IPC 방식 비교

```
Pipe:
  Writer ──write()──▶ [커널 버퍼] ──read()──▶ Reader
                         복사 2회

Shared Memory:
  Writer ──▶ [물리 메모리] ◀── Reader
              복사 0회 (직접 접근)

→ 대용량 데이터 전송에서 Shared Memory가 압도적으로 빠름
```

## 🔧 구현 내용

### 1. 공유 메모리 구조체

```c
typedef struct {
    char            message[MSG_SIZE]; /* 공유 데이터 */
    volatile int    data_ready;        /* 1: 새 데이터, 0: 소비됨 */
    volatile int    done;              /* 1: writer 완료 */
} t_shm;
```

`volatile`: 컴파일러가 레지스터 캐싱 없이 매번 메모리에서 읽도록 강제

### 2. 생성 및 매핑 흐름

```c
/* 1. 공유 메모리 객체 생성 (/dev/shm/name 또는 macOS /private/tmp/) */
shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

/* 2. 크기 설정 (생성 직후 0바이트) */
ftruncate(shm_fd, sizeof(t_shm));

/* 3. 프로세스 주소 공간에 매핑 */
shm = mmap(NULL, sizeof(t_shm),
           PROT_READ | PROT_WRITE,
           MAP_SHARED,   /* 모든 프로세스가 같은 물리 페이지 공유 */
           shm_fd, 0);

close(shm_fd);  /* mmap 이후 fd 불필요 */
```

### 3. MAP_SHARED vs MAP_PRIVATE

```
MAP_SHARED:
  프로세스 A ──▶ [물리 페이지]
  프로세스 B ──▶ [물리 페이지]  ← 동일한 페이지
  → A의 쓰기가 B에서 즉시 보임

MAP_PRIVATE (Copy-on-Write):
  프로세스 A ──▶ [물리 페이지 A] (쓰기 시 복사)
  프로세스 B ──▶ [물리 페이지 B] (쓰기 시 복사)
  → 서로의 쓰기가 보이지 않음 (IPC 불가)
```

### 4. Spin-wait 동기화

```c
/* Writer: Reader가 소비할 때까지 대기 */
while (shm->data_ready)
    usleep(1000);
shm->data_ready = 1;

/* Reader: 새 데이터가 올 때까지 대기 */
while (!shm->data_ready)
    usleep(1000);
shm->data_ready = 0;
```

**한계:** Spin-wait은 CPU를 소모함. 실무에서는 `sem_open` (POSIX 세마포어) 또는 `pthread_mutex` + `pthread_cond`를 공유 메모리 안에 넣어 사용

### 5. 공유 메모리 수명 (Lifetime)

```
shm_open() ──▶ 객체 생성 (파일 시스템에 이름 등록)
                          ↓
               프로세스가 종료해도 객체는 남아있음!
                          ↓
shm_unlink() ──▶ 이름 제거 (이후 새 open 불가)
                 마지막 참조가 사라지면 메모리 해제
```

**주의:** 프로그램이 비정상 종료하면 `/shm_name`이 남을 수 있음
→ 시작 시 `shm_unlink()`로 잔여 객체 제거하는 패턴 사용

### 6. 독립 프로세스 간 연결 (shm_writer / shm_reader)

```
shm_writer:
  shm_open(O_CREAT | O_RDWR) → 생성
  ftruncate → 크기 설정
  mmap → 매핑
  → 데이터 쓰기

shm_reader:
  shm_open(O_RDWR)           → 기존 객체에 연결 (O_CREAT 없음)
  mmap → 같은 물리 메모리에 매핑
  → 데이터 읽기

※ 두 프로세스의 가상 주소(포인터 값)는 달라도
   같은 물리 페이지를 가리킴
```

## 💡 배운 점

### Zero-Copy가 중요한 이유

```
Pipe로 1GB 전송:
  write() → 커널 버퍼 복사 (1GB)
  read()  → 유저 버퍼 복사 (1GB)
  총 복사: 2GB

Shared Memory로 1GB 전송:
  포인터로 직접 접근
  총 복사: 0GB

→ 대용량 데이터 / 고빈도 통신에서 차이가 극명
```

### mmap 이후 fd를 닫아도 되는 이유

```c
shm_fd = shm_open(...);
shm = mmap(..., shm_fd, 0);
close(shm_fd);  // ← 안전!
```

mmap이 내부적으로 파일(공유 메모리 객체)에 대한 참조를 유지하므로
fd를 닫아도 매핑은 유효함. fd를 일찍 닫으면 실수로 두 번 close 하는 버그 방지.

### Pipe와 용도 구분

```
Pipe:
  - 단순 데이터 스트림
  - 부모-자식 관계 (또는 FIFO로 무관계)
  - 소량 데이터

Shared Memory:
  - 대용량 / 고빈도 데이터
  - 임의 접근 (랜덤 읽기/쓰기)
  - 별도 동기화 필수 (Mutex, Semaphore)
```

### 실무 응용

```
OpenCV / 영상 처리:
  카메라 프레임 → [Shared Memory] → 처리 프로세스
  (프레임당 수 MB → 복사 비용이 치명적)

Redis / 캐시:
  공유 메모리에 캐시 데이터 적재
  여러 Worker가 직접 접근

PostgreSQL:
  shared_buffers = 공유 메모리 기반 버퍼 풀
  Backend 프로세스들이 같은 페이지 캐시 공유
```

## 🔮 개선 가능 사항

- [ ] POSIX 세마포어 동기화 (`sem_open`, `sem_wait`, `sem_post`)
- [ ] `pthread_mutex` + `pthread_cond` in shared memory (PTHREAD_PROCESS_SHARED)
- [ ] 링 버퍼 구조로 다중 메시지 큐 구현
- [ ] 다중 Reader / 다중 Writer (Reader-Writer Lock)
- [ ] `mmap`으로 파일 매핑 (File-backed shared memory)

## 📚 참고 자료

### Man Pages

```bash
man shm_open
man mmap
man ftruncate
man munmap
man shm_unlink
```

### Linux (컴파일 차이)

```bash
# Linux에서는 -lrt 링크 필요
cc -Wall -Wextra -Werror shm_basic.c -o shm_basic -lrt

# macOS는 libc에 포함 (추가 플래그 불필요)
cc -Wall -Wextra -Werror shm_basic.c -o shm_basic
```

### 관련 프로젝트

- [day1-pipe_basic](../day1-pipe_basic) - 익명 파이프 기초
- [day3-fifo](../day3-fifo) - Named Pipe
- [day6-message-queue](../day6-message-queue) - POSIX 메시지 큐

## 📝 프로젝트 정보

```
개발 기간: Day 4-5
환경: macOS (POSIX)
언어: C
빌드: cc, make
```

**파일 구조:**

```
day4-5-shared-memory/
├── shm_basic.c    # fork 기반 단일 바이너리 데모
├── shm_writer.c   # 독립 Writer 프로세스
├── shm_reader.c   # 독립 Reader 프로세스
├── Makefile       # all / test / valgrind / debug / sanitize
└── README.md
```

**핵심 흐름:**

```c
/* 생성 (Writer) */
shm_fd = shm_open(NAME, O_CREAT | O_RDWR, 0666);
ftruncate(shm_fd, sizeof(t_shm));
shm = mmap(NULL, sizeof(t_shm), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
close(shm_fd);

/* 연결 (Reader) */
shm_fd = shm_open(NAME, O_RDWR, 0666);  /* O_CREAT 없음 */
shm = mmap(NULL, sizeof(t_shm), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
close(shm_fd);

/* 정리 (Writer) */
munmap(shm, sizeof(t_shm));
shm_unlink(NAME);
```

---

**Author:** OnePaperHoon
**Date:** February 2026
**Project:** Linux Kernel Study - Week 3, Day 4-5
**Topic:** POSIX Shared Memory (shm_open + mmap)
