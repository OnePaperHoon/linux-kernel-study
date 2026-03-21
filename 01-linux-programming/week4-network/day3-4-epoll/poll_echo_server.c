// =============================================================================
// poll_echo_server.c — poll() 기반 에코 서버
//
// 목적: select()의 fd 개수 제한(FD_SETSIZE=1024)을 없앤 poll()로
//       동일한 에코 서버를 구현한다.
//
// select()와의 차이:
//   - fd_set 비트맵 대신 struct pollfd 배열을 사용한다.
//     → fd 개수 제한 없음 (배열 크기만큼 확장 가능).
//     → 이벤트 종류(events/revents)가 명확히 분리되어 있어 직관적이다.
//   - 여전히 매 호출마다 배열 전체를 커널에 복사 (O(n) 오버헤드는 동일).
//   - 준비된 fd를 찾으려면 배열 전체를 순회 (O(n) 탐색도 동일).
//
// struct pollfd:
//   int   fd      : 감시할 fd (-1이면 poll()이 자동으로 무시)
//   short events  : 관심 이벤트 (POLLIN = 읽기 가능)
//   short revents : poll() 반환 후 커널이 채워주는 실제 발생 이벤트
//
// 실행 흐름:
//   main → 리슨 소켓 생성 + bind + listen
//        → struct pollfd fds[] 배열 초기화
//        → 이벤트 루프:
//            poll(fds, nfds, -1) 호출 (-1 = 무한 대기)
//            fds[0] (listen_fd) POLLIN → accept() → 배열에 새 fd 추가
//            fds[i] POLLIN → recv() → 에코 또는 종료 처리
// =============================================================================

#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define PORT        8002
#define BUF_SIZE    1024
#define MAX_CLIENTS 1024

// -----------------------------------------------------------------------------
// add_client: 빈 슬롯(fd == -1)을 찾아 새 클라이언트 fd 등록
// 반환: 추가 성공 시 1, 슬롯 없으면 0
// -----------------------------------------------------------------------------
static int add_client(struct pollfd *fds, int *nfds, int new_fd)
{
    // 기존 배열에서 fd == -1인 빈 슬롯 탐색
    for (int i = 1; i < *nfds; i++)
    {
        if (fds[i].fd == -1)
        {
            fds[i].fd     = new_fd;
            fds[i].events = POLLIN;
            return 1;
        }
    }

    // 빈 슬롯 없음 → 배열 끝에 추가
    if (*nfds >= MAX_CLIENTS)
    {
        fprintf(stderr, "max clients reached\n");
        return 0;
    }
    fds[*nfds].fd     = new_fd;
    fds[*nfds].events = POLLIN;
    (*nfds)++;
    return 1;
}

// -----------------------------------------------------------------------------
// handle_new_client: accept() 후 pollfd 배열에 등록
// -----------------------------------------------------------------------------
static void handle_new_client(struct pollfd *fds, int *nfds)
{
    struct sockaddr_in  client_addr;
    socklen_t           addr_len = sizeof(client_addr);
    char                ip_buf[INET_ADDRSTRLEN];

    int client_fd = accept(fds[0].fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        perror("accept");
        return;
    }

    if (!add_client(fds, nfds, client_fd))
    {
        // 슬롯이 없으면 연결 거절
        close(client_fd);
        return;
    }

    inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
    printf("[poll] new connection: fd=%d, ip=%s, port=%d\n",
        client_fd, ip_buf, ntohs(client_addr.sin_port));
}

// -----------------------------------------------------------------------------
// handle_client: 기존 클라이언트 데이터 처리
// -----------------------------------------------------------------------------
static void handle_client(struct pollfd *pfd)
{
    char    buf[BUF_SIZE];
    ssize_t n = recv(pfd->fd, buf, sizeof(buf), 0);

    if (n <= 0)
    {
        if (n == 0)
            printf("[poll] client fd=%d disconnected\n", pfd->fd);
        else
            perror("recv");

        close(pfd->fd);
        pfd->fd = -1; // -1로 표시해두면 poll()이 자동으로 무시함
        return;
    }

    if (send(pfd->fd, buf, n, 0) < 0)
        perror("send");
}

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

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

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

    printf("[poll] echo server listening on port %d\n", PORT);

    // -------------------------------------------------------------------------
    // [2] pollfd 배열 초기화
    //
    // fds[0]: 항상 listen_fd (새 연결 수락 전용)
    // fds[1~]: 클라이언트 fd들
    // nfds: 현재 배열에서 유효한 항목 수 (poll()에 전달할 크기)
    // -------------------------------------------------------------------------
    struct pollfd fds[MAX_CLIENTS];
    int           nfds = 1;

    memset(fds, 0, sizeof(fds));
    for (int i = 0; i < MAX_CLIENTS; i++)
        fds[i].fd = -1; // -1: 비어있는 슬롯

    fds[0].fd     = listen_fd;
    fds[0].events = POLLIN;

    // -------------------------------------------------------------------------
    // [3] 이벤트 루프
    // -------------------------------------------------------------------------
    while (1)
    {
        // poll()은 select()와 달리 배열을 복사하지 않아도 된다
        // (revents만 수정하고 events는 건드리지 않음)
        int nready = poll(fds, nfds, -1);
        if (nready < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        // fds[0]: 새 연결 확인
        if (fds[0].revents & POLLIN)
        {
            handle_new_client(fds, &nfds);
            nready--;
        }

        // fds[1~]: 기존 클라이언트 처리
        for (int i = 1; i < nfds && nready > 0; i++)
        {
            if (fds[i].fd == -1)
                continue;

            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR))
            {
                handle_client(&fds[i]);
                nready--;
            }
        }
    }

    close(listen_fd);
    return 0;
}
