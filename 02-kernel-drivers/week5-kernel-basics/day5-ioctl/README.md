# Week 5 Day 5: ioctl

커널 드라이버에 ioctl 인터페이스를 추가해 유저 공간에서 디바이스를 제어하는 방법을 학습한다.

---

## 개념 정리

### ioctl이란?

`ioctl`(Input/Output Control)은 `read`/`write`로 표현하기 어려운 디바이스 고유 제어 명령을 전달하는 시스템 콜이다.

```c
int ioctl(int fd, unsigned long request, ...);
```

| 인자 | 설명 |
|------|------|
| `fd` | 열린 디바이스 파일 디스크립터 |
| `request` | 명령 코드 (매직 넘버 + 번호 + 데이터 방향 + 크기 인코딩) |
| `...` | 명령에 따라 정수 또는 포인터 |

### ioctl 명령 코드 생성 매크로

커널은 명령 코드를 32비트 정수로 인코딩한다. 직접 상수를 쓰는 대신 아래 매크로를 사용한다.

```c
#define IOCTL_RESET     _IO  (MAGIC, nr)           // 데이터 전달 없음
#define IOCTL_GET_COUNT _IOR (MAGIC, nr, int)      // 커널 → 유저 (Read)
#define IOCTL_SET_MSG   _IOW (MAGIC, nr, char[256])// 유저 → 커널 (Write)
#define IOCTL_GET_MSG   _IOR (MAGIC, nr, char[256])// 커널 → 유저 (Read)
#define IOCTL_EXCHANGE  _IOWR(MAGIC, nr, int)      // 양방향 (Read+Write)
```

| 매크로 | 비트 필드 | 설명 |
|--------|-----------|------|
| `_IO` | type + nr | 데이터 없음 |
| `_IOR` | type + nr + size + direction(R) | 커널이 유저에게 데이터 반환 |
| `_IOW` | type + nr + size + direction(W) | 유저가 커널에 데이터 전달 |
| `_IOWR` | type + nr + size + direction(RW) | 양방향 교환 |

`_IOC_TYPE(cmd)`, `_IOC_NR(cmd)`로 커널 핸들러에서 매직 넘버와 번호를 검증할 수 있다.

### 매직 넘버(Magic Number)

```c
#define IOCTL_MAGIC 'k'
```

ioctl 명령 코드의 상위 8비트를 차지하는 고유 식별자다. 다른 드라이버의 명령 코드와 충돌하지 않도록 드라이버마다 다른 문자를 사용한다. 커널은 `_IOC_TYPE(cmd) != IOCTL_MAGIC`이면 `-ENOTTY`를 반환하게 한다.

> `Documentation/userspace-api/ioctl/ioctl-number.rst`에 커널 트리에서 이미 할당된 매직 번호 목록이 있다.

### Q. `-ENOTTY`를 반환하는 이유는?

ioctl 핸들러에서 알 수 없는 명령이 들어오면 관례적으로 `-ENOTTY`("Not a typewriter")를 반환한다. 이름은 역사적으로 터미널 제어에서 유래했지만, 오늘날에는 "이 디바이스는 이 ioctl을 지원하지 않는다"는 표준 오류 코드로 쓰인다.

### Q. `unlocked_ioctl`과 `ioctl`의 차이는?

```c
static struct file_operations fops = {
    .unlocked_ioctl = device_ioctl,
};
```

| 필드 | 커널 버전 | 특징 |
|------|-----------|------|
| `.ioctl` | 2.6.36 이전 | 호출 전 BKL(Big Kernel Lock) 자동 획득 |
| `.unlocked_ioctl` | 2.6.36 이후 | BKL 없이 호출 — 드라이버가 직접 동기화 책임 |

현대 커널에서는 `.unlocked_ioctl`만 사용한다.

### Q. `IOCTL_EXCHANGE`는 어떻게 양방향 전달을 하는가?

포인터 하나로 값을 읽은 뒤, 같은 포인터에 다른 값을 써서 반환한다.

```c
case IOCTL_EXCHANGE:
    copy_from_user(&value, (int __user *)arg, sizeof(int)); // 유저 값 읽기
    copy_to_user((int __user *)arg, &access_count, sizeof(int)); // 기존 카운트 반환
    access_count = value; // 새 값 적용
```

유저 입장에서는 `arg`에 보낼 값을 넣고 `ioctl` 호출 후 같은 변수에서 결과를 읽는다.

---

## 파일 구조

```
day5-ioctl/
├── chardev_ioctl.c    # 커널 모듈 — ioctl 핸들러 포함
├── ioctl_cmd.h        # ioctl 명령 코드 정의 (커널/유저 공유)
├── test_ioctl.c       # 유저 공간 테스트 프로그램
└── Makefile           # 모듈 + 유저 프로그램 빌드, 로드/언로드 타겟
```

---

## 빌드 및 실행

### 빌드

```bash
make
```

커널 모듈(`chardev_ioctl.ko`)과 유저 테스트 프로그램(`test_ioctl`)을 모두 빌드한다.

### 모듈 로드

```bash
make load
# 또는
sudo insmod chardev_ioctl.ko
```

로드 후 `dmesg`로 할당된 메이저 번호를 확인한다.

```bash
dmesg | tail -10
# ioctl_dev: Registered with major 240
# ioctl_dev: Create device:
#   sudo mknod /dev/ioctl_dev c 240 0
#   sudo chmod 666 /dev/ioctl_dev
```

### `/dev` 파일 생성

```bash
make create-dev
# 메이저 번호를 수동으로 입력하면 /dev/ioctl_dev 생성
```

또는 직접 실행:

```bash
sudo mknod /dev/ioctl_dev c <메이저번호> 0
sudo chmod 666 /dev/ioctl_dev
```

### 테스트

```bash
make test
# 또는
./test_ioctl
```

아래 순서로 ioctl 명령을 수행한다.

```
1. IOCTL_GET_COUNT   — 현재 접근 카운터 읽기
2. IOCTL_SET_MSG     — 커널 내 메시지 설정
3. IOCTL_GET_MSG     — 커널 내 메시지 읽기
4. IOCTL_EXCHANGE    — 카운터와 값 교환 (양방향)
5. IOCTL_GET_COUNT   — 교환 후 카운터 확인
6. IOCTL_RESET       — 카운터 초기화 + 메시지 리셋
7. IOCTL_GET_COUNT   — 리셋 후 카운터 확인
```

### 모듈 언로드

```bash
make unload
# sudo rmmod chardev_ioctl + /dev/ioctl_dev 삭제
```

---

## Makefile 타겟 요약

| 타겟 | 설명 |
|------|------|
| `make` | 커널 모듈 + 유저 프로그램 빌드 |
| `make module` | 커널 모듈만 빌드 |
| `make userspace` | 유저 테스트 프로그램만 빌드 |
| `make load` | 모듈 로드 + dmesg 출력 |
| `make create-dev` | `/dev/ioctl_dev` 파일 생성 |
| `make test` | 테스트 프로그램 실행 + dmesg 확인 |
| `make unload` | 모듈 언로드 + 디바이스 파일 삭제 |
| `make clean` | 빌드 산출물 제거 |
| `make help` | 타겟 목록 출력 |

---

## 핵심 포인트

- ioctl 명령 코드는 `_IO` / `_IOR` / `_IOW` / `_IOWR` 매크로로 생성한다 — 직접 상수를 쓰면 드라이버 간 충돌이 발생한다.
- 핸들러에서 `_IOC_TYPE(cmd) != IOCTL_MAGIC`으로 매직 넘버를 검증해 엉뚱한 명령을 차단한다.
- 알 수 없는 명령은 `-ENOTTY`를 반환하는 것이 POSIX 관례다.
- `ioctl_cmd.h`는 커널 모듈과 유저 프로그램이 함께 포함한다 — 명령 코드를 한 곳에서 관리해 불일치를 방지한다.
- `unlocked_ioctl`을 쓰면 BKL 없이 호출되므로, 공유 상태(`access_count`, `message`)에 대한 동기화는 드라이버가 직접 책임져야 한다 (이 예제는 단순 학습용이라 락 없음).
