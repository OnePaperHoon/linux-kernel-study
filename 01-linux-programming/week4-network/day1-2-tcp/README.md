# TCP Echo Server / Client

소켓 프로그래밍의 기본 — `socket → bind → listen → accept` (서버) / `socket → connect` (클라이언트) 흐름을 구현하고, 스레드 기반 동시 접속 테스트로 서버의 다중 클라이언트 처리를 검증한다.

## 🖥️ 실행 화면

### tcp_echo_server + tcp_echo_client

```bash
# Terminal 1
$ ./tcp_echo_server
TCP Echo Server is running on port 8080...
Client connected: 127.0.0.1:54321
Received from client: hello world
Client connected: 127.0.0.1:54322
Received from client: tcp test

# Terminal 2
$ ./tcp_echo_client
Connected to 127.0.0.1:8080
Enter message (q to quit): hello world
Echo: hello world  (RTT: 0.312 ms)
Enter message (q to quit): tcp test
Echo: tcp test  (RTT: 0.287 ms)
Enter message (q to quit): q
Connection closed.
```

### tcp_multi_client (동시 접속 테스트)

```bash
# Terminal 2 (서버가 실행 중인 상태에서)
$ ./tcp_multi_client
=== TCP Multi Client Test: 10 clients → 127.0.0.1:8080 ===

[Client  0] sent: Client 0: hello       | echo: Client 0: hello       | RTT: 0.421 ms
[Client  3] sent: Client 3: hello       | echo: Client 3: hello       | RTT: 0.398 ms
[Client  7] sent: Client 7: hello       | echo: Client 7: hello       | RTT: 0.445 ms
[Client  1] sent: Client 1: hello       | echo: Client 1: hello       | RTT: 0.512 ms
[Client  5] sent: Client 5: hello       | echo: Client 5: hello       | RTT: 0.389 ms
[Client  2] sent: Client 2: hello       | echo: Client 2: hello       | RTT: 0.467 ms
[Client  9] sent: Client 9: hello       | echo: Client 9: hello       | RTT: 0.501 ms
[Client  4] sent: Client 4: hello       | echo: Client 4: hello       | RTT: 0.478 ms
[Client  6] sent: Client 6: hello       | echo: Client 6: hello       | RTT: 0.423 ms
[Client  8] sent: Client 8: hello       | echo: Client 8: hello       | RTT: 0.411 ms

=== Test Result ===
Total clients : 10
Success       : 10
Fail          : 0
Avg RTT       : 0.444 ms
Total elapsed : 1.823 ms
```

## 🚀 빌드 및 실행

```bash
# 전체 빌드 (server, client, multi_client)
make

# 터미널 1: 서버 실행
./tcp_echo_server

# 터미널 2: 대화형 클라이언트
./tcp_echo_client

# 터미널 2: 동시 접속 테스트
./tcp_multi_client

# 메모리 검사
make valgrind

# 디버그 빌드
make debug
```

## 📖 프로젝트 개요

### 목적
TCP 소켓 API의 전체 흐름을 서버/클라이언트 양쪽에서 직접 구현하고,
`fork` 기반 멀티클라이언트 처리와 `pthread` 기반 동시 접속 테스트를 통해
네트워크 프로그래밍의 기초를 체득한다.

### 학습 목표
- [x] `socket()` / `bind()` / `listen()` / `accept()` / `connect()` API 이해
- [x] `send()` / `recv()` 반환값 처리 (partial send/recv)
- [x] `htons()` / `inet_pton()` 바이트 순서 및 주소 변환
- [x] `fork()`로 다중 클라이언트 동시 처리 (서버)
- [x] `pthread`로 동시 접속 부하 테스트 (클라이언트)
- [x] `pthread_mutex`로 공유 자원 경쟁 조건 방지
- [x] `gettimeofday()`로 RTT(왕복 지연 시간) 측정

### TCP 연결 수립 흐름 (3-way handshake)

```
Client                    Server
  |                          |
  |------- SYN ----------->  |  connect() 호출
  |                          |  (listen() 상태에서 대기 중)
  |<------ SYN-ACK --------  |
  |                          |
  |------- ACK ----------->  |  accept() 반환
  |                          |
  |====== 데이터 송수신 ======|  send() / recv()
  |                          |
  |------- FIN ----------->  |  close() 호출
  |<------ FIN-ACK --------  |
```

## 🔧 구현 내용

### 1. 서버 소켓 생성 및 바인딩

```c
/* [1] 소켓 생성: 네트워크 통신을 위한 파일 디스크립터 */
int server_socket = socket(AF_INET, SOCK_STREAM, 0);
//                         ↑        ↑             ↑
//                       IPv4     TCP (연결 지향)  프로토콜 자동

/* [2] 주소 구조체 설정 */
struct sockaddr_in addr;
memset(&addr, 0, sizeof(addr));     // 패딩 바이트까지 0으로 초기화
addr.sin_family      = AF_INET;
addr.sin_addr.s_addr = INADDR_ANY;  // 모든 NIC(네트워크 인터페이스)에서 수신
addr.sin_port        = htons(8080); // 호스트→네트워크 바이트 순서 변환

/* [3] 소켓을 포트에 묶기 */
bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));

/* [4] 연결 요청 대기열 크기 설정 */
listen(server_socket, 5);
// backlog=5: 아직 accept하지 않은 연결을 최대 5개까지 큐에 보관
```

### 2. fork 기반 멀티클라이언트 처리

```c
while (1) {
    int client_sock = accept(server_socket, ...); // 연결 수락

    pid_t pid = fork();
    if (pid == 0) {
        // 자식 프로세스: client_sock 만 필요, server_socket은 닫는다
        close(server_socket);   // ← 반드시 닫아야 함
        handle_client(client_sock);
        exit(0);
    } else if (pid > 0) {
        // 부모 프로세스: client_sock 필요 없음, 다음 accept로 복귀
        close(client_sock);     // ← 반드시 닫아야 함
    }
}

/*
 * 주의: fork() 후 부모/자식이 각자 불필요한 소켓을 닫지 않으면
 *       소켓의 참조 카운트가 0이 되지 않아 연결이 끊기지 않는다.
 *       → 두 프로세스가 모두 close()해야 실제로 FIN이 전송된다.
 */
```

### 3. 클라이언트 연결

```c
/* inet_pton: 문자열 IP → 네트워크 바이너리 변환 */
inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
// inet_addr()는 구형 API, 오류 반환값이 INADDR_NONE(=255.255.255.255)으로
// 브로드캐스트 주소와 구분 불가 → inet_pton() 사용 권장

/* connect: 3-way handshake 시작, 완료되면 반환 */
connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
```

### 4. send / recv 반환값

```c
/*
 * TCP는 스트림 기반 → 메시지 경계가 없다.
 * send()/recv() 는 요청 크기보다 적게 처리될 수 있다 (partial send/recv).
 *
 * 반환값:
 *   > 0  : 실제 처리된 바이트 수
 *   == 0 : 상대방이 연결 종료 (FIN 수신)
 *   < 0  : 에러 (errno 확인)
 */
ssize_t received = recv(sock, buf, sizeof(buf) - 1, 0);
if      (received  > 0) { buf[received] = '\0'; /* 처리 */ }
else if (received == 0) { /* 상대방 close() */ }
else                    { perror("recv"); }
```

### 5. pthread 기반 동시 접속 테스트

```c
/* 스레드에 정수 ID를 전달하는 관용 패턴 */
pthread_create(&threads[i], NULL, client_thread, (void *)(intptr_t)i);
//                                                ↑
//                          int → intptr_t → void * : 포인터 크기 안전 캐스팅
//                          힙 할당 없이 간단한 정수를 전달할 때 사용

/* pthread_join: main이 먼저 종료되면 스레드가 강제 종료됨 → 반드시 join */
pthread_join(threads[i], NULL);
```

### 6. mutex로 경쟁 조건 방지

```c
/*
 * success_count++ 는 내부적으로 3단계:
 *   읽기 → 더하기 → 쓰기
 * 두 스레드가 동시에 읽으면 하나의 증가만 반영되는 경쟁 조건 발생.
 * → mutex로 한 번에 하나의 스레드만 접근하도록 보호
 */
pthread_mutex_lock(&g_mutex);
g_result.success_count++;
g_result.total_rtt += rtt;
pthread_mutex_unlock(&g_mutex);
```

### 7. RTT 측정

```c
/*
 * gettimeofday(): 초(tv_sec) + 마이크로초(tv_usec) 분리 반환
 * → double로 합치면 마이크로초 정밀도로 시간 차이 계산 가능
 *
 * RTT(Round Trip Time) = send() 직전 ~ recv() 직후 경과 시간
 * → 로컬 루프백(127.0.0.1)이면 0.1~1ms, 원격 서버면 수십~수백 ms
 */
double start = get_time();
send(sock, buf, len, 0);
recv(sock, buf, sizeof(buf), 0);
double rtt_ms = (get_time() - start) * 1000.0;
```

## 💡 학습 포인트

### TCP vs UDP

```
TCP (SOCK_STREAM):
  - 연결 지향: 통신 전 3-way handshake 필요
  - 신뢰성 보장: 순서 보장, 재전송, 흐름 제어
  - 스트림 기반: 메시지 경계 없음 → partial send/recv 주의
  - 사용: HTTP, SSH, FTP, 데이터베이스

UDP (SOCK_DGRAM):
  - 비연결: connect() 없이 바로 sendto()
  - 신뢰성 없음: 손실, 순서 바뀜 가능
  - 데이터그램 기반: 메시지 경계 있음
  - 사용: DNS, 게임, 영상 스트리밍
```

### 바이트 순서 변환

```
CPU(x86): 리틀 엔디안 → 0x1234 → 메모리: [0x34][0x12]
네트워크: 빅 엔디안   → 0x1234 → 전송:   [0x12][0x34]

htons(port)  : Host TO Network Short (16bit, 포트 번호)
htonl(addr)  : Host TO Network Long  (32bit, IP 주소)
ntohs(port)  : Network TO Host Short (수신 후 역변환)

→ sockaddr_in에 넣는 값은 반드시 htons/htonl 변환 후 대입
→ 출력할 때는 ntohs/ntohl로 역변환
```

### INADDR_ANY vs 특정 IP

```c
addr.sin_addr.s_addr = INADDR_ANY;    // 0.0.0.0: 모든 NIC에서 수신
addr.sin_addr.s_addr = inet_addr("192.168.0.1"); // 특정 NIC만 바인딩

/* 서버가 여러 NIC(이더넷 + WiFi)를 갖고 있을 때
 * INADDR_ANY: 어느 NIC으로 들어와도 받음 (개발 시 일반적)
 * 특정 IP: 해당 NIC으로 들어온 연결만 받음 (보안 강화 시)
 */
```

### fork 기반 vs 스레드 기반 서버

```
fork 기반 (tcp_echo_server):
  장점: 자식 프로세스가 독립적 → 한 클라이언트 crash가 서버에 영향 없음
  단점: 프로세스 생성 비용이 높음, 메모리 공유 어려움

스레드 기반:
  장점: 생성 비용 낮음, 메모리 공유 쉬움
  단점: 한 스레드 crash → 전체 프로세스 종료, mutex 관리 필요

epoll 기반 (day3-4):
  장점: 수천 개 연결을 단일 스레드로 처리 (이벤트 루프)
  단점: 코드 복잡도 증가
```

### partial send/recv 처리 (실무 패턴)

```c
/* 큰 데이터를 보낼 때 send()가 일부만 전송할 수 있다 */
ssize_t send_all(int sock, const char *buf, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return sent;
}

/* recv도 동일하게 원하는 크기를 다 받을 때까지 반복 */
ssize_t recv_all(int sock, char *buf, size_t len)
{
    size_t received = 0;
    while (received < len) {
        ssize_t n = recv(sock, buf + received, len - received, 0);
        if (n <= 0) return n; // 0: 연결 종료, -1: 에러
        received += n;
    }
    return received;
}
```

### SO_REUSEADDR (서버 재시작 시 필수)

```c
/*
 * 서버를 Ctrl+C로 종료 후 바로 재시작하면 bind()가 실패하는 경우가 있다.
 * 이유: TIME_WAIT 상태의 소켓이 포트를 점유 중 (기본 2분간 유지)
 *
 * SO_REUSEADDR 옵션으로 TIME_WAIT 상태의 포트도 즉시 재사용 가능
 */
int opt = 1;
setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
// bind() 전에 설정해야 한다
```

## 🔮 개선 가능 사항

- [ ] `SO_REUSEADDR` 옵션 추가 (서버 재시작 시 bind 실패 방지)
- [ ] `send_all()` / `recv_all()` 구현 (partial send/recv 완전 처리)
- [ ] 서버에 `SIGCHLD` 핸들러 추가 (좀비 프로세스 방지)
- [ ] 타임아웃 설정 (`SO_RCVTIMEO`, `SO_SNDTIMEO`)
- [ ] epoll 기반으로 전환하여 수천 클라이언트 동시 처리 → [day3-4-epoll](../day3-4-epoll)

## 📚 참고 자료

### Man Pages

```bash
man socket
man bind
man listen
man accept
man connect
man send
man recv
man inet_pton
man htons
man gettimeofday
```

### 관련 프로젝트

- [day3-4-epoll](../day3-4-epoll) - epoll 기반 이벤트 루프 서버
- [day5-http](../day5-http) - HTTP 서버 구현
- [day6-7-udp](../day6-7-udp) - UDP 소켓 프로그래밍
- [week2-threads/producer-consumer](../../week2-threads/producer-consumer) - pthread + mutex

## 📝 프로젝트 정보

```
개발 기간: Day 1-2
환경: Linux
언어: C
빌드: cc, make
```

**파일 구조:**

```
day1-2-tcp/
├── tcp_echo_server.c   # fork 기반 멀티클라이언트 echo 서버
├── tcp_echo_client.c   # 대화형 echo 클라이언트 + RTT 측정
├── tcp_multi_client.c  # pthread 기반 동시 접속 부하 테스트
├── Makefile            # all / clean / fclean / re / debug / sanitize
└── README.md
```

**핵심 흐름:**

```c
/* 서버 */
int srv = socket(AF_INET, SOCK_STREAM, 0);
bind(srv, &addr, sizeof(addr));
listen(srv, 5);
int cli = accept(srv, &client_addr, &len); // 블로킹: 연결 올 때까지 대기
fork(); // 자식이 cli 처리, 부모는 다음 accept로

/* 클라이언트 */
int sock = socket(AF_INET, SOCK_STREAM, 0);
connect(sock, &server_addr, sizeof(server_addr)); // 3-way handshake
send(sock, msg, strlen(msg), 0);
recv(sock, buf, sizeof(buf), 0);
close(sock); // FIN 전송
```

---

**Author:** OnePaperHoon
**Date:** March 2026
**Project:** Linux Kernel Study - Week 4, Day 1-2
**Topic:** TCP Socket (echo server + client + multi-client load test)
