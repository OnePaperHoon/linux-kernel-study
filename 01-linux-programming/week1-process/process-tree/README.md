# Process Tree Visualizer

/proc 파일시스템을 활용한 프로세스 트리 시각화 도구

## 📋 목차

- [실행 화면](#실행-화면)
- [빌드 및 실행](#빌드-및-실행)
- [프로젝트 개요](#프로젝트-개요)
- [구현 내용](#구현-내용)
- [배운 점](#배운-점)
- [성능](#성능)
- [개선 가능 사항](#개선-가능-사항)
- [참고 자료](#참고-자료)

## 🖥️ 실행 화면

```
$ ./pstree
Found 125 processes

PID    PPID   S   CMD
----------------------------------
1      0      S   systemd
├─ 287    1      S   systemd-journal
├─ 329    1      S   systemd-timesyn
├─ 342    1      S   systemd-udevd
├─ 580    1      S   cron
├─ 581    1      S   dbus-daemon
├─ 620    1      S   dhcpcd
│  ├─ 622    620    S   dhcpcd
│  │  ├─ 664    622    S   dhcpcd
│  │  ├─ 665    622    S   dhcpcd
│  │  └─ 880    622    S   dhcpcd
│  └─ 623    620    S   dhcpcd
├─ 645    1      S   sshd
│  └─ 117581 645    S   sshd-session
│     └─ 117626 117581 S   sshd-session
│        └─ 117627 117626 S   bash
└─ 676    1      S   systemd
```

## 🚀 빌드 및 실행

### 기본 빌드
```bash
make              # 컴파일
make re           # 재컴파일
make clean        # 오브젝트 파일 삭제
make fclean       # 전체 삭제
```

### 실행
```bash
make test         # 빌드 후 실행
./pstree          # 직접 실행
```

### 테스트
```bash
make valgrind     # 메모리 누수 체크
make check        # 빠른 메모리 체크
make count        # 프로세스 수만 출력
```

### 디버그
```bash
make debug        # 디버그 심볼 포함 빌드
make sanitize     # Address Sanitizer 활성화
```

## 📖 프로젝트 개요

### 목적
- Linux 시스템 프로그래밍 학습
- /proc 파일시스템 이해
- 트리 자료구조 구현
- 메모리 관리 연습

### 학습 목표
✅ `/proc` 파일시스템 활용  
✅ 파일 I/O (`opendir`, `readdir`, `fopen`)  
✅ 동적 메모리 관리 (`malloc`, `realloc`, `free`)  
✅ 트리 자료구조 구현  
✅ 재귀 알고리즘  
✅ 메모리 디버깅 (`valgrind`)

## 🔧 구현 내용

### 1. /proc 파일시스템 파싱

Linux의 `/proc`은 커널 정보를 파일 형태로 제공하는 가상 파일시스템입니다.

**사용한 파일:**
- `/proc/[pid]/stat` - 프로세스 상태 정보
- `/proc/[pid]/cmdline` - 명령어 라인 (선택적)

**stat 파일 형식:**
```
PID (COMM) STATE PPID PGRP SESSION TTY_NR ...
1234 (bash) S 1233 1234 1234 34816 ...
```

**파싱 코드:**
```c
FILE *fp = fopen("/proc/[pid]/stat", "r");
fgets(line, sizeof(line), fp);
sscanf(line, "%d %s %c %d", &pid, comm, &state, &ppid);
```

### 2. 자료구조 설계

```c
typedef struct process {
    pid_t pid;                      // 프로세스 ID
    pid_t ppid;                     // 부모 프로세스 ID
    char state;                     // 상태 (R/S/D/Z/T)
    char comm[256];                 // 명령어 이름
    struct process *parent;         // 부모 포인터
    struct process **children;      // 자식 배열 (동적)
    int num_children;               // 현재 자식 수
    int children_capacity;          // 배열 용량
} process_t;
```

**동적 배열 관리:**
- 초기 용량: 10
- 가득 차면 2배로 확장 (`realloc`)
- 자식 추가 시 자동 확장

```c
void process_add_child(process_t *parent, process_t *child)
{
    if (parent->num_children >= parent->children_capacity) {
        parent->children_capacity *= 2;
        parent->children = realloc(parent->children,
            parent->children_capacity * sizeof(process_t *));
    }
    parent->children[parent->num_children++] = child;
    child->parent = parent;
}
```

### 3. 트리 구축 알고리즘

**단계:**
1. `/proc` 디렉토리 읽기
2. 숫자 디렉토리(PID)만 필터링
3. 각 프로세스 정보 읽기
4. 부모-자식 관계 연결
5. 재귀적으로 출력

```c
// 1. /proc 읽기
DIR *dir = opendir("/proc");
while ((entry = readdir(dir)) != NULL) {
    if (isdigit(entry->d_name[0])) {
        pid = atoi(entry->d_name);
        process = process_create(pid);
        process_read_stat(process);
    }
}

// 2. 트리 구축
for each process:
    parent = find_process_by_pid(process->ppid);
    if (parent) {
        process_add_child(parent, process);
    }

// 3. 출력
print_tree(init_process, depth=0);
```

### 4. 출력 형식

**트리 표현:**
```
├─  - 중간 자식
└─  - 마지막 자식
│   - 수직선
```

**재귀 함수:**
```c
void print_tree(process_t *proc, int depth, int is_last[])
{
    // 들여쓰기 출력
    for (int i = 0; i < depth - 1; i++) {
        printf(is_last[i] ? "   " : "│  ");
    }
    
    // 브랜치 출력
    if (depth > 0) {
        printf(is_last[depth-1] ? "└─ " : "├─ ");
    }
    
    // 프로세스 정보
    printf("%-6d %-6d %c  %s\n", 
           proc->pid, proc->ppid, proc->state, proc->comm);
    
    // 자식들 재귀 출력
    for (int i = 0; i < proc->num_children; i++) {
        is_last[depth] = (i == proc->num_children - 1);
        print_tree(proc->children[i], depth + 1, is_last);
    }
}
```

### 5. 메모리 관리

**해제 전략:**
```c
void cleanup(process_list_t *list)
{
    // 1. children 배열 먼저 해제
    for (int i = 0; i < list->count; i++) {
        free(list->processes[i]->children);
    }
    
    // 2. 프로세스 구조체 해제
    for (int i = 0; i < list->count; i++) {
        free(list->processes[i]);
    }
    
    // 3. 리스트 해제
    free(list);
}
```

**검증 (Valgrind):**
```bash
$ valgrind --leak-check=full ./pstree

==129572== HEAP SUMMARY:
==129572==     in use at exit: 0 bytes in 0 blocks
==129572==   total heap usage: 507 allocs, 507 frees
==129572== 
==129572== All heap blocks were freed -- no leaks are possible
==129572== ERROR SUMMARY: 0 errors from 0 contexts
```

✅ **메모리 누수 0**  
✅ **507 allocs = 507 frees**  
✅ **Use-after-free 없음**

## 💡 배운 점

### /proc 파일시스템

**개념:**
- 커널 정보를 user space에서 접근하는 인터페이스
- 실제 디스크가 아닌 메모리 기반 가상 파일시스템
- 텍스트 기반으로 파싱이 필요

**주요 디렉토리:**
```bash
/proc/[pid]/stat      # 프로세스 상태
/proc/[pid]/status    # 상세 정보 (읽기 쉬움)
/proc/[pid]/cmdline   # 명령어 라인
/proc/[pid]/exe       # 실행파일 심볼릭 링크
/proc/[pid]/fd/       # 열린 파일 디스크립터
/proc/[pid]/task/     # 스레드 정보
```

**활용:**
- `ps`, `top`, `htop` 같은 도구들이 모두 `/proc` 사용
- 시스템 모니터링의 기본

### 프로세스 관계

**트리 구조:**
- 모든 프로세스는 부모를 가짐 (PPID)
- init (PID 1)이 최상위 부모
- PPID 0은 커널 프로세스

**프로세스 상태:**
- `R` (Running) - 실행 중
- `S` (Sleeping) - 대기 중
- `D` (Disk sleep) - I/O 대기
- `Z` (Zombie) - 종료했지만 정리 안됨
- `T` (Stopped) - 일시 정지

### C 프로그래밍 기법

**1. 디렉토리 탐색**
```c
DIR *dir = opendir("/proc");
struct dirent *entry;
while ((entry = readdir(dir)) != NULL) {
    // entry->d_name 사용
}
closedir(dir);
```

**2. 동적 메모리 관리**
```c
// 할당
ptr = malloc(size);
ptr = calloc(count, size);  // 0으로 초기화

// 재할당
ptr = realloc(ptr, new_size);

// 해제
free(ptr);
```

**3. 문자열 파싱**
```c
// sscanf - 형식 지정 파싱
sscanf(line, "%d %s %c %d", &pid, comm, &state, &ppid);

// strtok - 구분자로 분리
token = strtok(str, " \t\n");
```

**4. 재귀 함수**
```c
// Base case
if (condition)
    return;

// Recursive case
for each child:
    recursive_function(child);
```

### 디버깅 도구

**Valgrind:**
```bash
# 메모리 누수
valgrind --leak-check=full ./program

# 모든 메모리 문제
valgrind --leak-check=full --show-leak-kinds=all ./program

# 상세 추적
valgrind --track-origins=yes ./program
```

**일반적인 메모리 문제:**
- Memory leak (해제 안함)
- Double free (두 번 해제)
- Use after free (해제 후 사용)
- Invalid read/write (범위 밖 접근)

## 📊 성능

### 측정 결과
```
프로세스 수: 125
실행 시간: ~5ms
메모리 사용: ~100KB
메모리 누수: 0 bytes
할당/해제: 507/507 (완벽 매칭)
```

### 시간 복잡도
- 프로세스 읽기: O(n) - n은 프로세스 수
- 트리 구축: O(n)
- 트리 출력: O(n)
- **전체: O(n)**

### 공간 복잡도
- 프로세스 구조체: O(n)
- 자식 배열: O(n)
- **전체: O(n)**

## 🔮 개선 가능 사항

### 기능 추가
- [ ] 명령줄 옵션
  - `--pid [PID]` - 특정 PID만 표시
  - `--threads` - 스레드도 표시 (`/proc/[pid]/task`)
  - `--user [USER]` - 특정 사용자 프로세스만
  - `--help` - 도움말

- [ ] 추가 정보 표시
  - CPU 사용률 (`/proc/[pid]/stat` 13-14번 필드)
  - 메모리 사용량 (`/proc/[pid]/status` VmRSS)
  - 실행 시간
  - 우선순위

- [ ] 출력 개선
  - 색상 출력 (ANSI escape codes)
  - 정렬 옵션 (CPU, 메모리, PID)
  - JSON 출력 옵션
  - 실시간 업데이트 (watch 모드)

### 코드 개선
- [ ] 에러 처리 강화
  - 권한 없는 프로세스 처리
  - 프로세스가 중간에 종료된 경우
  
- [ ] 성능 최적화
  - 해시 테이블로 PID 검색 O(1)
  - 프로세스 캐싱

- [ ] 테스트
  - 단위 테스트 추가
  - 스트레스 테스트 (많은 프로세스)

## 📚 참고 자료

### Man Pages
```bash
man 5 proc        # /proc 파일시스템
man 3 readdir     # 디렉토리 읽기
man 3 opendir     # 디렉토리 열기
man 1 ps          # ps 명령어
man 1 pstree      # pstree 명령어
```

### 관련 명령어
```bash
# 시스템 기본 명령어
ps auxf              # 프로세스 트리
pstree              # 간단한 트리
htop                # 인터랙티브 모니터

# /proc 정보 확인
cat /proc/[pid]/stat
cat /proc/[pid]/status
ls -l /proc/[pid]/
```

### 문서
- The Linux Programming Interface (TLPI) - Chapter 12
- Linux Kernel Documentation - `/proc` interface
- Advanced Programming in the UNIX Environment (APUE)

### 온라인 자료
- [Linux Kernel Docs - proc.txt](https://www.kernel.org/doc/Documentation/filesystems/proc.txt)
- [man7.org - proc(5)](https://man7.org/linux/man-pages/man5/proc.5.html)

## 🎯 다음 단계

### Week 2 예정
- **Multi-threading** (pthread)
- **Thread pool** 구현
- **동기화** (mutex, semaphore)
- **Thread-safe** 자료구조

### 이 프로젝트 확장
Thread pool을 적용하면:
- 프로세스 읽기를 병렬 처리
- 여러 프로세스 동시 파싱
- 성능 개선 가능

## 📝 프로젝트 정보

**개발 기간:** Week 1 (2일)  
**개발 환경:** Debian Linux, GCC  
**언어:** C  
**라인 수:** ~400 lines  

**파일 구조:**
```
process-tree/
├── process.h       # 자료구조 정의
├── process.c       # 프로세스 관리
├── pstree.c        # 메인 로직
├── Makefile        # 빌드 설정
└── README.md       # 문서
```

---

**Author:** OnepaperHoon\
**Date:** January 2025  
**Project:** Linux Kernel Study - Week 1  
**Repository:** [linux-kernel-study](https://github.com/OnePaperHoon/linux-kernel-study)