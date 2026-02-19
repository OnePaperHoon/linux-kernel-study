# Pipe - 프로세스 간 단방향 통신

부모-자식 프로세스 간 통신의 가장 기본적인 방법

## 🖥️ 실행 화면

### pipe_basic

```bash
$ ./pipe_basic

=== Pipe Basic Example ===

[Parent] Pipe created
[Parent] Read end (FD):  3
[Parent] Write end (FD): 4

[Parent] Child PID: 12345
[Parent] Closed read end
[Child] Started (PID: 12345)
[Child] Closed write end
[Child] Waiting for message from pipe...
[Parent] Sending message: "Hello from parent process!"
[Parent] Sent 28 bytes
[Parent] Closed write end (sent EOF)
[Parent] Waiting for child...
[Child] Received 28 bytes: "Hello from parent process!"
[Child] Closed read end
[Child] Exiting
[Parent] Child finished

=== Pipe Communication Complete ===
```

### pipe_shell

```bash
$ ./pipe_shell

=== Implementing: ls | grep .c ===

[ls] Starting...
[grep] Starting...
-rw-r--r-- 1 user user 2156 Jan 10 10:00 pipe_basic.c
-rw-r--r-- 1 user user 1845 Jan 10 10:00 pipe_shell.c

=== Pipeline Complete ===
```

## 🚀 빌드 및 실행

```bash
# 컴파일
make

# 실행
./pipe_basic
./pipe_shell

# 테스트
make test

# 정리
make clean
```

## 📖 프로젝트 개요

### 목적
Pipe를 사용한 프로세스 간 통신(IPC) 마스터

### 학습 목표
- [x] pipe() 시스템 콜 이해
- [x] Fork 후 파일 디스크립터 복사
- [x] Read end / Write end 개념
- [x] EOF 처리
- [x] dup2()로 리다이렉션
- [x] Shell 파이프 구현 (ls | grep)

### 실무 응용
```
Shell 명령어:
$ ls | grep txt | wc -l
  ↑     ↑        ↑
 Pipe  Pipe    Pipe

프로세스 통신:
Parent → [Pipe] → Child

파이프라인:
Process A → Process B → Process C
```

## 🔧 구현 내용

### 1. Pipe 기본 구조

```
부모 프로세스                자식 프로세스
    │                           │
    ├─── write(pipefd[1]) ─────→│
    │         [Pipe]            │
    │←───── read(pipefd[0]) ─────┤
    │                           │
```

**단방향 통신:**
- Writer → Pipe → Reader
- 반대 방향 불가능
- 양방향 필요 → Pipe 2개

### 2. pipe() 시스템 콜

```c
#include <unistd.h>

int pipefd[2];
int ret = pipe(pipefd);

// 성공: 0 리턴
// 실패: -1 리턴, errno 설정

// pipefd[0]: read end  (출구) 🚪←
// pipefd[1]: write end (입구) 🚪→
```

**내부 동작:**
```
┌──────────────────────────────┐
│     Kernel Space             │
│                              │
│  pipefd[1] → [Buffer] → pipefd[0]
│              (64KB)          │
│                              │
└──────────────────────────────┘
```

**특징:**
- Kernel 버퍼 사용 (보통 64KB)
- Blocking I/O (기본)
- FIFO (First In First Out)
- 원자적 연산 (PIPE_BUF 이하)

### 3. pipe_basic 구현

```c
int main(void)
{
    int pipefd[2];
    pid_t pid;
    
    /* 1. Pipe 생성 */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    /* 2. Fork */
    pid = fork();
    
    if (pid == 0) {
        /* === 자식 프로세스 === */
        
        /* Write end 닫기 (읽기만 할 거니까) */
        close(pipefd[1]);
        
        /* Pipe에서 읽기 */
        char buf[100];
        ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
        buf[n] = '\0';
        printf("Received: %s\n", buf);
        
        /* Read end 닫기 */
        close(pipefd[0]);
        
        exit(0);
        
    } else {
        /* === 부모 프로세스 === */
        
        /* Read end 닫기 (쓰기만 할 거니까) */
        close(pipefd[0]);
        
        /* Pipe에 쓰기 */
        char *msg = "Hello!";
        write(pipefd[1], msg, strlen(msg));
        
        /* Write end 닫기 (EOF 전달!) */
        close(pipefd[1]);
        
        /* 자식 대기 */
        waitpid(pid, NULL, 0);
    }
    
    return 0;
}
```

**실행 흐름:**
```
T0: pipe() → [Pipe 생성]
T1: fork()
T2: 부모: close(pipefd[0])
T3: 자식: close(pipefd[1])
T4: 자식: read() [블로킹]
T5: 부모: write("Hello!")
T6: 자식: read() 리턴, 데이터 수신
T7: 부모: close(pipefd[1]) [EOF 전달]
T8: 자식: read() → 0 (EOF)
T9: 자식: 종료
T10: 부모: waitpid()
```

### 4. Fork 후 FD 복사

```c
pipe(pipefd);
// pipefd[0] = 3
// pipefd[1] = 4

fork();

// 부모와 자식 모두:
// pipefd[0] = 3  (같은 파일 가리킴!)
// pipefd[1] = 4  (같은 파일 가리킴!)
```

**메모리 구조:**
```
         부모                    자식
    ┌──────────┐            ┌──────────┐
    │ pipefd[0]│────┐  ┌────│ pipefd[0]│
    │    = 3   │    │  │    │    = 3   │
    ├──────────┤    │  │    ├──────────┤
    │ pipefd[1]│──┐ │  │ ┌──│ pipefd[1]│
    │    = 4   │  │ │  │ │  │    = 4   │
    └──────────┘  │ │  │ │  └──────────┘
                  ↓ ↓  ↓ ↓
            ┌────────────────────┐
            │   Kernel Space     │
            │  FD 3: [Pipe Read] │
            │  FD 4: [Pipe Write]│
            └────────────────────┘
```

**핵심:**
- FD 값은 복사되지만
- 같은 Kernel 객체를 가리킴
- 한쪽이 닫아도 다른 쪽은 유효

### 5. 사용 안하는 End 닫기

```c
if (pid == 0) {
    /* 자식: 읽기만 */
    close(pipefd[1]);  // Write end 닫기!
    
    read(pipefd[0], buf, size);
    close(pipefd[0]);
    
} else {
    /* 부모: 쓰기만 */
    close(pipefd[0]);  // Read end 닫기!
    
    write(pipefd[1], data, size);
    close(pipefd[1]);  // EOF 전달!
}
```

**왜 닫아야 하나?**

**이유 1: EOF 감지**
```c
// Writer
write(pipefd[1], "Hello", 5);
close(pipefd[1]);  // 모든 write end 닫힘 → EOF 전달

// Reader
while ((n = read(pipefd[0], buf, size)) > 0) {
    // 데이터 처리
}
// n == 0 → EOF 수신!

// 만약 close 안하면?
// → read()가 영원히 블로킹!
// → 프로세스 멈춤
```

**이유 2: 리소스 절약**
```c
// 프로세스당 FD 제한: 1024개 (기본)
// 사용 안하는 FD는 닫아서 재사용
```

**이유 3: 명확성**
```c
// 단방향 통신 명시
// "나는 읽기만 한다" → write end 닫기
// "나는 쓰기만 한다" → read end 닫기
```

### 6. pipe_shell 구현 (ls | grep)

```c
int main(void)
{
    int pipefd[2];
    pid_t pid1, pid2;
    
    pipe(pipefd);
    
    /* 첫 번째 자식: ls */
    pid1 = fork();
    if (pid1 == 0) {
        /* stdout을 pipe write end로 변경 */
        dup2(pipefd[1], STDOUT_FILENO);
        
        /* 사용 안하는 FD 닫기 */
        close(pipefd[0]);
        close(pipefd[1]);
        
        /* ls 실행 */
        execlp("ls", "ls", "-l", NULL);
        perror("execlp ls");
        exit(1);
    }
    
    /* 두 번째 자식: grep */
    pid2 = fork();
    if (pid2 == 0) {
        /* stdin을 pipe read end로 변경 */
        dup2(pipefd[0], STDIN_FILENO);
        
        /* 사용 안하는 FD 닫기 */
        close(pipefd[0]);
        close(pipefd[1]);
        
        /* grep 실행 */
        execlp("grep", "grep", ".c", NULL);
        perror("execlp grep");
        exit(1);
    }
    
    /* 부모: pipe 사용 안함 */
    close(pipefd[0]);
    close(pipefd[1]);
    
    /* 자식들 종료 대기 */
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    
    return 0;
}
```

**동작 과정:**
```
1. pipe() 생성
   pipefd[0] = 3 (read)
   pipefd[1] = 4 (write)

2. fork() → ls 프로세스
   dup2(4, 1)  // stdout → pipe write
   execlp("ls")
   → ls의 출력이 pipe로!

3. fork() → grep 프로세스
   dup2(3, 0)  // stdin ← pipe read
   execlp("grep")
   → grep이 pipe에서 읽음!

4. 부모는 pipe 안씀
   close(3)
   close(4)

결과: ls | grep
```

### 7. dup2() 리다이렉션

```c
#include <unistd.h>

int dup2(int oldfd, int newfd);

// oldfd를 newfd로 복사
// newfd가 열려있으면 먼저 닫음
// 성공: newfd 리턴
// 실패: -1
```

**예제:**
```c
int pipefd[2];
pipe(pipefd);

// stdout을 pipe write end로 변경
dup2(pipefd[1], STDOUT_FILENO);

// 이제 printf()의 출력이 pipe로!
printf("Hello\n");  // → pipe로 전송

close(pipefd[1]);
```

**내부 동작:**
```
Before:
FD 0: stdin
FD 1: stdout  ──→ [터미널]
FD 2: stderr
FD 3: [pipe read]
FD 4: [pipe write]

dup2(4, 1);

After:
FD 0: stdin
FD 1: stdout  ──→ [pipe write]  (변경!)
FD 2: stderr
FD 3: [pipe read]
FD 4: [pipe write]

이제 printf() → FD 1 → pipe로 전송!
```

---

## 💡 배운 점

### Pipe란?

**정의:**
```
단방향 통신 채널
부모-자식 프로세스 간 데이터 전달
Kernel 버퍼 사용
```

**특징:**
- **단방향**: Write → Read만 가능
- **익명**: 파일시스템에 없음 (이름 없음)
- **관계**: 부모-자식 또는 형제 프로세스만
- **Blocking**: 기본적으로 블로킹 I/O
- **FIFO**: First In First Out 순서 보장
- **원자성**: PIPE_BUF(4096) 이하는 원자적

### Pipe vs FIFO vs Shared Memory

```
Pipe:
- 단방향
- 부모-자식만
- 익명 (이름 없음)
- 속도: 보통

FIFO (Named Pipe):
- 단방향
- 무관한 프로세스
- 파일시스템에 존재
- 속도: 보통

Shared Memory:
- 양방향
- 무관한 프로세스
- 메모리 직접 공유
- 속도: 가장 빠름!
```

### Pipe 크기 (PIPE_BUF)

```c
#include <limits.h>

printf("PIPE_BUF: %d\n", PIPE_BUF);
// Linux: 4096 bytes

// 원자성 보장:
// ≤ PIPE_BUF → 원자적 (섞이지 않음)
// > PIPE_BUF → 여러 번 나눠 씀 (섞일 수 있음)
```

**예시:**
```c
// Writer A
write(pipefd[1], "AAAA", 4000);  // 원자적

// Writer B
write(pipefd[1], "BBBB", 4000);  // 원자적

// Reader
read() → "AAAA...AAAA" 또는 "BBBB...BBBB"
// 섞이지 않음!

// 하지만:
write(pipefd[1], "AAA...", 10000);  // 비원자적
write(pipefd[1], "BBB...", 10000);  // 비원자적

// Reader
read() → "AAA...BBB...AAA...BBB..."  // 섞임!
```

### EOF 처리

**EOF란?**
```
End Of File
"더 이상 데이터 없음" 신호
read()가 0을 리턴
```

**Pipe에서 EOF:**
```c
// 모든 write end가 닫혀야 EOF 발생!

// Writer
write(pipefd[1], "Hello", 5);
close(pipefd[1]);  // EOF 전달

// Reader
n = read(pipefd[0], buf, size);
// n > 0: 데이터 수신
// n == 0: EOF (모든 writer 종료)
// n < 0: 에러
```

**주의사항:**
```c
// ❌ Write end 안 닫으면
write(pipefd[1], "Hello", 5);
// close(pipefd[1]);  // 안 닫음!

// Reader
n = read(pipefd[0], buf, size);  // 영원히 블로킹!

// ✅ 반드시 닫기
write(pipefd[1], "Hello", 5);
close(pipefd[1]);  // EOF 전달!
```

### Blocking vs Non-blocking

**Blocking (기본):**
```c
// Reader
n = read(pipefd[0], buf, size);
// Pipe 비어있으면 데이터 올 때까지 대기

// Writer
n = write(pipefd[1], data, size);
// Pipe 가득 차면 공간 생길 때까지 대기
```

**Non-blocking:**
```c
#include <fcntl.h>

// Non-blocking 설정
int flags = fcntl(pipefd[0], F_GETFL);
fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

// Reader
n = read(pipefd[0], buf, size);
if (n == -1 && errno == EAGAIN) {
    // 데이터 없음, 나중에 다시
}

// Writer
n = write(pipefd[1], data, size);
if (n == -1 && errno == EAGAIN) {
    // 버퍼 가득 찼음, 나중에 다시
}
```

### SIGPIPE 시그널

```c
// Reader가 먼저 종료하면?

// Reader
close(pipefd[0]);
exit(0);

// Writer (나중에)
write(pipefd[1], data, size);  // SIGPIPE 발생!
// 기본 동작: 프로세스 종료

// 해결 1: SIGPIPE 무시
signal(SIGPIPE, SIG_IGN);

// 해결 2: 에러 처리
if (write(pipefd[1], data, size) == -1) {
    if (errno == EPIPE) {
        // Broken pipe
    }
}
```

### Shell 파이프 구현

**원리:**
```bash
$ ls | grep txt

동작:
1. Shell이 pipe() 생성
2. fork() → ls 프로세스
3. ls의 stdout을 pipe write end로 변경
4. fork() → grep 프로세스
5. grep의 stdin을 pipe read end로 변경
6. 각자 exec()
7. ls 출력 → pipe → grep 입력
```

**다중 파이프:**
```bash
$ ls | grep txt | wc -l

         Pipe 1        Pipe 2
ls ──→ [Pipe] ──→ grep ──→ [Pipe] ──→ wc
```

---

## 📊 성능

### Pipe vs Shared Memory

```
Benchmark: 1MB 데이터 전송

Pipe:           5-10 ms
Shared Memory:  0.5-1 ms

Pipe는 Shared Memory의 10배 느림!
이유: Kernel 경유 (copy overhead)
```

**Pipe 속도:**
```
Process A                    Process B
    │                            │
    ├─ write() ─→ [Kernel] ─→ read()
    │     ↓         ↓          ↑
    │   User     Kernel      User
    │   Space    Space      Space
    │
    └─ 2번 복사 필요! (User→Kernel→User)
```

**Shared Memory 속도:**
```
Process A                    Process B
    │                            │
    └─────→ [Shared Memory] ←────┘
              (0 copy!)
```

### Pipe Buffer 크기

```bash
# Pipe 버퍼 크기 확인
$ cat /proc/sys/fs/pipe-max-size
1048576  # 1MB (최대)

# 기본 크기
$ ulimit -p
8  # 8 * 512 bytes = 4KB (기본)

# 실제 버퍼 크기
$ cat /proc/sys/fs/pipe-user-pages-soft
16384  # 64MB per user
```

**실험:**
```c
// Writer
char buf[1024 * 1024];  // 1MB
write(pipefd[1], buf, sizeof(buf));
// 여러 번 나눠서 씀 (버퍼 크기 제한)

// Reader
while (read(pipefd[0], buf, size) > 0) {
    // 여러 번 읽음
}
```

---

## 🔮 개선 가능 사항

### 1. 양방향 Pipe

- [ ] Pipe 2개 사용
- [ ] 부모 ↔ 자식 양방향 통신
- [ ] socketpair() 사용 (더 간단)

```c
// Pipe 2개
int pipe1[2];  // 부모 → 자식
int pipe2[2];  // 자식 → 부모

pipe(pipe1);
pipe(pipe2);

fork();
```

### 2. Non-blocking I/O

- [ ] fcntl()로 O_NONBLOCK 설정
- [ ] select()나 poll()로 다중 Pipe 모니터링
- [ ] Busy-wait 방지

### 3. 에러 처리 강화

- [ ] SIGPIPE 처리
- [ ] EINTR (시스템 콜 중단) 재시도
- [ ] Partial write/read 처리

### 4. 성능 최적화

- [ ] 큰 버퍼 사용 (64KB+)
- [ ] splice() 시스템 콜 (zero-copy)
- [ ] vmsplice() (memory → pipe zero-copy)

---

## 📚 참고 자료

### Man Pages
```bash
man 2 pipe
man 2 dup2
man 2 read
man 2 write
man 2 close
man 7 pipe
```

### 관련 문서
- [[01-process-basics]] - Fork, Exec, Wait
- [Week 1: Process Tree](../../week1-process/process-tree-visualizer)
- TLPI Chapter 44: Pipes and FIFOs

### 시스템 콜
```c
pipe()    // Pipe 생성
read()    // Pipe 읽기
write()   // Pipe 쓰기
close()   // FD 닫기
dup2()    // FD 복사
fork()    // 프로세스 복사
exec()    // 프로그램 실행
```

### 실무 응용
- Shell 파이프라인
- Process 간 통신
- Log 수집 시스템
- 부모-자식 프로세스 협업

---

## 🎯 다음 단계

**학습 순서:**
1. ✅ Pipe (현재)
2. → FIFO (Named Pipe)
3. → Shared Memory
4. → Message Queue
5. → Signal

**다음 프로젝트:**
- pipe_bidirectional: 양방향 통신
- pipe_nonblocking: Non-blocking I/O
- Day 3: FIFO (Named Pipe)

---

## 📝 프로젝트 정보

```
개발 기간: Day 1-2
환경: Linux (Debian/Ubuntu)
언어: C
라이브러리: unistd.h, sys/wait.h
빌드: gcc, make
```

**파일 구조:**
```
day1-2-pipe/
├── pipe_basic.c      # 부모-자식 통신
├── pipe_shell.c      # ls | grep 구현
├── Makefile          # 빌드 설정
└── README.md         # 이 문서
```

**핵심 개념:**
```c
// Pipe 생성
int pipefd[2];
pipe(pipefd);  // [0]=read, [1]=write

// Fork
fork();

// 자식: 읽기만
close(pipefd[1]);
read(pipefd[0], buf, size);

// 부모: 쓰기만
close(pipefd[0]);
write(pipefd[1], data, size);
close(pipefd[1]);  // EOF 전달!

// 리다이렉션
dup2(pipefd[1], STDOUT_FILENO);  // stdout → pipe
```

---

**Author:** OnePaperHoon  
**Date:** February 2026  
**Project:** Linux Kernel Study - Week 3, Day 1-2  
**Topic:** Pipe (Inter-Process Communication)