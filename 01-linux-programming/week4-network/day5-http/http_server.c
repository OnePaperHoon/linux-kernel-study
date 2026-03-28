#include <arpa/inet.h>
#include <sys/epoll.h>


// =============================================================================
// http_server.c: 간단한 HTTP 서버 (epoll 버전)
// 목적: HTTP 프로토콜을 이해하고, epoll 기반 서버에서 www.html 파일을 제공한다.
// =============================================================================

// -----------------------------------------------------------------------------
#define PORT		8000
#define BUF_SIZE	1024




int main(void)
{
	// -------------------------------------------------------------------------
	// [1] 리슨 소켓 생성
	// -------------------------------------------------------------------------
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// -------------------------------------------------------------------------
	// [2] bind + listen
	// -------------------------------------------------------------------------
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);
	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	if (listen(listen_fd, 128) < 0)
	{
		perror("listen");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	printf("[http] server listening on port %d\n", PORT);

	int epfd = epoll_create1();
	struct epoll_event ev, events[10];
	ev.events = EPOLLIN;
	ev.data.fd = listen_fd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
	{
		perror("epoll_ctl");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		// -------------------------------------------------------------------------
		// epoll_wait으로 이벤트를 대기
		// 반환 값은 이벤트가 발생한 파일 디스크립터의 개수
		// -------------------------------------------------------------------------
		int nready = epoll_wait(epfd, events, 10, -1);
		if (nready < 0)
		{
			perror("epoll_wait");
			break;
		}

		// -------------------------------------------------------------------------
		// 이벤트 처리
		// 위에서 받은 nready는 events 배열에서 유효한 항목 수를 의미한다.
		// -------------------------------------------------------------------------
		for (int i = 0; i < nready; i++)
		{
			if (events[i].data.fd == listen_fd)
			{
				// 새 연결 수락
				int client_fd = accept(listen_fd, NULL, NULL);
				if (client_fd < 0)
				{
					perror("accept");
					continue;
				}

				// 새 클라이언트 fd를 epoll에 등록
				ev.events = EPOLLIN;
				ev.data.fd = client_fd;
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
				{
					perror("epoll_ctl: add client");
					close(client_fd);
					continue;
				}

				printf("[http] new client connected: fd=%d\n", client_fd);
			}
			else
			{
				// 클라이언트로부터 데이터 수신
				int client_fd = events[i].data.fd;
				char buf[BUF_SIZE];
				ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
				if (n < 0)
				{
					perror("recv");
					close(client_fd);
					continue;
				}
				else if (n == 0)
				{
					// 클라이언트가 연결을 종료한 경우
					printf("[http] client disconnected: fd=%d\n", client_fd);
					close(client_fd);
					continue;
				}

				buf[n] = '\0'; // 문자열로 처리하기 위해 null-terminate

				printf("[http] received request from fd=%d:\n%s\n", client_fd, buf);
		}
	}
}
