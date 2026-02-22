# Pipe Shell — `ls | grep .c` 구현

Shell 파이프라인의 내부 동작을 직접 구현 (`ls | grep .c`)

## 🖥️ 실행 화면

```bash
$ ./pipe_shell

=== Implementing: ls | grep .c ===

[ls] Starting...
[grep] Starting...
-rw-r--r-- 1 user user  88 Feb 10 10:00 pipe_shell.c

=== Pipeline Complete ===
```

## 🚀 빌드 및 실행

```bash
# 빌드
make

# 실행
./pipe_shell

# 테스트
make test
```

## 📖 프로젝트 개요

### 목적
`$ ls | grep .c` 명령어가 내부에서 어떻게 동작하는지 직접 구현

### 학습 목표
- [x] 2개 프로세스를 Pipe로 연결
- [x] dup2()로 stdout / stdin 리다이렉션
- [x] execlp()로 외부 명령어 실행
- [x] 부모 프로세스의 Pipe FD 정리
- [x] 두 자식 프로세스 대기 (waitpid × 2)

### day1-pipe_basic과의 차이

```
day1-pipe_basic:
부모 → [Pipe] → 자식
(직접 write/read)

day2-pipe_shell:
자식1(ls) → [Pipe] → 자식2(grep)
(dup2로 stdout/stdin 교체 후 exec)
```

## 🔧 구현 내용

### 1. 전체 구조

```
                  Pipe
┌──────────┐  pipefd[1]→pipefd[0]  ┌──────────┐
│  ls -l   │ ─────────────────────→ │ grep .c  │
│ (stdout) │                        │ (stdin)  │
└──────────┘                        └──────────┘
    pid1                                pid2
                   부모(waitpid × 2)
```

### 2. 핵심 흐름

```c
/* 1. Pipe 생성 */
pipe(pipefd);
// pipefd[0] = read end
// pipefd[1] = write end

/* 2. ls 프로세스 */
pid1 = fork();
if (pid1 == 0) {
    dup2(pipefd[1], STDOUT_FILENO); // stdout → pipe write
    close(pipefd[0]);
    close(pipefd[1]);
    execlp("ls", "ls", "-l", NULL); // ls 실행
}

/* 3. grep 프로세스 */
pid2 = fork();
if (pid2 == 0) {
    dup2(pipefd[0], STDIN_FILENO);  // stdin ← pipe read
    close(pipefd[0]);
    close(pipefd[1]);
    execlp("grep", "grep", ".c", NULL); // grep 실행
}

/* 4. 부모: pipe 불필요 → 닫기 */
close(pipefd[0]);
close(pipefd[1]);

/* 5. 두 자식 종료 대기 */
waitpid(pid1, NULL, 0);
waitpid(pid2, NULL, 0);
```

### 3. dup2() 리다이렉션 원리

**ls stdout → pipe:**
```
Before dup2:
FD 0: stdin
FD 1: stdout → [터미널]
FD 3: pipefd[0] (read)
FD 4: pipefd[1] (write)

dup2(pipefd[1], STDOUT_FILENO)  →  dup2(4, 1)

After dup2:
FD 0: stdin
FD 1: stdout → [pipe write]  ← 변경!
FD 3: pipefd[0] (read)
FD 4: pipefd[1] (write)

이제 printf(), puts() 등 → FD 1 → pipe로!
```

**grep stdin ← pipe:**
```
dup2(pipefd[0], STDIN_FILENO)  →  dup2(3, 0)

After dup2:
FD 0: stdin ← [pipe read]  ← 변경!
FD 1: stdout → [터미널]
...

이제 grep의 read() → FD 0 → pipe에서 읽음!
```

### 4. 부모가 Pipe를 닫아야 하는 이유

```c
/* 부모가 close 안 하면? */
// ls 종료 → pipefd[1] 닫힘
// 하지만 부모도 pipefd[1] 보유 중
// → grep의 read() 입장에서 write end가 아직 열려있음
// → EOF 도달 안 함 → grep이 영원히 대기
// → 데드락!

/* ✅ 부모가 반드시 닫아야 함 */
close(pipefd[0]);
close(pipefd[1]);
```

**FD 참조 카운트:**
```
pipe() 후:
pipefd[1] 참조: 부모(1)

fork() × 2 후:
pipefd[1] 참조: 부모(1) + pid1(1) + pid2(1) = 3

pid1이 dup2 + close 후:
pipefd[1] 참조: 부모(1) + pid2(1) = 2
                           ↑
                     (dup2로 STDOUT에 복사됐지만 원본 닫음)

pid1 실행 후 종료:
pipefd[1] 참조: 부모(1) = 1  ← 아직 1개 남음!

부모도 close:
pipefd[1] 참조: 0 → EOF 발생! grep 정상 종료
```

## 💡 배운 점

### Shell 파이프 구현 원리

**실제 Shell (`bash`, `zsh`)이 하는 일:**
```bash
$ ls | grep .c

Shell 내부:
1. pipe(pipefd)
2. fork() → ls
   - dup2(pipefd[1], 1)
   - exec("ls")
3. fork() → grep
   - dup2(pipefd[0], 0)
   - exec("grep .c")
4. close(pipefd[0]), close(pipefd[1])
5. wait(ls), wait(grep)
```

→ Shell은 이 과정을 자동화한 것!

### execlp() vs execvp()

```c
/* execlp: 인자를 직접 나열 */
execlp("ls", "ls", "-l", NULL);
//      ^프로그램  ^argv[0]  ^argv[1]  ^NULL 종료

/* execvp: 배열로 전달 */
char *args[] = {"ls", "-l", NULL};
execvp("ls", args);
```

**공통점:**
- PATH에서 실행파일 자동 탐색
- 성공하면 리턴 안함 (프로세스 교체)
- 실패하면 -1 리턴

### 다중 파이프 확장

```bash
$ ls | grep .c | wc -l
```

```c
/* Pipe 2개 필요 */
int pipe1[2]; // ls → grep
int pipe2[2]; // grep → wc

/* 프로세스 3개 */
fork() → ls:   dup2(pipe1[1], 1)
fork() → grep: dup2(pipe1[0], 0), dup2(pipe2[1], 1)
fork() → wc:   dup2(pipe2[0], 0)
```

→ minishell에서 구현한 파이프라인 로직과 동일!

## 📊 성능

```
프로세스 수: 3개 (부모 + ls + grep)
Pipe 버퍼:  64KB (기본)
실행 시간:  < 10ms (파일 수에 따라 다름)
```

**오버헤드:**
```
ls 출력 → pipe 버퍼 → grep 읽기
         (Kernel 경유: 2번 복사)
```

## 🔮 개선 가능 사항

- [ ] 인자로 명령어 받기 (`argv`로 pipe 구성)
- [ ] N개 파이프 지원 (`ls | grep | sort | head`)
- [ ] 에러 처리 강화 (exec 실패 시 exit code 전달)
- [ ] Non-blocking pipe + select()

## 📚 참고 자료

### Man Pages
```bash
man 2 pipe
man 2 dup2
man 3 execlp
man 2 fork
man 2 waitpid
```

### 관련 프로젝트
- [day1-pipe_basic](../day1-pipe_basic) - Pipe 기초 (read/write 직접)
- [day3-fifo](../day3-fifo) - Named Pipe (무관한 프로세스 간 통신)

## 🎯 다음 단계

1. ✅ [day1-pipe_basic](../day1-pipe_basic) - Pipe 기본
2. ✅ pipe_shell (현재) - Shell 파이프라인 구현
3. → [day3-fifo](../day3-fifo) - Named Pipe (FIFO)
4. → [day4-5-shared-memory](../day4-5-shared-memory) - 공유 메모리

## 📝 프로젝트 정보

```
개발 기간: Day 2
환경: Linux (Debian/Ubuntu)
언어: C
라이브러리: unistd.h, sys/wait.h
빌드: gcc, make
```

**파일 구조:**
```
day2-pipe_shell/
├── pipe_shell.c    # ls | grep .c 구현
└── Makefile        # 빌드 설정
```

**핵심 흐름:**
```
pipe() → fork(ls) → dup2(write→stdout) → exec(ls)
      → fork(grep) → dup2(read→stdin) → exec(grep)
      → 부모: close pipe → waitpid × 2
```

---

**Author:** OnePaperHoon
**Date:** February 2026
**Project:** Linux Kernel Study - Week 3, Day 2
**Topic:** Pipe Shell — dup2 + exec으로 파이프라인 구현
