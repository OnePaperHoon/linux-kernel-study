# Week 5 Day 4: Character Device Driver

커널에 문자 디바이스를 등록하고 `/dev` 파일을 통해 유저 공간과 읽기/쓰기로 통신하는 드라이버를 구현한다.

---

## 개념 정리

### 문자 디바이스(Character Device)란?

리눅스 디바이스는 크게 두 종류다.

| 종류 | 설명 | 예시 |
|------|------|------|
| 문자 디바이스 (char) | 바이트 스트림, 순차 접근 | 터미널, 시리얼 포트, 키보드 |
| 블록 디바이스 (block) | 블록 단위, 랜덤 접근 | 하드디스크, SSD |

문자 디바이스는 `/dev/` 아래 파일로 노출되며, `open` / `read` / `write` / `close` 시스템 콜이 드라이버의 함수로 매핑된다.

### 메이저/마이너 번호

```
ls -la /dev/tty0
crw--w---- 1 root tty 4, 0  ← 4: 메이저, 0: 마이너
```

| 번호 | 역할 |
|------|------|
| 메이저(major) | 어떤 드라이버가 처리할지 식별 |
| 마이너(minor) | 동일 드라이버 내 개별 디바이스 구분 |

`register_chrdev(0, name, &fops)`에서 첫 인자 `0`은 커널이 알아서 빈 메이저 번호를 할당하라는 의미다.

### `file_operations` 구조체

유저가 `/dev/foo`에 시스템 콜을 호출하면 커널이 이 구조체에서 해당 함수 포인터를 찾아 호출한다.

```c
static struct file_operations fops = {
    .owner   = THIS_MODULE,  // 모듈 참조 카운트 관리
    .open    = device_open,
    .release = device_release,
    .read    = device_read,
    .write   = device_write,
};
```

### Q. `struct inode`와 `struct file`의 차이는?

두 구조체 모두 `open` / `release` 핸들러 인자로 넘어오는데, 역할이 다르다.

| 구조체 | 역할 | 생명 주기 |
|--------|------|-----------|
| `struct inode` | 파일 자체의 메타데이터 (디바이스 번호, 권한 등) | 파일이 존재하는 동안 유일하게 1개 |
| `struct file` | `open()` 한 번의 세션 상태 (현재 오프셋, 플래그 등) | `open()` ~ `close()` 동안 유지 |

같은 파일을 두 프로세스가 동시에 열면 `inode`는 1개, `file`은 2개다. `device_read`에서 `*offset`을 추적하는 것이 `struct file` 안에 있는 이유다.

### Q. `loff_t *offset`은 무엇인가?

`loff_t`는 `long long offset`의 typedef다 — 파일 내 현재 읽기/쓰기 위치(바이트 단위).

```c
typedef long long loff_t;  // 64비트 파일 오프셋
```

`read()`나 `write()` 호출마다 커널이 이 포인터를 넘겨주고, 드라이버가 직접 값을 증가시켜야 한다. `*offset += bytes_read`를 하지 않으면 `cat`이 같은 위치를 무한 반복해서 읽는다.

32비트 시스템에서 `int`(4GB 제한)로는 대용량 파일을 다룰 수 없어 `long long`(8EB까지)을 사용한다.

### Q. `__user`는 무엇인가?

```c
static ssize_t device_read(struct file *file, char __user *user_buffer, ...)
```

`__user`는 이 포인터가 유저 공간 주소임을 나타내는 어노테이션이다. 실행에는 영향을 주지 않지만 `sparse`(커널 정적 분석 도구)가 직접 역참조(`*user_buffer`)를 경고로 잡아낸다. `copy_to_user()` / `copy_from_user()` 없이 접근하는 실수를 빌드 타임에 차단하는 안전 장치다.

### Q. `kmalloc(size, GFP_KERNEL)`의 `GFP_KERNEL`은 무엇인가?

`GFP`는 **Get Free Pages**의 약자로, 커널 메모리 할당 플래그다.

| 플래그 | 의미 | 사용 위치 |
|--------|------|-----------|
| `GFP_KERNEL` | 슬립 허용, 일반적인 커널 컨텍스트 | 모듈 초기화, 일반 커널 태스크 |
| `GFP_ATOMIC` | 슬립 불가, 즉시 할당 | 인터럽트 핸들러, 스핀락 내부 |
| `GFP_DMA` | DMA 가능한 메모리 영역 | 하드웨어 DMA 버퍼 |

`GFP_KERNEL`은 메모리가 부족하면 커널이 페이지를 회수할 때까지 **슬립(대기)**할 수 있다. 그래서 인터럽트 컨텍스트처럼 슬립이 불가능한 곳에서는 사용하면 안 된다.

```c
device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
if (!device_buffer)       // 할당 실패 시 NULL 반환
    return -ENOMEM;
```

### `try_module_get` / `module_put`

```c
try_module_get(THIS_MODULE);  // open 시 모듈 참조 카운트 +1
module_put(THIS_MODULE);      // release 시 참조 카운트 -1
```

디바이스가 열려 있는 동안 `rmmod`로 모듈이 제거되는 것을 방지한다. 참조 카운트가 0이 되어야 커널이 모듈을 언로드할 수 있다.

---

## 파일 구조

```
day4-chardev/
├── chardev_basic.c    # 커널 모듈 소스
└── Makefile           # 빌드 및 관리 타겟
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
sudo insmod chardev_basic.ko
```

로드 후 `dmesg`에서 할당된 메이저 번호를 확인한다.

```bash
dmesg | tail -10
# chardev: Registered with major number 240
```

### `/dev` 파일 생성

```bash
sudo mknod /dev/onepaperhoon_dev c <메이저번호> 0
sudo chmod 666 /dev/onepaperhoon_dev
```

`mknod` 인자: `c` = 문자 디바이스, `<메이저>`, `<마이너>`

### 쓰기 테스트

```bash
make test-write
# 또는
echo "Hello from OnePaperHoon!" | sudo tee /dev/onepaperhoon_dev
```

### 읽기 테스트

```bash
make test-read
# 또는
sudo cat /dev/onepaperhoon_dev
```

### `/dev` 파일 삭제

```bash
make remove-dev
# 또는
sudo rm -f /dev/onepaperhoon_dev
```

### 모듈 언로드

```bash
make unload
# 또는
sudo rmmod chardev_basic
```

---

## Makefile 타겟 요약

| 타겟 | 설명 |
|------|------|
| `make` | 커널 모듈 빌드 |
| `make clean` | 빌드 산출물 제거 |
| `make load` | 모듈 로드 + 메이저 번호 확인 |
| `make create-dev` | `/dev` 파일 생성 (메이저 번호 수동 입력) |
| `make test-write` | 디바이스에 문자열 쓰기 |
| `make test-read` | 디바이스에서 읽기 |
| `make remove-dev` | `/dev` 파일 삭제 |
| `make unload` | 모듈 언로드 |
| `make info` | `modinfo`로 모듈 정보 출력 |
| `make help` | 타겟 목록 출력 |

---

## 핵심 포인트

- `inode`는 파일 자체, `struct file`은 열린 세션 — 같은 파일을 두 번 열면 `inode` 1개 + `file` 2개.
- `loff_t *offset`은 드라이버가 직접 증가시켜야 한다 — 안 하면 EOF에 도달하지 못한다.
- `__user` 포인터는 반드시 `copy_to_user()` / `copy_from_user()`로만 접근해야 한다.
- `GFP_KERNEL`은 슬립 가능한 컨텍스트에서만 사용 — 인터럽트 핸들러에서는 `GFP_ATOMIC`.
- 모듈 언로드 전에 반드시 `/dev` 파일을 삭제하고 `rmmod`해야 한다. 디바이스가 열린 상태에서 `rmmod`하면 `try_module_get`이 막아준다.
