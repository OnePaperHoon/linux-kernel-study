// =============================================================================
// UDP 브로드캐스트 송신자 / 수신자
//
// 역할: 같은 네트워크의 모든 호스트에게 동시에 메시지를 보낸다.
//
// 브로드캐스트(Broadcast)란:
//   - 특정 IP가 아닌 서브넷의 모든 호스트에게 동시 전송
//   - 브로드캐스트 주소: 서브넷 주소의 호스트 부분을 모두 1로 설정
//     예) 192.168.1.0/24 → 브로드캐스트: 192.168.1.255
//   - 255.255.255.255: 제한 브로드캐스트 (로컬 서브넷 전체, 라우터를 넘지 않음)
//
// 멀티캐스트(Multicast)와의 차이:
//   - 브로드캐스트: 서브넷 내 모든 호스트에 전달 (관심 없는 호스트도 수신)
//   - 멀티캐스트: 특정 그룹에 가입한 호스트에만 전달 (더 효율적)
//
// 사용 예: 서비스 디스커버리, DHCP, ARP, 간단한 네트워크 알림
//
// 컴파일 후 사용법:
//   수신자: ./udp_broadcast receiver
//   송신자: ./udp_broadcast sender
//   (두 터미널에서 수신자를 먼저 실행한 뒤 송신자를 실행)
// =============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BROADCAST_IP    "255.255.255.255"  // 제한 브로드캐스트 주소
#define PORT            9091
#define BUF_SIZE        1024

// -----------------------------------------------------------------------------
// run_sender: 브로드캐스트 송신
//
// SO_BROADCAST:
//   기본적으로 UDP 소켓은 브로드캐스트 전송을 허용하지 않는다.
//   이 옵션을 설정해야 브로드캐스트 주소로 sendto()가 가능하다.
//   (실수로 브로드캐스트 트래픽을 대량 발생시키는 것을 방지하기 위함)
//
// 주의: 브로드캐스트는 라우터를 넘지 않는다.
//       같은 L2 세그먼트(스위치/서브넷) 내에서만 동작한다.
// -----------------------------------------------------------------------------
void run_sender()
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// SO_BROADCAST: 브로드캐스트 전송 허용 (기본값: 비허용)
	int opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt SO_BROADCAST");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// 브로드캐스트 목적지 주소 설정
	struct sockaddr_in bcast_addr;
	memset(&bcast_addr, 0, sizeof(bcast_addr));
	bcast_addr.sin_family      = AF_INET;
	bcast_addr.sin_port        = htons(PORT);
	bcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

	printf("UDP Broadcast Sender ready (-> %s:%d)\n", BROADCAST_IP, PORT);
	printf("Enter messages to broadcast (q to quit):\n");

	char buf[BUF_SIZE];
	int  seq = 0;

	while (1)
	{
		printf("[%d] > ", ++seq);
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		buf[strcspn(buf, "\n")] = '\0';
		if (strcmp(buf, "q") == 0)
			break;
		if (strlen(buf) == 0)
			continue;

		// 브로드캐스트 전송: 같은 서브넷의 모든 호스트가 수신
		ssize_t sent = sendto(sock, buf, strlen(buf), 0,
				(struct sockaddr *)&bcast_addr, sizeof(bcast_addr));
		if (sent < 0)
			perror("sendto");
		else
			printf("  Broadcasted %zd bytes\n", sent);
	}

	close(sock);
}

// -----------------------------------------------------------------------------
// run_receiver: 브로드캐스트 수신
//
// 수신 측은 일반 UDP 서버와 동일하게 bind() → recvfrom()이다.
// INADDR_ANY로 바인딩하면 브로드캐스트 패킷도 수신된다.
//
// SO_REUSEADDR:
//   같은 머신에서 수신자를 여러 개 실행할 때 필요.
//   브로드캐스트 테스트 시 동일 포트에 여러 프로세스가 바인딩될 수 있다.
// -----------------------------------------------------------------------------
void run_receiver()
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// SO_REUSEADDR: 포트 재사용 (동일 포트에 여러 수신자 허용)
	int opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt SO_REUSEADDR");
		close(sock);
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in bind_addr;
	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_family      = AF_INET;
	bind_addr.sin_addr.s_addr = INADDR_ANY;  // 모든 인터페이스 (브로드캐스트 포함)
	bind_addr.sin_port        = htons(PORT);

	if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0)
	{
		perror("bind");
		close(sock);
		exit(EXIT_FAILURE);
	}

	printf("UDP Broadcast Receiver listening on port %d...\n", PORT);

	char buf[BUF_SIZE];
	struct sockaddr_in sender_addr;
	socklen_t sender_len = sizeof(sender_addr);

	while (1)
	{
		ssize_t n = recvfrom(sock, buf, BUF_SIZE - 1, 0,
				(struct sockaddr *)&sender_addr, &sender_len);
		if (n < 0)
		{
			perror("recvfrom");
			continue;
		}
		buf[n] = '\0';
		printf("[broadcast from %s:%d] %s\n",
			inet_ntoa(sender_addr.sin_addr),
			ntohs(sender_addr.sin_port),
			buf);
	}

	close(sock);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s [sender|receiver]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "sender") == 0)
		run_sender();
	else if (strcmp(argv[1], "receiver") == 0)
		run_receiver();
	else
	{
		fprintf(stderr, "Unknown mode: %s (use 'sender' or 'receiver')\n", argv[1]);
		return EXIT_FAILURE;
	}

	return 0;
}
