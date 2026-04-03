# Week 5 Day 1: Hello World Kernel Module

리눅스 커널 모듈의 기본 구조를 익히는 첫 번째 실습. `insmod`/`rmmod`로 모듈을 커널에 로드하고 언로드하는 생명 주기를 확인한다.

---

## 개념 정리

### 커널 모듈이란?

커널 모듈(Kernel Module)은 실행 중인 커널에 동적으로 추가하거나 제거할 수 있는 코드 조각이다. 커널을 재컴파일하지 않고도 디바이스 드라이버, 파일시스템, 시스템 콜 등을 추가할 수 있다.

- 확장자: `.ko` (Kernel Object)
- 커널 공간(ring 0)에서 실행 → 잘못된 코드는 커널 패닉 유발
- `lsmod`, `insmod`, `rmmod`, `modinfo` 명령어로 관리

### 모듈 생명 주기

```
insmod hello.ko
      │
      ▼
 hello_init()  ← __init 어노테이션, 로드 후 메모리 해제
      │
   [동작 중]
      │
      ▼
 hello_exit()  ← __exit 어노테이션, rmmod 시 호출
      │
      ▼
rmmod hello
```

### 주요 매크로 및 어노테이션

| 항목 | 설명 |
|------|------|
| `module_init(fn)` | 모듈 로드 시 호출할 함수 등록 |
| `module_exit(fn)` | 모듈 언로드 시 호출할 함수 등록 |
| `__init` | 초기화 후 메모리 해제 구역(`.init.text`)에 배치 |
| `__exit` | 모듈이 빌트인일 경우 컴파일러가 제거 |
| `MODULE_LICENSE` | 라이선스 선언 (GPL 필수 — 미선언 시 커널 오염 경고) |
| `MODULE_AUTHOR` | 작성자 정보 |
| `MODULE_DESCRIPTION` | 모듈 설명 |
| `MODULE_VERSION` | 버전 정보 |

### `static int __init` 선언 분석

```c
static int __init hello_init(void)
```

세 키워드가 각각 독립적인 이유로 붙는다.

#### `static` — 심볼 충돌 방지

커널은 수천 개의 모듈이 같은 주소 공간에 올라간다. `static` 없이 선언하면 다른 모듈의 동명 함수와 심볼 충돌이 날 수 있다. `module_init()`에 함수 포인터로 등록하기 때문에 외부에서 이름으로 직접 호출할 일도 없으므로 전역 심볼로 export할 필요가 없다.

#### `int` — 로드 성공/실패 반환

`module_init()`이 등록받는 함수 포인터 타입이 `int (*)(void)`로 정의되어 있다.

| 반환값 | 의미 |
|--------|------|
| `0` | 로드 성공 |
| 음수 (`-ENOMEM`, `-ENODEV` 등) | 로드 실패 → `insmod`가 에러로 처리 |

`void`로 선언하면 컴파일 타임에 타입 불일치 경고/에러가 발생한다.

#### `__init` — 초기화 후 메모리 회수

```c
#define __init  __section(".init.text")
```

링커가 `__init` 함수들을 `.init.text` 섹션에 모아둔다. 모든 초기화가 끝나면 커널이 이 섹션 전체를 해제한다. 한 번 실행하고 다시는 호출되지 않는 코드이므로 메모리를 계속 점유할 이유가 없다.

> `__exit`도 같은 원리. 모듈이 커널에 빌트인으로 컴파일되면(`obj-y`) rmmod가 불가능하므로 링커가 `__exit` 함수 자체를 바이너리에서 제거한다.

---

### printk와 로그 레벨

`printk`는 커널 공간의 `printf`다. 유저 공간의 `stdout`이 없으므로 커널 링 버퍼(ring buffer)에 기록한다.

```c
printk(KERN_INFO "메시지\n");   // 일반 정보
printk(KERN_WARNING "경고\n");  // 경고
printk(KERN_ERR "에러\n");      // 에러
```

출력 확인: `dmesg | tail -20`

---

## 파일 구조

```
day1-hello/
├── hello.c     # 커널 모듈 소스
└── Makefile    # 빌드 및 관리 타겟
```

---

## 빌드 및 실행

### 빌드

```bash
make
```

커널 빌드 시스템(`/lib/modules/$(uname -r)/build`)을 호출해 `hello.ko`를 생성한다.

### 모듈 로드

```bash
make load
# 또는
sudo insmod hello.ko
```

### 커널 로그 확인

```bash
dmesg | tail -15
```

출력 예시:
```
[ 1234.567890] ===================================
[ 1234.567891] Hello, Kernel Module!
[ 1234.567892] Week 5 Day 1: Module Basics
[ 1234.567893] Author: OnePaperHoon
[ 1234.567894] ===================================
```

### 모듈 언로드

```bash
make unload
# 또는
sudo rmmod hello
```

### 로드된 모듈 목록 확인

```bash
make list
# 또는
lsmod | grep hello
```

### 모듈 정보 확인

```bash
make info
# 또는
modinfo hello.ko
```

### 빌드 파일 정리

```bash
make clean
```

---

## Makefile 타겟 요약

| 타겟 | 설명 |
|------|------|
| `make` | 커널 모듈 빌드 (`hello.ko` 생성) |
| `make clean` | 빌드 산출물 제거 |
| `make load` | `insmod`로 모듈 로드 후 `dmesg` 출력 |
| `make unload` | `rmmod`로 모듈 언로드 후 `dmesg` 출력 |
| `make info` | `modinfo`로 모듈 메타데이터 출력 |
| `make list` | `lsmod`로 로드 상태 확인 |
| `make log` | 최근 커널 로그 30줄 출력 |
| `make help` | 타겟 목록 출력 |

---

## 핵심 포인트

- 커널 모듈은 `init` / `exit` 두 함수가 진입점이다.
- `__init`은 로드 완료 후 해당 메모리를 커널이 회수한다 — 초기화 코드의 메모리 절약 기법.
- `MODULE_LICENSE("GPL")` 없이는 커널이 "tainted(오염됨)" 상태로 표시된다.
- `printk` 출력은 `dmesg`로 확인하며, 유저 공간 터미널에는 직접 출력되지 않는다.
