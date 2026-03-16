// 쉬운 TCP 서버 만들기


// TCP 서버는 클라이언트와 연결을 수립한 후, 데이터를 주고 받는 프로그램입니다.

// 예상하고 있는 TCP 서버의 동작은 다음과 같습니다:
/*
	1. TCP 서버는 특정 포트에서 클라이언트의 연결 요청을 기다립니다.
	2. 클라이언트가 연결 요청을 보내면, TCP 서버는 해당 요청을 수락하고, 클라이언트와의 연결을 수립합니다.
	3. TCP 서버는 클라이언트로 부터 데이터를 수신하고, 받은 데이터를 토대로 응답을 생성하여 클라이언트에게 전송합니다.
	4. 클라이언트의 연결 해제 또는 서버의 종료 시까지 TCP 서버는 클라리언트와의 연결을 유집합니다.
	5. TCP 서버는 여러 클라이언트 연결을 동시에 처리할 수 있도록 설계될 수 있습니다.
*/

// TCP 서버를 구현하기 위해서는 다음과 같은 주요 단계가 필요합니다:
/*
	1. 소켓 생성 TCP 서버는 소켓을 생성하여 네트워크 통신을 위한 엔드포인트를 만듭니다. 이 소켓은 TCP 프로토콜을 사용하도록 설정되어야 합니다.
	2. 소켓 바인딩 TCP 서버는 생성된 소켓을 특정 포트에 바인딩하여 클라이언트가 해당 포트로 연결될 수 있도록 합니다.
	3. 연결 수신 TCP 서버는 클라이언트의 연결 요청을 수신하기 위해 소켓을 리슨 상태로 설정합니다.
	이 단계에서는 서버가 동시에 여러 클라이언트 연결을 처리할 수 있도록 설정할 수 있습니다.
	4. 연결 수락 TCP 서버는 클라이언트의 연결 요청을 수락하여 새로운 소켓을 생성합니다. 이 새로운 소켓은 클라이언트와의 통신에 사용됩니다.
	5. 데이터 송수신 TCP 서버는 클라이언트와의 연결이 수립된 후, 데이터를 송수신할 수 있습니다. 서버는 클라이언트로부터 데이터를 수신하고, 받은 데이터를 처리하여 응답을 생성한 후 클라이언트에게 전송합니다.
	6. 연결 종료 TCP 서버는 클라이언트의 연결이 해제되거나 서버가 종료될 때 연결을 종료합니다. 이 단계에서는 리소스를 정리하고, 필요한 경우 로그를 기록할 수 있습니다.
*/

// 필요 헤더 및 상수, 함수 선언
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int create_server_socket()
{
	// TCP 소켓 생성
	// AF_INET: IPv4 인터넷 프로토콜, SOCK_STREAM: TCP 소켓, 0: 프로토콜 자동 선택
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	return server_socket;
}

void handle_client(int client_socket)
{
	char buffer[BUFFER_SIZE];
	ssize_t bytes_received;

	// 클라이언트로 부터 데이터 수신
	bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

	if (bytes_received < 0) {
		perror("recv");
		close(client_socket);
		return ;
	} else if (bytes_received == 0) {
		printf("Client disconnected\n");
		close(client_socket);
		return ;
	}

	// 수신된 데이터 처리 (ex: echo)
	buffer[bytes_received] = '\0'; // 문자열 종료 문자 추가
	printf("Received from client: %s\n", buffer);

	// 클라이언트에게 응답 전송 (echo)
	if (send(client_socket, buffer, bytes_received, 0) < 0)
	{
		perror("send");
	}

	// 클라이언트 소켓 닫기
	close(client_socket);
}

void run_server()
{
	int server_socket = create_server_socket();

	// 서버 주소 구조체 설정
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr)); // 구조체 초기화
	server_addr.sin_family = AF_INET; // IPv4
	server_addr.sin_addr.s_addr = INADDR_ANY; // 모든 인터페이스 사용
	server_addr.sin_port = htons(PORT); // 포트 번호 설정 (네트워크 바이트 순서)

	// 소켓 바인딩
	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	// 연결 수신 대기
	if (listen(server_socket, 5) < 0)
	{
		perror("listen");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	printf("TCP Echo Server is running on port %d...\n", PORT);

	while(1)
	{
		// 클라이언트 연결 수락
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

		if (client_socket < 0)
		{
			perror("accept");
			continue; // 연결 수락 실패 시 다음 연결을 위해 대기
		}

		printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		// 멀티 클라이언트 지원을 위해 fork 사용
		pid_t pid = fork();
		if (pid == 0) {
			// 자식 프로세스: 클라이언트 처리
			close(server_socket);
			handle_client(client_socket);
			exit(0);
		} else if (pid > 0) {
			// 부모 프로세스: 다음 연결 대기
			close(client_socket);
		} else {
			perror("fork");
			close(client_socket);
		}
	}
}

int main()
{
	run_server();

	return 0;
}

