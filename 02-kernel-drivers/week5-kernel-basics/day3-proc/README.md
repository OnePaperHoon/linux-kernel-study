# Week 5 Day 3: /proc Filesystem

커널 모듈에서 `/proc` 파일시스템에 가상 파일을 생성하고, 유저 공간과 읽기/쓰기로 통신하는 방법을 익힌다.

---

## 개념 정리

### `/proc` 파일시스템이란?

`/proc`는 디스크에 실제로 존재하지 않는 **가상 파일시스템(Virtual File System)**이다. 커널이 메모리 위에 만들어 놓은 인터페이스로, `cat`이나 `echo`로 접근하면 커널 함수가 호출되어 데이터를 생성하거나 명령을 처리한다.

```
유저 공간          커널 공간
cat /proc/foo  →  proc_show() 호출 → seq_printf()로 데이터 생성
echo x > /proc/foo  →  proc_write() 호출 → 명령 처리
```

### Q. 왜 일반 `file_operations` 대신 `proc_ops`를 쓰는가?

커널 5.6부터 `/proc` 전용 `struct proc_ops`가 도입됐다. 기존 `file_operations`는 모든 파일 타입을 위한 범용 구조체라 `/proc`에 불필요한 필드가 많다. `proc_ops`는 `/proc` 전용으로 최적화되어 있고, `file_operations`를 사용하면 5.6 이상 커널에서 컴파일 에러가 발생한다.

```c
// 커널 5.6+
static const struct proc_ops proc_fops = {
    .proc_open    = proc_open,
    .proc_read    = seq_read,
    .proc_write   = proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
```

### Q. `seq_file`이란 무엇인가?

`/proc` 파일을 읽을 때 단순히 문자열을 반환하면 대용량 데이터에서 페이지 경계 문제가 생긴다. `seq_file`은 커널이 제공하는 순차 출력 인터페이스로, 버퍼 관리를 자동으로 처리해준다.

| 함수 | 역할 |
|------|------|
| `single_open(file, show_fn, data)` | 단일 페이지 출력에 최적화된 `seq_file` 초기화 |
| `seq_printf(m, fmt, ...)` | `seq_file` 버퍼에 포맷 문자열 출력 |
| `seq_read` | `seq_file`의 표준 read 핸들러 (직접 구현 불필요) |
| `single_release` | `single_open`에 대응하는 release 핸들러 |

### Q. `copy_from_user()`가 왜 필요한가?

유저 공간과 커널 공간은 메모리 주소 공간이 분리되어 있다. 유저가 `echo reset > /proc/foo`로 데이터를 쓰면, 그 데이터는 유저 공간 메모리에 있다. 커널에서 직접 포인터로 접근하면 페이지 폴트나 보안 문제가 생긴다. `copy_from_user()`는 유저 공간 → 커널 공간으로 안전하게 데이터를 복사한다.

```c
if (copy_from_user(cmd, buffer, count))
    return -EFAULT;  // 복사 실패 시 에러 반환
```

반대로 커널 → 유저 공간 복사는 `copy_to_user()`를 사용한다. `seq_printf()`는 내부적으로 이를 처리해주므로 직접 쓸 일이 없다.

### Q. `__user` 어노테이션은 무슨 역할인가?

```c
static ssize_t proc_write(struct file *file, const char __user *buffer, ...)
```

`__user`는 컴파일러 어노테이션으로 실행에 영향을 주지 않는다. `sparse`(커널 정적 분석 도구)가 이 포인터를 유저 공간 주소로 인식해, 직접 역참조(`*buffer`)하면 경고를 발생시킨다. `copy_from_user()` 없이 직접 접근하는 실수를 빌드 타임에 잡아주는 안전 장치다.

### `jiffies`

```c
seq_printf(m, "Current jiffies: %lu\n", jiffies);
```

`jiffies`는 커널 부팅 이후 타이머 인터럽트가 발생한 횟수다. 1초에 `HZ`번 증가한다(일반적으로 250 또는 1000). 커널에서 고해상도 시간 대신 간단한 경과 시간 측정에 쓰인다.

### `proc_create_data()` vs `proc_create()`

```c
proc_entry = proc_create_data(PROC_NAME, 0666, NULL, &proc_fops, NULL);
```

| 함수 | 차이 |
|------|------|
| `proc_create(name, mode, parent, fops)` | 기본 생성 |
| `proc_create_data(name, mode, parent, fops, data)` | `data` 포인터를 `file->private_data`에 저장, `proc_show`에서 `v`로 받을 수 있음 |

이 예제에서는 `data`를 `NULL`로 넘기지만, 여러 `/proc` 엔트리가 하나의 핸들러를 공유할 때 컨텍스트를 전달하는 데 활용한다.

---

## 파일 구조

```
day3-proc/
├── proc_basic.c    # 커널 모듈 소스
└── Makefile        # 빌드 및 관리 타겟
```

---

## 빌드 및 실행

### 빌드

```bash
make
```

### 모듈 로드

```bash
make load
# 또는
sudo insmod proc_basic.ko
```

로드 성공 시 `/proc/onepaperhoon_info` 파일이 생성된다.

### `/proc` 파일 읽기

```bash
make read
# 또는
cat /proc/onepaperhoon_info
```

출력 예시:
```
====================================
OnePaperHoon Kernel Module Info
====================================
Access count: 1
Current jiffies: 4298765432
Module: proc_basic
====================================
```

`cat`을 반복 실행할수록 `Access count`가 증가한다.

### `/proc` 파일 상태 확인

```bash
make check
```

파일 존재 여부와 내용을 함께 출력한다.

### 카운터 리셋

```bash
make reset
# 또는
echo reset | sudo tee /proc/onepaperhoon_info > /dev/null
```

`proc_write()`에서 `"reset"` 문자열을 감지해 `access_count`를 0으로 초기화한다.

### 모듈 언로드

```bash
make unload
# 또는
sudo rmmod proc_basic
```

언로드 시 `remove_proc_entry()`로 `/proc/onepaperhoon_info`가 삭제되고, 총 접근 횟수가 `dmesg`에 출력된다.

---

## Makefile 타겟 요약

| 타겟 | 설명 |
|------|------|
| `make` | 커널 모듈 빌드 |
| `make clean` | 빌드 산출물 제거 |
| `make load` | 모듈 로드 + `dmesg` 출력 |
| `make unload` | 모듈 언로드 + `dmesg` 출력 |
| `make check` | `/proc` 파일 존재 확인 + 내용 출력 |
| `make read` | `/proc` 파일 읽기 |
| `make reset` | 카운터 리셋 |
| `make info` | `modinfo`로 모듈 정보 출력 |
| `make log` | 최근 커널 로그 20줄 출력 |
| `make help` | 타겟 목록 출력 |

---

## 핵심 포인트

- `/proc` 파일은 디스크에 없다 — `cat`하면 커널 함수(`proc_show`)가 호출되어 데이터를 즉석에서 생성한다.
- 커널 5.6부터 `file_operations` 대신 `proc_ops`를 써야 한다.
- 유저 공간 → 커널 공간 데이터 이동은 반드시 `copy_from_user()`를 거쳐야 한다. 직접 포인터 역참조는 커널 패닉 또는 보안 취약점이다.
- `__user` 어노테이션은 `sparse` 정적 분석 도구가 잘못된 직접 접근을 감지하기 위한 마커다.
- 모듈 언로드 시 `remove_proc_entry()`를 반드시 호출해야 한다 — 누락 시 모듈이 제거된 후에도 `/proc` 엔트리가 남아 `cat` 시 커널 패닉이 발생한다.
