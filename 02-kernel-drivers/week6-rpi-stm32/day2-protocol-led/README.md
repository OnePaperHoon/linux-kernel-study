# Day 2 - UART 프로토콜 설계 & LED 제어 (STM32 펌웨어)

커스텀 UART 프로토콜을 설계하고, STM32에서 패킷 파싱 상태 머신을 구현하여 시리얼 명령으로 LED를 제어한다.

## 📋 목차

- [실행 화면](#-실행-화면)
- [빌드 및 실행](#-빌드-및-실행)
- [프로젝트 개요](#-프로젝트-개요)
- [구현 내용](#-구현-내용)
- [배운 점](#-배운-점)
- [성능](#-성능)
- [개선 가능 사항](#-개선-가능-사항)
- [참고 자료](#-참고-자료)
- [프로젝트 정보](#-프로젝트-정보)

## 🖥️ 실행 화면

```
❯ python3 test.py
[*] Connecting to /dev/cu.usbmodem1403 @ 115200...
[1] LED ON
  TX: 02 01 00 01 03
  RX: 02 aa 01 01 aa 03
[2] LED OFF
  TX: 02 02 00 02 03
  RX: 02 aa 01 02 a9 03
[3] LED TOGGLE
  TX: 02 03 00 03 03
  RX: 02 aa 01 03 a8 03
[4] LED TOGGLE
  TX: 02 03 00 03 03
  RX: 02 aa 01 03 a8 03
[*] 테스트 완료!
```

- LED ON → 보드 LD2(PA5) 점등 확인
- LED OFF → 소등 확인
- TOGGLE × 2 → ON/OFF 교대 확인
- 모든 명령에 ACK 패킷 정상 수신

## 🚀 빌드 및 실행

### STM32 펌웨어

```bash
# STM32CubeIDE에서 프로젝트 열기
# Project → Build All (Ctrl+B)
# Run → Debug (F11) 또는 Run (Ctrl+F11)
```

### CubeIDE .ioc 설정

1. **Board Selector** → NUCLEO-F446RE
2. **USART2** → Asynchronous, 115200 / 8N1
3. **NVIC** → USART2 global interrupt **Enable**
4. **PA5** → GPIO_Output (온보드 LED)

### Python 테스트

```bash
# pyserial 설치
pip3 install pyserial --break-system-packages

# 포트 확인
ls /dev/cu.usbmodem*

# 테스트 실행 (screen/minicom 세션 종료 후)
killall screen
python3 test/send_cmd.py
```

## 📖 프로젝트 개요

### 목적

Day 1에서 구현한 raw UART 통신 위에 **구조화된 프로토콜 계층**을 올려, 명령-응답 방식의 통신 체계를 구축한다. 나중에 라즈베리파이 커널 드라이버에서 ioctl로 이 프로토콜을 사용하여 STM32를 제어하는 것이 최종 목표이다.

### 학습 목표

- [x] 바이너리 프로토콜 설계 (프레임 구조, 체크섬)
- [x] 상태 머신(State Machine) 기반 패킷 파서 구현
- [x] UART 인터럽트 수신 + 콜백 처리
- [x] ACK 응답 메커니즘 구현
- [ ] 라즈베리파이 커널 드라이버 연동 (Day 2 후반부)

## 🔧 구현 내용

### 1. 프로토콜 설계

호스트(PC/라즈베리파이)와 STM32 간 통신을 위한 바이너리 패킷 프로토콜이다.

**패킷 구조:**

```
[START][CMD][LEN][DATA...][CHECKSUM][END]
  0x02   1B   1B  0~8B      1B     0x03
```

| 필드 | 크기 | 설명 |
|------|------|------|
| START | 1 byte | 패킷 시작 마커 `0x02` (STX) |
| CMD | 1 byte | 명령 코드 |
| LEN | 1 byte | DATA 길이 (0~8) |
| DATA | 0~8 bytes | 명령 파라미터 |
| CHECKSUM | 1 byte | `CMD ^ LEN ^ DATA[0] ^ ... ^ DATA[N-1]` |
| END | 1 byte | 패킷 종료 마커 `0x03` (ETX) |

**명령어 정의:**

```c
#define CMD_LED_ON      0x01  // LED 켜기
#define CMD_LED_OFF     0x02  // LED 끄기
#define CMD_LED_TOGGLE  0x03  // LED 토글
#define CMD_ACK         0xAA  // 응답 (DATA에 원래 CMD 포함)
```

**체크섬 계산 예시 — LED ON 패킷:**

```
CMD=0x01, LEN=0x00
Checksum = 0x01 ^ 0x00 = 0x01
패킷: 02 01 00 01 03
```

**ACK 응답 예시 — LED ON에 대한 ACK:**

```
CMD=0xAA, LEN=0x01, DATA=0x01(원래 명령)
Checksum = 0xAA ^ 0x01 ^ 0x01 = 0xAA
패킷: 02 AA 01 01 AA 03
```

### 2. 상태 머신 패킷 파서

바이트 단위로 수신되는 UART 데이터를 패킷으로 조립하는 상태 머신이다. 인터럽트 컨텍스트에서 1바이트씩 `parser_feed()`에 넣으면 패킷이 완성되었을 때 알려준다.

**상태 전이 흐름:**

```
WAIT_START → GET_CMD → GET_LEN → GET_DATA(반복) → GET_CHECKSUM → GET_END
     ↑                    │                                          │
     │                    └─ LEN==0이면 GET_DATA 건너뜀              │
     └───────────────────── 완료 또는 에러 시 복귀 ──────────────────┘
```

**핵심 구현 — `parser_feed()`:**

```c
// 바이트 하나씩 먹여주는 상태 머신
// 반환: 1 = 유효한 패킷, 0 = 수집 중, -1 = 에러
static inline int parser_feed(PacketParser *p, uint8_t byte) {
    switch (p->state) {
    case STATE_WAIT_START:
        if (byte == PACKET_START) {
            p->data_idx = 0;
            p->ready    = 0;
            p->state    = STATE_GET_CMD;
        }
        return 0;

    case STATE_GET_LEN:
        p->pkt.len = byte;
        if (byte > MAX_DATA_LEN) {
            p->state = STATE_WAIT_START;
            return -1;  // 오버사이즈 방어
        }
        // LEN=0이면 DATA 단계 건너뛰기
        p->state = (byte == 0) ? STATE_GET_CHECKSUM : STATE_GET_DATA;
        return 0;

    case STATE_GET_END:
        p->state = STATE_WAIT_START;
        if (byte == PACKET_END) {
            if (calc_checksum(&p->pkt) == p->pkt.checksum) {
                p->ready = 1;
                return 1;  // 유효한 패킷 완성
            }
            return -1;  // 체크섬 불일치
        }
        return -1;  // END 마커 불일치
    // ...
    }
}
```

**설계 포인트:**
- `LEN > MAX_DATA_LEN` 시 즉시 `WAIT_START`로 복귀 → 버퍼 오버플로우 방지
- `LEN == 0`이면 `STATE_GET_DATA`를 건너뛰고 바로 `STATE_GET_CHECKSUM`으로 전이
- 체크섬과 END 마커 이중 검증 → 손상된 패킷 필터링

### 3. UART 인터럽트 수신 & 패킷 처리

HAL의 UART 인터럽트 수신을 사용하여 1바이트씩 비동기로 받는다. 폴링 방식과 달리 CPU가 수신 대기 중 다른 작업을 할 수 있다.

```c
static PacketParser parser;
static uint8_t rx_byte;

// 초기화 (main에서 while 루프 전)
parser_init(&parser);
HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

// UART 수신 완료 콜백 — 인터럽트 컨텍스트에서 호출
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        int result = parser_feed(&parser, rx_byte);
        if (result == 1) {
            process_packet(&parser.pkt);
        }
        // 다음 1바이트 수신 대기 재등록
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}
```

**수신 흐름:**
1. `HAL_UART_Receive_IT()` → RXNE 인터럽트 활성화
2. 바이트 도착 → HAL ISR → `HAL_UART_RxCpltCallback()` 호출
3. `parser_feed()`로 바이트 전달 → 패킷 완성 시 `process_packet()` 실행
4. 다시 `HAL_UART_Receive_IT()` 재등록 → 다음 바이트 대기

### 4. LED 제어 & ACK 응답

```c
static void process_packet(const Packet *pkt) {
    switch (pkt->cmd) {
    case CMD_LED_ON:
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        break;
    case CMD_LED_OFF:
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        break;
    case CMD_LED_TOGGLE:
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        break;
    default:
        return;  // 알 수 없는 명령 → ACK 안 보냄
    }
    send_ack(&huart2, pkt->cmd);
}

static void send_ack(UART_HandleTypeDef *huart, uint8_t original_cmd) {
    uint8_t ack[] = {
        PACKET_START,
        CMD_ACK,          // 0xAA
        0x01,             // LEN = 1
        original_cmd,     // DATA = 원래 명령어
        CMD_ACK ^ 0x01 ^ original_cmd,  // CHECKSUM
        PACKET_END
    };
    HAL_UART_Transmit(huart, ack, sizeof(ack), 100);
}
```

**ACK 설계 의도:** 응답 패킷의 DATA에 원래 명령어를 포함시켜, 호스트가 어떤 명령에 대한 응답인지 식별할 수 있다.

## 💡 배운 점

### 1. 바이너리 프로토콜 설계 원칙

임베디드 시리얼 통신에서 텍스트 기반("LED ON\n") 대신 바이너리 프로토콜을 쓰는 이유는 파싱이 단순하고, 데이터 크기가 작으며, 에러 검출이 용이하기 때문이다. `START/END` 마커로 패킷 경계를 구분하고, 체크섬으로 무결성을 검증하는 것이 기본 패턴이다.

실제로 `0x02`(STX)와 `0x03`(ETX)를 사용했는데, 이는 ASCII 제어 문자 표준에서 온 것으로 산업용 프로토콜에서도 흔히 쓰인다.

### 2. 상태 머신 패턴

UART는 바이트 스트림이라 패킷 경계가 없다. 상태 머신으로 바이트 단위 파싱을 하면 불완전한 수신, 노이즈, 재동기화를 자연스럽게 처리할 수 있다. 에러 발생 시 `WAIT_START`로 돌아가면 다음 유효한 패킷의 START 마커부터 다시 파싱을 시작한다.

이 패턴은 Modbus RTU, HDLC 같은 실제 산업 프로토콜에서도 동일하게 사용된다.

### 3. 인터럽트 수신 vs 폴링

`HAL_UART_Receive_IT()`는 RXNE 인터럽트를 활성화하고, 바이트가 도착하면 ISR에서 콜백을 호출한다. 폴링(`HAL_UART_Receive()`)은 바이트가 올 때까지 CPU가 블로킹되지만, 인터럽트 방식은 수신 대기 중 `main()` 루프에서 다른 작업이 가능하다.

콜백 끝에서 `HAL_UART_Receive_IT()`를 재호출해야 다음 바이트를 받을 수 있다는 점이 중요하다 — 이걸 빠뜨리면 첫 바이트만 받고 멈춘다.

### 4. ST-Link Virtual COM Port

NUCLEO 보드는 ST-Link 디버거가 내장되어 있고, USB 연결 시 **VCP(Virtual COM Port)**가 자동으로 생성된다. 별도의 USB-Serial 어댑터 없이 `USART2(PA2/PA3)`로 PC와 시리얼 통신이 가능하다. Mac에서는 `/dev/cu.usbmodemXXXX`로 잡힌다.

단, `screen`이나 다른 프로그램이 포트를 점유하고 있으면 `Resource busy` 에러가 발생한다 — 반드시 이전 세션을 종료(`killall screen`)한 후 연결해야 한다.

## 📊 성능

### 패킷 처리 속도

- UART 속도: 115200 baud = 약 11,520 bytes/sec
- 최소 패킷 크기: 5 bytes (START + CMD + LEN + CHECKSUM + END)
- 이론상 최대 처리량: ~2,304 packets/sec
- 실측: 1초 간격 명령 4회 → 모두 정상 ACK 수신 (100% 성공률)

### 메모리 사용

- `PacketParser` 구조체: 약 16 bytes (상태 1 + Packet 11 + data_idx 1 + ready 1 + 패딩)
- `rx_byte` 수신 버퍼: 1 byte
- ACK 송신 버퍼: 6 bytes (스택)
- 동적 할당 없음 → 메모리 누수 불가

## 🔮 개선 가능 사항

- [ ] 라즈베리파이 커널 드라이버 연동 (ioctl → UART 패킷 송신)
- [ ] 타임아웃 처리 — 패킷 수신 도중 타임아웃 시 상태 초기화
- [ ] NACK 응답 — 체크섬 에러 시 에러 패킷 반환
- [ ] 다중 명령어 확장 — LED 밝기(PWM), 상태 조회(CMD_STATUS)
- [ ] 링 버퍼 도입 — 고속 수신 시 데이터 유실 방지
- [ ] DMA 수신 — 인터럽트 오버헤드 감소

## 📚 참고 자료

- [STM32F446RE Reference Manual (RM0390)](https://www.st.com/resource/en/reference_manual/rm0390.pdf) — USART 레지스터
- [NUCLEO-F446RE User Manual (UM1724)](https://www.st.com/resource/en/user_manual/um1724.pdf) — VCP 핀 매핑
- [HAL UART API](https://www.st.com/resource/en/user_manual/um1725.pdf) — `HAL_UART_Receive_IT()`, 콜백
- STX/ETX 프레이밍 — ASCII 제어 문자 기반 패킷 구분 표준

## 📝 프로젝트 정보

| 항목 | 내용 |
|------|------|
| 개발 기간 | Day 2 |
| 개발 환경 | STM32CubeIDE (Mac), Python 3 |
| 타겟 보드 | STM32 NUCLEO-F446RE |
| 통신 | USART2 (PA2/PA3) → ST-Link VCP |
| 테스트 | Mac → USB → Python pyserial |
| 언어 | C (펌웨어), Python (테스트) |
| 작성자 | OnePaperHoon |

### 파일 구조

```
day2-protocol-led/
├── stm32/                  ← STM32CubeIDE 프로젝트
│   └── Core/
│       ├── Inc/
│       │   ├── protocol.h       # 프로토콜 정의 (패킷 구조, 명령어, 체크섬)
│       │   └── packet_parser.h  # 상태 머신 패킷 파서
│       └── Src/
│           └── main.c           # 인터럽트 수신, 패킷 처리, LED 제어
├── test/
│   └── send_cmd.py             # Python 테스트 스크립트
└── README.md
```