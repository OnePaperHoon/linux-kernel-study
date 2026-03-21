// =============================================================================
// select_echo_server.c — select() 기반 에코 서버
//
// 목적: I/O 멀티플렉싱의 가장 기초인 select()를 이용해
//       단일 스레드로 여러 클라이언트를 동시에 처리한다.
//
// select()의 동작 원리:
//   - 관심 있는 fd들을 fd_set(비트맵)에 등록한다.
//   - select()를 호출하면 커널이 해당 fd 중 "읽기 가능한" fd가 생길 때까지 블로킹한다.
//   - 반환 후, fd_set을 순회하며 어떤 fd가 준비됐는지 FD_ISSET으로 확인한다.
//
// 단점 (poll/epoll과 비교 시):
//   - fd_set 크기가 FD_SETSIZE(보통 1024)로 제한된다.
//   - 매 호출마다 fd_set 전체를 커널에 복사해야 한다 (O(n) 오버헤드).
//   - 준비된 fd를 찾으려면 전체 fd를 순회해야 한다 (O(n) 탐색).
//
// 실행 흐름:
//   main → 리슨 소켓 생성 + bind + listen
//        → fd_set 초기화 (FD_ZERO, FD_SET)
//        → 이벤트 루프:
//            select() 호출 (타임아웃 NULL = 무한 대기)
//            리슨 소켓 준비됨 → accept() → 새 클라이언트 fd를 fd_set에 추가
//            클라이언트 fd 준비됨 → recv() → 데이터 있으면 send() 에코
//                                            → 데이터 0이면 연결 종료 처리
// =============================================================================

#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define PORT        8001
#define BUF_SIZE    1024

// -----------------------------------------------------------------------------
// handle_new_client: accept() 후 fd_set에 등록
// -----------------------------------------------------------------------------
static void handle_new_client(int listen_fd, fd_set *master_set, int *max_fd)
{
    struct sockaddr_in  client_addr;
    socklen_t           addr_len = sizeof(client_addr);
    char                ip_buf[INET_ADDRSTRLEN];

    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        perror("accept");
        return;
    }

    // fd_set에 새 클라이언트 fd 추가
    FD_SET(client_fd, master_set);

    // max_fd 갱신: select()의 첫 번째 인자는 max_fd + 1이어야 한다
    if (client_fd > *max_fd)
        *max_fd = client_fd;

    inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
    printf("[select] new connection: fd=%d, ip=%s, port=%d\n",
        client_fd, ip_buf, ntohs(client_addr.sin_port));
}

// -----------------------------------------------------------------------------
// handle_client: 기존 클라이언트 데이터 처리
// -----------------------------------------------------------------------------
static void handle_client(int fd, fd_set *master_set)
{
    char    buf[BUF_SIZE];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);

    if (n <= 0)
    {
        // n == 0: 클라이언트 정상 종료
        // n <  0: 에러 (ECONNRESET 등)
        if (n == 0)
            printf("[select] client fd=%d disconnected\n", fd);
        else
            perror("recv");

        // fd_set에서 제거하고 소켓 닫기
        FD_CLR(fd, master_set);
        close(fd);
        return;
    }

    // 받은 데이터 그대로 에코
    if (send(fd, buf, n, 0) < 0)
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

    // SO_REUSEADDR: 서버 재시작 시 "Address already in use" 방지
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

    printf("[select] echo server listening on port %d\n", PORT);

    // -------------------------------------------------------------------------
    // [2] fd_set 준비
    //
    // master_set: 현재 감시 중인 모든 fd의 원본 집합
    // read_set  : select()에 넘길 작업용 복사본
    //             (select()가 반환 시 read_set을 덮어쓰므로 매 반복마다 복사 필요)
    // max_fd    : 등록된 fd 중 최댓값. select()의 첫 번째 인자는 max_fd + 1
    // -------------------------------------------------------------------------
    fd_set  master_set;
    fd_set  read_set;
    int     max_fd = listen_fd;

    FD_ZERO(&master_set);
    FD_SET(listen_fd, &master_set);

    // -------------------------------------------------------------------------
    // [3] 이벤트 루프
    // -------------------------------------------------------------------------
    while (1)
    {
        // select()는 read_set을 수정하므로 매 반복마다 master_set을 복사
        read_set = master_set;

        // select() 호출: 준비된 fd가 생길 때까지 블로킹 (타임아웃 NULL = 무한 대기)
        int nready = select(max_fd + 1, &read_set, NULL, NULL, NULL);
        if (nready < 0)
        {
            if (errno == EINTR)
                continue; // 시그널 인터럽트는 재시도
            perror("select");
            break;
        }

        // 0 ~ max_fd 전체 순회하며 준비된 fd 찾기 (O(n) — select의 핵심 비효율)
        for (int i = 0; i <= max_fd; i++)
        {
            if (!FD_ISSET(i, &read_set))
                continue;

            if (i == listen_fd)
                handle_new_client(listen_fd, &master_set, &max_fd);
            else
                handle_client(i, &master_set);
        }
    }

    close(listen_fd);
    return 0;
}
