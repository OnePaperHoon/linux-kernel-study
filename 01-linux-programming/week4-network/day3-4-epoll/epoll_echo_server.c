// =============================================================================
// epoll_echo_server.c — epoll() 기반 에코 서버 (Linux 전용)
//
// 목적: Linux 전용 epoll을 이용해 select/poll의 O(n) 한계를 극복하고
//       대규모 동시 접속을 효율적으로 처리한다.
//
// epoll이 select/poll보다 빠른 이유:
//   - 관심 fd를 커널 내부 자료구조(레드블랙 트리)에 등록한다.
//     → 매 호출마다 fd 목록을 커널에 복사할 필요 없다.
//   - 이벤트가 발생한 fd만 결과 배열에 담겨 반환된다.
//     → 전체 fd를 순회하지 않아도 된다 (O(1) 탐색).
//   - 수만 개의 fd를 관리해도 성능 저하가 거의 없다.
//
// 핵심 API:
//   epoll_create1(0)              : epoll 인스턴스 생성, epfd 반환
//   epoll_ctl(epfd, op, fd, &ev) : fd 등록/수정/삭제
//     EPOLL_CTL_ADD : 새 fd 등록
//     EPOLL_CTL_DEL : fd 제거 (close 전에 호출 권장)
//   epoll_wait(epfd, events, max, timeout) : 이벤트 대기
//     events  : 이벤트가 발생한 fd 정보가 담길 배열 (커널이 채움)
//     반환값  : 실제로 이벤트가 발생한 fd 수
//
// LT(Level Trigger, 기본) vs ET(Edge Trigger, EPOLLET):
//   - LT: 데이터가 남아있는 한 계속 이벤트 발생 → 구현 쉬움 (이 예제)
//   - ET: 상태 변화(버퍼가 빔→참) 시점에만 이벤트 1회 발생
//         → 비블로킹 소켓 + EAGAIN까지 루프 필수, 고성능 서버에 적합
//
// 실행 흐름:
//   main → 리슨 소켓 생성 + bind + listen
//        → epoll_create1(0)으로 epoll 인스턴스 생성
//        → epoll_ctl로 listen_fd 등록 (EPOLLIN)
//        → 이벤트 루프:
//            epoll_wait()로 이벤트 대기
//            events[i].data.fd == listen_fd → accept() → epoll_ctl ADD
//            events[i].data.fd != listen_fd → recv() → 에코 또는 종료 처리
// =============================================================================

#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define PORT        8003
#define BUF_SIZE    1024
// epoll_wait()에 한 번에 반환받을 최대 이벤트 수
// (fd 최대 개수가 아니라 한 번에 처리할 배치 크기)
#define MAX_EVENTS  64

// -----------------------------------------------------------------------------
// handle_new_client: accept() 후 epoll에 등록
// -----------------------------------------------------------------------------
static void handle_new_client(int epfd, int listen_fd)
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

    // 새 클라이언트 fd를 epoll에 등록
    struct epoll_event ev;
    ev.events  = EPOLLIN;       // 읽기 이벤트 감시 (LT 모드)
    ev.data.fd = client_fd;     // 이벤트 발생 시 식별할 fd 저장

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
    {
        perror("epoll_ctl ADD");
        close(client_fd);
        return;
    }

    inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
    printf("[epoll] new connection: fd=%d, ip=%s, port=%d\n",
        client_fd, ip_buf, ntohs(client_addr.sin_port));
}

// -----------------------------------------------------------------------------
// handle_client: 기존 클라이언트 데이터 처리
// -----------------------------------------------------------------------------
static void handle_client(int epfd, int fd)
{
    char    buf[BUF_SIZE];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);

    if (n <= 0)
    {
        if (n == 0)
            printf("[epoll] client fd=%d disconnected\n", fd);
        else
            perror("recv");

        // epoll에서 제거 후 소켓 닫기
        // (커널 2.6.9 이후로 close()만 해도 자동 제거되지만 명시적으로 삭제 권장)
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return;
    }

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

    printf("[epoll] echo server listening on port %d\n", PORT);

    // -------------------------------------------------------------------------
    // [2] epoll 인스턴스 생성
    //
    // epoll_create1(0): 인자 0은 flags (EPOLL_CLOEXEC 등, 지금은 불필요)
    // 반환된 epfd는 일반 파일 디스크립터 → 종료 시 close() 필요
    // -------------------------------------------------------------------------
    int epfd = epoll_create1(0);
    if (epfd < 0)
    {
        perror("epoll_create1");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // -------------------------------------------------------------------------
    // [3] listen_fd를 epoll에 등록
    // -------------------------------------------------------------------------
    struct epoll_event ev;
    ev.events  = EPOLLIN;
    ev.data.fd = listen_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
    {
        perror("epoll_ctl ADD listen_fd");
        close(epfd);
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // -------------------------------------------------------------------------
    // [4] 이벤트 루프
    // -------------------------------------------------------------------------
    struct epoll_event events[MAX_EVENTS];

    while (1)
    {
        // epoll_wait: 이벤트가 발생한 fd 목록만 events[]에 채워서 반환
        // select/poll과 달리 전체 fd를 순회하지 않아도 됨
        int nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready < 0)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }

        // 준비된 fd만 처리 (O(nready), nready << 전체 fd 수)
        for (int i = 0; i < nready; i++)
        {
            int fd = events[i].data.fd;

            if (fd == listen_fd)
                handle_new_client(epfd, listen_fd);
            else
                handle_client(epfd, fd);
        }
    }

    close(epfd);
    close(listen_fd);
    return 0;
}
