# Day 6-7: UDP 소켓 프로그래밍

## 학습 목표
- UDP 소켓의 비연결형(Connectionless) 특성 이해
- `recvfrom()` / `sendto()` API 사용
- TCP와 UDP의 차이점 코드로 확인
- UDP 브로드캐스트 구현

## 파일 구성

| 파일 | 설명 |
|------|------|
| `udp_server.c` | UDP echo 서버 — `recvfrom`으로 수신, `sendto`로 응답 |
| `udp_client.c` | UDP 클라이언트 — `connect()`로 대상 고정 후 RTT 측정 |
| `udp_broadcast.c` | 브로드캐스트 송신자/수신자 — `SO_BROADCAST` 옵션 |

## 빌드 및 실행

```bash
make

# UDP echo 테스트
# 터미널 1:
./udp_server

# 터미널 2:
./udp_client

# 브로드캐스트 테스트
# 터미널 1:
./udp_broadcast receiver

# 터미널 2:
./udp_broadcast sender
```

## TCP vs UDP 핵심 차이

| | TCP | UDP |
|---|---|---|
| 연결 수립 | 3-way handshake 필요 | 없음 |
| 신뢰성 | 순서 보장, 재전송 | 없음 (손실 가능) |
| 속도 | 상대적으로 느림 | 빠름 |
| 데이터 경계 | 없음 (스트림) | 있음 (데이터그램) |
| API | `recv()`/`send()` | `recvfrom()`/`sendto()` |
| 사용 예 | HTTP, FTP, SSH | DNS, DHCP, 게임, 스트리밍 |

## 핵심 API

```c
// UDP 소켓 생성 (SOCK_DGRAM)
int sock = socket(AF_INET, SOCK_DGRAM, 0);

// 수신 (송신자 주소도 함께 받음)
ssize_t n = recvfrom(sock, buf, len, 0,
                     (struct sockaddr *)&src_addr, &addrlen);

// 전송 (목적지 주소를 매번 명시)
ssize_t sent = sendto(sock, buf, n, 0,
                      (struct sockaddr *)&dst_addr, sizeof(dst_addr));

// 브로드캐스트 허용
int opt = 1;
setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

// 수신 타임아웃 (패킷 손실 대비)
struct timeval tv = { .tv_sec = 3, .tv_usec = 0 };
setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
```

## 주요 개념

### UDP connect()
UDP 소켓에 `connect()`를 사용하면 실제 연결은 수립되지 않고,
커널에 "기본 목적지 주소"만 등록된다.
이후 `send()`/`recv()` 사용이 가능해지고,
등록된 주소 이외에서 온 패킷은 자동으로 필터링된다.

### 브로드캐스트 주소
- `255.255.255.255`: 제한 브로드캐스트 (로컬 서브넷, 라우터를 넘지 않음)
- `192.168.1.255`: 특정 서브넷 브로드캐스트
- 라우터가 브로드캐스트를 차단하므로 같은 L2 세그먼트 내에서만 동작
