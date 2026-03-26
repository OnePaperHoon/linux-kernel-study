// =============================================================================
// UDP Echo 클라이언트
//
// 역할: UDP 서버에 메시지를 전송하고 에코 응답을 받는다.
//       RTT(Round Trip Time)를 측정하여 네트워크 지연을 확인한다.
//
// TCP 클라이언트와의 핵심 차이:
//   TCP: socket() → connect() → send()/recv() → close()
//   UDP: socket() → sendto() → recvfrom() (connect() 없음)
//
// UDP 클라이언트에서 connect() 사용 여부:
//   - connect()를 쓰지 않으면: sendto()/recvfrom()으로 매번 주소 지정
//   - connect()를 쓰면: send()/recv() 사용 가능 (대상 고정, 코드 간결)
//     단, 실제 연결이 수립되는 게 아니라 커널에 "기본 대상" 을 등록하는 것이다.
//   이 예제는 connect()를 사용해 코드를 간결하게 작성한다.
//
// UDP의 패킷 손실 처리:
//   UDP는 신뢰성이 없으므로 패킷이 유실될 수 있다.
//   recvfrom()에 타임아웃(SO_RCVTIMEO)을 설정해 무한 대기를 방지한다.
// =============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>    // gettimeofday(), struct timeval

#define SERVER_IP   "127.0.0.1"
#define PORT        9090
#define BUF_SIZE    1024
#define TIMEOUT_SEC 3    // recvfrom 타임아웃 (초)

// -----------------------------------------------------------------------------
// get_time: 현재 시각을 초 단위 실수로 반환 (마이크로초 정밀도)
// -----------------------------------------------------------------------------
double get_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

// -----------------------------------------------------------------------------
// create_udp_socket: UDP 소켓 생성 및 서버 주소 설정
//
// connect()를 UDP 소켓에 사용하면:
//   1. send()/recv() 사용 가능 (sendto()/recvfrom() 불필요)
//   2. 다른 주소에서 온 데이터그램은 자동으로 필터링됨
//   3. ICMP "port unreachable" 에러가 recv()에 반영됨
//      (connect() 없으면 ICMP 에러가 무시됨)
//
// SO_RCVTIMEO: 수신 타임아웃 설정
//   UDP는 패킷이 유실되면 recvfrom()이 영원히 블록된다.
//   타임아웃을 설정하면 지정된 시간 후 EAGAIN/EWOULDBLOCK 에러가 반환된다.
// -----------------------------------------------------------------------------
int create_udp_socket()
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// SO_RCVTIMEO: 수신 타임아웃 설정 (패킷 손실 시 무한 대기 방지)
	struct timeval tv;
	tv.tv_sec  = TIMEOUT_SEC;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
	{
		perror("setsockopt SO_RCVTIMEO");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// 서버 주소 구조체 설정
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port   = htons(PORT);
	if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
	{
		perror("inet_pton");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// UDP connect(): 실제 연결이 아닌 "기본 대상 주소" 등록
	// 이후 send()/recv() 사용 가능
	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connect");
		close(sock);
		exit(EXIT_FAILURE);
	}

	printf("UDP client ready (target: %s:%d, timeout: %ds)\n",
		SERVER_IP, PORT, TIMEOUT_SEC);
	return sock;
}

// -----------------------------------------------------------------------------
// run_echo_loop: 메시지 전송 → 에코 수신 루프
//
// UDP의 신뢰성 문제와 대응:
//   - 패킷 손실: recvfrom()이 타임아웃되면 "Timeout" 출력 후 재시도 가능
//   - 순서 뒤바뀜: 이 예제는 1:1 요청-응답이므로 순서 문제가 발생하지 않음
//   - 중복 수신: 실제 환경에서는 시퀀스 번호로 중복 체크 필요
//
// 주의: UDP는 전송 보장이 없으므로 애플리케이션 레벨에서
//       재전송/확인응답(ACK) 등을 구현해야 한다. (TFTP, QUIC 등이 이 방식)
// -----------------------------------------------------------------------------
void run_echo_loop(int sock)
{
	char send_buf[BUF_SIZE];
	char recv_buf[BUF_SIZE];

	while (1)
	{
		printf("Enter message (q to quit): ");

		if (fgets(send_buf, sizeof(send_buf), stdin) == NULL)
			break;

		send_buf[strcspn(send_buf, "\n")] = '\0';

		if (strcmp(send_buf, "q") == 0)
			break;

		if (strlen(send_buf) == 0)
			continue;

		// RTT 측정 시작
		double start = get_time();

		// UDP 전송: connect()로 대상이 고정되어 있으므로 send() 사용
		ssize_t sent = send(sock, send_buf, strlen(send_buf), 0);
		if (sent < 0)
		{
			perror("send");
			break;
		}

		// 에코 응답 수신 (타임아웃 내에 응답이 없으면 에러 반환)
		ssize_t n = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
		if (n < 0)
		{
			// EAGAIN / EWOULDBLOCK: 타임아웃 (패킷 손실 가능성)
			perror("recv (timeout or error)");
			printf("  -> Packet may have been lost. Retry or check server.\n");
			continue;
		}

		double rtt = get_time() - start;

		recv_buf[n] = '\0';
		printf("Echo: %s  (RTT: %.3f ms)\n", recv_buf, rtt * 1000.0);
	}
}

int main()
{
	int sock = create_udp_socket();
	run_echo_loop(sock);
	close(sock);
	printf("Client closed.\n");
	return 0;
}
