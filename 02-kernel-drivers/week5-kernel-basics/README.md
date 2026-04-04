# Week 5: Kernel Basics

커널 모듈의 기초부터 문자 디바이스 드라이버까지 단계적으로 학습한다.

## 학습 목차

| Day | 주제 | 디렉토리 |
|-----|------|---------|
| Day 1 | Hello World 모듈 — 모듈 생명 주기, `__init`/`__exit` | `day1-hello/` |
| Day 2 | 모듈 파라미터 — `module_param`, `/sys` 런타임 변경 | `day2-params/` |
| Day 3 | `/proc` 파일시스템 — 가상 파일 생성, `seq_file`, `copy_from_user` | `day3-proc/` |
| Day 4 | 문자 디바이스 드라이버 — `file_operations`, 메이저/마이너 번호 | `day4-chardev/` |

---

## 몰랐던 개념 정리

### `struct inode`

파일 자체의 메타데이터를 담는 구조체다. 파일명이 아니라 파일 시스템 내 실제 객체를 나타낸다.

```c
static int device_open(struct inode *inode, struct file *file)
```

- 파일 하나당 `inode`는 딱 1개 존재
- 디바이스 번호(`i_rdev`), 파일 권한, 소유자 등을 담고 있음
- 같은 파일을 10개 프로세스가 열어도 `inode`는 여전히 1개

> `inode`는 파일의 "정체성", `struct file`은 파일을 "열어둔 상태"

---

### `struct file`

`open()` 한 번의 세션 상태를 담는 구조체다.

```c
static ssize_t device_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
```

| 주요 필드 | 내용 |
|-----------|------|
| `f_pos` | 현재 파일 오프셋 (= `loff_t *offset`의 원본) |
| `f_flags` | `O_RDONLY`, `O_NONBLOCK` 등 open 플래그 |
| `f_mode` | 읽기/쓰기 권한 |
| `private_data` | 드라이버가 자유롭게 쓸 수 있는 포인터 |

같은 파일을 두 프로세스가 열면 `inode` 1개 + `struct file` 2개. 각자 `f_pos`(오프셋)를 독립적으로 관리한다.

---

### `loff_t`

```c
typedef long long loff_t;  // 64비트 파일 오프셋
```

`read()` / `write()` 핸들러에서 현재 파일 위치(바이트 단위)를 나타낸다.

**왜 `int`나 `long`이 아닌가?**
- 32비트 `int`는 최대 2GB, `long`은 32비트 시스템에서 4GB까지만 표현 가능
- `long long`(64비트)은 최대 8EB(엑사바이트)까지 표현 가능 → 대용량 파일 지원

드라이버에서 직접 증가시켜야 한다:

```c
*offset += bytes_to_read;  // 안 하면 cat이 같은 위치를 무한 반복
```

---

### `__user`

```c
static ssize_t device_read(struct file *file, char __user *user_buffer, ...)
```

이 포인터가 **유저 공간 주소**임을 표시하는 컴파일러 어노테이션이다. 런타임 동작에는 영향이 없다.

**역할:**
- `sparse`(커널 정적 분석 도구)가 `__user` 포인터를 직접 역참조하면 경고 발생
- `copy_to_user()` / `copy_from_user()` 없이 접근하는 실수를 빌드 타임에 잡아냄

**왜 직접 접근하면 안 되는가?**

유저 공간과 커널 공간은 가상 주소 공간이 분리되어 있다. 커널에서 유저 포인터를 그냥 역참조하면:
1. 해당 페이지가 스왑 아웃되어 있을 수 있음
2. 악의적인 주소를 넘겨 커널 메모리를 읽게 만들 수 있음 (보안 취약점)

`copy_to_user()` / `copy_from_user()`는 페이지 존재 여부 확인 + 안전한 복사를 보장한다.

---

### `GFP_KERNEL` (Get Free Pages)

```c
device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
```

`GFP`는 **Get Free Pages**의 약자로, 커널 메모리 할당 방식을 지정하는 플래그다.

| 플래그 | 슬립 가능 | 사용 위치 |
|--------|-----------|-----------|
| `GFP_KERNEL` | 가능 | 일반 커널 컨텍스트, 모듈 초기화 |
| `GFP_ATOMIC` | 불가 | 인터럽트 핸들러, 스핀락 보호 구간 |
| `GFP_DMA` | 가능 | DMA 가능 메모리 영역 필요 시 |

**`GFP_KERNEL`이 슬립을 허용하는 이유:**

메모리가 부족하면 커널이 페이지 회수(page reclaim)를 수행해야 한다. 이 과정에서 I/O가 발생할 수 있어 태스크가 잠시 슬립 상태가 된다. 인터럽트 핸들러는 슬립할 수 없으므로 그 안에서 `GFP_KERNEL`을 쓰면 커널 패닉이 발생한다 → 그럴 때는 `GFP_ATOMIC`.

```c
// 할당 실패 처리는 필수
device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
if (!device_buffer)
    return -ENOMEM;

// 해제는 kfree()
kfree(device_buffer);
```

`kmalloc`은 유저 공간의 `malloc`에 대응하지만, 연속된 물리 메모리를 보장한다는 점이 다르다.
