// =============================================================================
// UDP Echo 서버
//
// 역할: UDP 소켓으로 클라이언트의 데이터그램을 수신하고, 그대로 돌려보낸다.
//
// TCP 서버와의 핵심 차이:
//   TCP: socket() → bind() → listen() → accept() → recv()/send() → close()
//   UDP: socket() → bind() → recvfrom()/sendto() (연결 수립 없음)
//
// UDP의 특성:
//   - 비연결형(Connectionless): 매 데이터그램마다 목적지 주소를 지정해야 한다.
//   - 신뢰성 없음: 패킷 순서 보장, 재전송, 흐름 제어 없다.
//   - 빠르다: 연결 수립 오버헤드(3-way handshake)가 없다.
//   - 데이터그램 경계 유지: TCP와 달리 송신 단위가 수신 단위와 일치한다.
// =============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT        9090
#define BUF_SIZE    1024

// -----------------------------------------------------------------------------
// create_udp_socket: UDP 소켓 생성 및 포트 바인딩
//
// SOCK_DGRAM: 데이터그램 소켓 (UDP)
//   - SOCK_STREAM(TCP)은 스트림 기반, 연결 필요
//   - SOCK_DGRAM(UDP)은 독립적인 데이터그램 단위로 송수신
//
// SO_REUSEADDR:
//   서버를 재시작할 때 "Address already in use" 에러를 방지한다.
//   소켓이 TIME_WAIT 상태에 있을 때도 같은 포트를 즉시 재사용할 수 있게 해준다.
// -----------------------------------------------------------------------------
int create_udp_socket()
{
	// SOCK_DGRAM: UDP 프로토콜 지정
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// SO_REUSEADDR: 포트 재사용 허용
	int opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// 서버 주소 구조체 설정
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;  // 모든 인터페이스에서 수신
	server_addr.sin_port        = htons(PORT);

	// bind: 소켓에 주소(IP + 포트)를 연결
	// UDP 서버는 listen()/accept()가 없다.
	// bind() 이후 바로 recvfrom()으로 수신 대기한다.
	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind");
		close(sock);
		exit(EXIT_FAILURE);
	}

	return sock;
}

// -----------------------------------------------------------------------------
// run_echo_loop: UDP 에코 루프
//
// recvfrom():
//   TCP의 recv()와 달리, 데이터와 함께 송신자의 주소(IP + 포트)를 받는다.
//   반환값: 수신한 바이트 수 (오류 시 -1)
//
//   int recvfrom(
//       int sockfd,              // 소켓 fd
//       void *buf,               // 수신 버퍼
//       size_t len,              // 버퍼 크기
//       int flags,               // 보통 0
//       struct sockaddr *src,    // 송신자 주소가 여기에 채워진다
//       socklen_t *addrlen       // 주소 구조체 크기 (in/out 파라미터)
//   );
//
// sendto():
//   TCP의 send()와 달리, 매번 목적지 주소를 명시해야 한다.
//   recvfrom()으로 받은 client_addr를 그대로 sendto()에 전달하면 에코된다.
//
// UDP 데이터그램 경계:
//   TCP는 recv() 한 번에 여러 send()의 데이터가 합쳐져 올 수 있지만,
//   UDP는 send() 한 번 = recvfrom() 한 번이 보장된다.
//   단, 데이터그램 크기가 BUF_SIZE를 초과하면 나머지는 버려진다(잘림).
// -----------------------------------------------------------------------------
void run_echo_loop(int sock)
{
	char    buf[BUF_SIZE];
	struct  sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	printf("UDP Echo Server listening on port %d...\n", PORT);

	while (1)
	{
		// 클라이언트로부터 데이터그램 수신
		// client_addr에 송신자의 IP:PORT가 자동으로 채워진다
		ssize_t n = recvfrom(sock, buf, BUF_SIZE - 1, 0,
				(struct sockaddr *)&client_addr, &client_len);
		if (n < 0)
		{
			perror("recvfrom");
			continue;
		}

		buf[n] = '\0';
		printf("[%s:%d] received: %s (%zd bytes)\n",
			inet_ntoa(client_addr.sin_addr),
			ntohs(client_addr.sin_port),
			buf, n);

		// 수신한 데이터를 그대로 클라이언트에게 돌려보냄 (echo)
		// sendto: 매 전송마다 목적지 주소를 명시 (비연결형의 핵심)
		ssize_t sent = sendto(sock, buf, n, 0,
				(struct sockaddr *)&client_addr, client_len);
		if (sent < 0)
			perror("sendto");
	}
}

int main()
{
	int sock = create_udp_socket();
	run_echo_loop(sock);
	close(sock);
	return 0;
}
