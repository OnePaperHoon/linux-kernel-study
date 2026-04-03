# Week 5 Day 2: Module Parameters

`insmod` 시 외부에서 값을 주입하거나, 로드 후 `/sys`를 통해 런타임에 파라미터를 변경하는 방법을 익힌다.

---

## 개념 정리

### 모듈 파라미터란?

커널 모듈에 `insmod` 시 `key=value` 형태로 값을 전달하거나, 로드 이후 `/sys/module/<모듈명>/parameters/` 경로를 통해 런타임에 값을 읽고 쓸 수 있는 인터페이스다.

### `module_param()` 매크로

```c
module_param(변수명, 타입, 권한);
```

| 인자 | 설명 |
|------|------|
| 변수명 | 이미 선언된 `static` 변수 |
| 타입 | `int`, `bool`, `charp`(char*), `long`, `uint` 등 |
| 권한 | `/sys`에 노출되는 파일 퍼미션 |

권한 값:

| 값 | 의미 |
|----|------|
| `0000` | `/sys`에 노출하지 않음 |
| `0444` | 읽기 전용 (외부에서 값 변경 불가) |
| `0644` | 읽기/쓰기 (런타임 변경 가능) |

### Q. 왜 파라미터 변수를 `static`으로 선언하는가?

`module_param()`은 해당 변수의 주소를 커널 파라미터 테이블에 등록한다. `static`을 붙이면 다른 모듈과의 심볼 충돌을 방지하면서, 파일 스코프 내에서 변수의 생명 주기를 모듈 전체와 동일하게 유지한다. `insmod` 이후 파라미터 값이 지속적으로 참조되므로, 지역 변수처럼 스택에서 사라지면 안 된다.

### Q. 왜 `charp`이고 `char*`가 아닌가?

`module_param()`은 타입 인자를 문자열 이름으로 받는다 — C 타입 그 자체가 아니다. 내부적으로 타입별 파싱 함수(`param_get_*`, `param_set_*`)가 매핑되어 있고, `char*`는 포인터 타입이므로 `charp`(char pointer)라는 별칭을 사용한다.

```c
// 커널 내부 등록 구조
extern struct kernel_param_ops param_ops_charp;
extern struct kernel_param_ops param_ops_int;
extern struct kernel_param_ops param_ops_bool;
```

### Q. `/sys/module/params/parameters/`는 언제 생기는가?

`module_param()`의 세 번째 인자(권한)가 `0`이 아닌 경우에만 생성된다. 권한이 `0000`이면 `insmod` 시 값 주입은 가능하지만 `/sys`에는 노출되지 않아 런타임 확인/변경이 불가능하다.

### `MODULE_PARM_DESC`

```c
MODULE_PARM_DESC(변수명, "설명 문자열");
```

`modinfo params.ko` 실행 시 `parm:` 항목으로 출력된다. 사용자에게 파라미터 용도를 알리는 문서화 용도다.

### `KERN_DEBUG` 로그 레벨

```c
printk(KERN_DEBUG "[DEBUG] ...\n");
```

기본 콘솔 로그 레벨보다 낮아 일반 상황에서는 `dmesg`에 출력되지 않을 수 있다. `/proc/sys/kernel/printk`에서 로그 레벨을 조정하거나, `dmesg -x`로 레벨 포함 출력 시 확인 가능하다.

---

## 파일 구조

```
day2-params/
├── params.c    # 커널 모듈 소스
└── Makefile    # 빌드 및 관리 타겟
```

---

## 빌드 및 실행

### 빌드

```bash
make
```

### 기본 파라미터로 로드

```bash
make load
# 또는
sudo insmod params.ko
```

`count=1`, `name="OnepaperHoon"`, `debug=false`로 로드된다.

### 커스텀 파라미터로 로드

```bash
make load-custom
# 또는
sudo insmod params.ko count=3 name="TNS_Lab" debug=true
```

`insmod` 시 `key=value` 형태로 공백 구분하여 전달한다.

### 커널 로그 확인

```bash
dmesg | tail -15
```

`debug=true`로 로드한 경우 `[DEBUG]` 라인이 추가로 출력된다.

### `/sys`에서 파라미터 확인

```bash
make sysfs
# 또는
ls /sys/module/params/parameters/
cat /sys/module/params/parameters/count
```

### 런타임 파라미터 변경 (권한 `0644`)

```bash
make change-param
# 또는
echo 5 | sudo tee /sys/module/params/parameters/count
```

모듈을 언로드하지 않고도 값을 변경할 수 있다.

### 모듈 언로드

```bash
make unload
# 또는
sudo rmmod params
```

### 모듈 정보 확인

```bash
make info
# 또는
modinfo params.ko
```

`parm:` 항목에서 `MODULE_PARM_DESC`로 작성한 설명이 출력된다.

---

## Makefile 타겟 요약

| 타겟 | 설명 |
|------|------|
| `make` | 커널 모듈 빌드 |
| `make clean` | 빌드 산출물 제거 |
| `make load` | 기본 파라미터로 로드 |
| `make load-custom` | `count=3 name="TNS_Lab" debug=true`로 로드 |
| `make unload` | 모듈 언로드 |
| `make info` | `modinfo`로 파라미터 설명 확인 |
| `make sysfs` | `/sys/module/params/parameters/` 내용 출력 |
| `make change-param` | `count`를 5로 런타임 변경 |
| `make help` | 타겟 목록 출력 |

---

## 핵심 포인트

- `module_param()`은 변수 주소를 커널 파라미터 테이블에 등록하는 것이지, 값을 복사하는 게 아니다 — 그래서 `static` 변수여야 한다.
- 권한이 `0644`이면 `/sys`를 통해 모듈이 로드된 상태에서 실시간으로 값을 바꿀 수 있다.
- `charp`는 `char*`의 커널 파라미터 타입 별칭이다.
- `MODULE_PARM_DESC`는 동작에 영향을 주지 않고 `modinfo` 출력에만 반영되는 문서화 매크로다.
