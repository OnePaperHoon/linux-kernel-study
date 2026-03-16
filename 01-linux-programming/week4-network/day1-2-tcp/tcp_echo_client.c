// =============================================================================
// TCP Echo 클라이언트
//
// 역할: TCP 서버에 접속하여 메시지를 전송하고 에코 응답을 받는다.
//       get_time()으로 RTT(Round Trip Time, 왕복 지연 시간)를 측정한다.
//
// TCP 클라이언트의 기본 흐름:
//   socket() → connect() → send()/recv() 반복 → close()
//
// 서버와 달리 bind()/listen()/accept()가 필요 없다.
// 클라이언트는 서버에 "먼저 연결을 요청하는" 쪽이기 때문이다.
// =============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>    // gettimeofday()
#include <arpa/inet.h>   // inet_pton(), htons(), sockaddr_in

#define SERVER_IP   "127.0.0.1"  // 접속할 서버 IP (루프백: 같은 머신)
#define PORT        8080          // 서버와 동일한 포트를 사용해야 연결된다
#define BUF_SIZE    1024

// -----------------------------------------------------------------------------
// error_handling: 에러 메시지를 stderr에 출력하고 프로세스 종료
//
// perror()와 달리 커스텀 메시지를 출력한다.
// 소켓 프로그래밍에서는 에러 발생 시 즉시 종료하거나
// 리소스를 정리(close)하는 것이 중요하다.
// -----------------------------------------------------------------------------
void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

// -----------------------------------------------------------------------------
// get_time: 현재 시각을 초 단위 실수로 반환 (마이크로초 정밀도)
//
// gettimeofday()는 초(tv_sec)와 마이크로초(tv_usec)를 분리해서 반환한다.
// 두 값을 합쳐 double로 만들면 소수점 이하로 시간 차이를 계산할 수 있다.
// RTT 측정: send() 직전과 recv() 직후의 시간 차이 = 네트워크 왕복 시간
// -----------------------------------------------------------------------------
double get_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

// -----------------------------------------------------------------------------
// connect_to_server: 소켓 생성 및 서버 연결
//
// [1] socket(AF_INET, SOCK_STREAM, 0)
//   - AF_INET    : IPv4 주소 체계 사용
//   - SOCK_STREAM: TCP 프로토콜 (연결 지향, 신뢰성 보장)
//   - 0          : 프로토콜 자동 선택 (SOCK_STREAM이면 항상 TCP)
//
// [2] inet_pton()
//   - "127.0.0.1" 같은 문자열 IP를 네트워크 바이너리 형식으로 변환
//   - inet_addr()는 구형 API이며 오류 감지가 불완전해 inet_pton()을 권장
//
// [3] htons(PORT)
//   - Host TO Network Short: CPU의 바이트 순서(리틀엔디안) →
//     네트워크 표준(빅엔디안)으로 변환
//   - 서로 다른 아키텍처끼리 통신할 때도 포트 번호가 일치하도록 보장
//
// [4] connect()
//   - 서버에 3-way handshake(SYN → SYN-ACK → ACK)를 시작한다.
//   - 이 함수가 성공하면 연결이 수립된 것이다.
// -----------------------------------------------------------------------------
int connect_to_server()
{
	// [1] 소켓 생성 — 파일 디스크립터처럼 정수값으로 반환된다
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");

	// 서버 주소 구조체 초기화
	// memset으로 0으로 채우지 않으면 패딩 바이트에 쓰레기값이 남을 수 있다
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;                               // [1] IPv4
	server_addr.sin_port   = htons(PORT);                           // [3] 포트 변환
	if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)  // [2] IP 변환
		error_handling("inet_pton() error");

	// [4] 서버에 연결 요청 (3-way handshake)
	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		error_handling("connect() error");

	printf("Connected to %s:%d\n", SERVER_IP, PORT);
	return sock;
}

// -----------------------------------------------------------------------------
// run_echo_loop: 메시지 전송 → 에코 수신을 반복하는 대화형 루프
//
// [send 주의사항]
//   send()는 버퍼 공간이 부족하면 일부만 전송할 수 있다 (partial send).
//   이 예제에서는 짧은 메시지를 보내므로 생략했지만,
//   실제 프로덕션 코드에서는 sent < len이면 나머지를 재전송해야 한다.
//
// [recv 주의사항]
//   recv()도 마찬가지로 요청한 크기보다 적게 받을 수 있다 (partial recv).
//   TCP는 스트림 기반이므로 메시지 경계가 없다.
//   이 에코 서버는 받은 만큼 바로 돌려보내므로 한 번의 recv로 충분하지만,
//   큰 데이터를 주고받을 때는 반복 recv가 필요하다.
//
// [recv 반환값]
//    > 0 : 수신한 바이트 수
//   == 0 : 상대방이 연결을 정상 종료(FIN)
//    < 0 : 에러
// -----------------------------------------------------------------------------
void run_echo_loop(int sock)
{
	char send_buf[BUF_SIZE];
	char recv_buf[BUF_SIZE];

	while (1)
	{
		printf("Enter message (q to quit): ");

		// fgets는 개행 문자('\n')까지 포함해서 읽는다
		if (fgets(send_buf, sizeof(send_buf), stdin) == NULL)
			break;

		// strcspn으로 '\n' 위치를 찾아 '\0'으로 교체 → 개행 제거
		send_buf[strcspn(send_buf, "\n")] = '\0';

		if (strcmp(send_buf, "q") == 0)
			break;

		if (strlen(send_buf) == 0)
			continue;

		// RTT 측정 시작: 전송 직전 시각 기록
		double start = get_time();

		// 메시지 전송
		// strlen(send_buf): 문자열 길이만 전송 (널 문자 제외)
		ssize_t sent = send(sock, send_buf, strlen(send_buf), 0);
		if (sent == -1)
		{
			perror("send");
			break;
		}

		// 에코 응답 수신
		ssize_t received = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
		if (received == 0)
		{
			// 서버가 연결을 닫은 경우 (FIN 수신)
			printf("Server closed connection\n");
			break;
		}
		else if (received < 0)
		{
			perror("recv");
			break;
		}

		// RTT 측정 종료 및 출력
		double rtt = get_time() - start;

		recv_buf[received] = '\0'; // 수신 데이터 끝에 널 문자 추가
		printf("Echo: %s  (RTT: %.3f ms)\n", recv_buf, rtt * 1000.0);
	}
}

int main()
{
	// 서버에 연결하고 소켓 fd를 받는다
	int sock = connect_to_server();

	// 대화형 에코 루프 실행
	run_echo_loop(sock);

	// 소켓을 닫으면 서버 측에서 recv()가 0을 반환하며 연결 종료를 감지한다
	// (TCP FIN 패킷 전송)
	close(sock);
	printf("Connection closed.\n");
	return 0;
}
