// =============================================================================
// http_server.c — epoll 기반 HTTP 정적 파일 서버 (Linux 전용)
//
// 목적: HTTP/1.1 GET 요청을 받아 www/ 디렉토리의 정적 파일(HTML, CSS 등)을
//       제공한다. epoll을 사용해 다수의 동시 접속을 효율적으로 처리한다.
//
// HTTP/1.1 응답 메시지 구조:
//   "<버전> <상태코드> <상태문구>\r\n"  ← 응답 라인
//   "<헤더이름>: <헤더값>\r\n"          ← 응답 헤더
//   "\r\n"                              ← 헤더 끝 구분자 (빈 줄 필수)
//   "<바디>"                            ← 응답 바디 (파일 내용)
//
// 파일 서빙 흐름:
//   클라이언트 요청 수신
//   → parse_http_request()로 HTTP 파싱
//   → 경로를 "www/<path>"로 매핑
//   → fopen()으로 파일 존재 확인
//   → 파일 있음: 200 OK + 파일 내용 전송
//   → 파일 없음: 404 Not Found + 404.html 전송
//
// 실행 흐름:
//   main → 소켓 생성 + SO_REUSEADDR + bind + listen
//        → epoll_create1(0) 으로 epoll 인스턴스 생성
//        → listen_fd를 epoll에 등록 (EPOLLIN)
//        → 이벤트 루프:
//            listen_fd 이벤트 → handle_new_client (accept + epoll 등록)
//            client_fd 이벤트 → handle_client (recv → HTTP 처리 → close)
//
// 주의:
//   이 서버는 HTTP/1.0 방식으로 응답 후 연결을 즉시 종료한다.
//   (Connection: close — keep-alive 미지원)
// =============================================================================

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "http_parser.h"

#define PORT        8080
#define BUF_SIZE    8192        // 요청 수신 버퍼 크기
#define MAX_EVENTS  64          // epoll_wait 한 번에 처리할 최대 이벤트 수
#define WWW_ROOT    "./www"     // 정적 파일 루트 디렉토리

// =============================================================================
// get_mime_type: 파일 확장자 → MIME 타입 문자열 반환
//
// Content-Type 헤더에 사용된다.
// 브라우저는 이 값을 보고 파일을 어떻게 렌더링할지 결정한다.
// =============================================================================
static const char   *get_mime_type(const char *path)
{
    const char  *ext = strrchr(path, '.');  // 마지막 '.' 이후 확장자 추출

    if (!ext)
        return ("application/octet-stream");
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
        return ("text/html; charset=utf-8");
    if (strcmp(ext, ".css") == 0)
        return ("text/css; charset=utf-8");
    if (strcmp(ext, ".js") == 0)
        return ("application/javascript");
    if (strcmp(ext, ".png") == 0)
        return ("image/png");
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return ("image/jpeg");
    if (strcmp(ext, ".ico") == 0)
        return ("image/x-icon");
    return ("application/octet-stream");    // 알 수 없는 확장자 → 바이너리 스트림
}

// =============================================================================
// send_response: HTTP 응답 헤더 + 바디 전송
//
// 응답 형식:
//   "HTTP/1.1 <status_code> <status_text>\r\n"
//   "Content-Type: <content_type>\r\n"
//   "Content-Length: <body_len>\r\n"
//   "Connection: close\r\n"
//   "\r\n"
//   <body>
// =============================================================================
static void send_response(int client_fd, int status_code,
        const char *status_text, const char *content_type,
        const char *body, size_t body_len)
{
    char    header[512];
    int     header_len;

    // 응답 헤더 구성
    header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_text,
        content_type,
        body_len);

    // 헤더 전송
    send(client_fd, header, header_len, 0);

    // 바디 전송 (HEAD 요청이 아닐 때만, 여기선 항상 전송)
    if (body && body_len > 0)
        send(client_fd, body, body_len, 0);
}

// =============================================================================
// send_file: 파일 내용을 읽어 HTTP 200 응답으로 전송
//
// fread()로 파일 전체를 메모리에 읽은 뒤 send_response()를 호출한다.
// 대용량 파일은 sendfile() 시스템 콜이 더 효율적이지만 이 예제에서는 단순화.
//
// 반환값: 0 성공, -1 파일 없음/읽기 오류
// =============================================================================
static int  send_file(int client_fd, const char *file_path)
{
    FILE        *fp;
    struct stat st;
    char        *file_buf;
    size_t      file_size;

    // 파일 크기 확인 (stat 사용)
    if (stat(file_path, &st) < 0)
        return (-1);    // 파일 없음

    file_size = (size_t)st.st_size;

    fp = fopen(file_path, "rb");
    if (!fp)
        return (-1);

    // 파일 내용을 메모리에 읽기
    file_buf = malloc(file_size);
    if (!file_buf)
    {
        fclose(fp);
        return (-1);
    }
    fread(file_buf, 1, file_size, fp);
    fclose(fp);

    // 200 OK 응답 전송
    send_response(client_fd, 200, "OK",
        get_mime_type(file_path), file_buf, file_size);

    free(file_buf);
    return (0);
}

// =============================================================================
// send_404: 404 Not Found 응답 전송
//
// www/404.html 파일을 읽어 전송한다.
// 파일이 없으면 간단한 HTML 문자열로 대체한다.
// =============================================================================
static void send_404(int client_fd)
{
    const char  *fallback =
        "<html><body><h1>404 Not Found</h1></body></html>";

    // www/404.html 전송 시도
    if (send_file(client_fd, WWW_ROOT "/404.html") < 0)
    {
        // 404.html 파일 자체가 없는 경우 인라인 HTML 반환
        send_response(client_fd, 404, "Not Found",
            "text/html; charset=utf-8",
            fallback, strlen(fallback));
    }
}

// =============================================================================
// handle_http_request: HTTP 요청 파싱 후 정적 파일 서빙
//
// 처리 흐름:
//   1. parse_http_request()로 요청 라인 파싱
//   2. 유효하지 않은 요청 → 400 Bad Request
//   3. GET 이외 메서드 → 405 Method Not Allowed
//   4. 경로 "/" → "/index.html"로 리다이렉트 처리
//   5. "www/<path>" 파일 전송 시도 → 실패 시 404
// =============================================================================
static void handle_http_request(int client_fd, const char *raw)
{
    t_http_request  req;
    char            file_path[512];
    const char      *path;
    const char      *bad_req = "<html><body><h1>400 Bad Request</h1></body></html>";
    const char      *not_allowed = "<html><body><h1>405 Method Not Allowed</h1></body></html>";

    // HTTP 요청 파싱
    parse_http_request(raw, &req);

    // [1] 파싱 실패 → 400 Bad Request
    if (!req.valid)
    {
        send_response(client_fd, 400, "Bad Request",
            "text/html; charset=utf-8",
            bad_req, strlen(bad_req));
        return ;
    }

    // [2] GET 이외 메서드 → 405 Method Not Allowed
    if (req.method != HTTP_GET)
    {
        send_response(client_fd, 405, "Method Not Allowed",
            "text/html; charset=utf-8",
            not_allowed, strlen(not_allowed));
        return ;
    }

    // [3] 경로 정규화: "/" → "/index.html"
    path = req.path;
    if (strcmp(path, "/") == 0)
        path = "/index.html";

    // [4] 파일 경로 구성: WWW_ROOT + path
    // ex) "./www" + "/index.html" → "./www/index.html"
    snprintf(file_path, sizeof(file_path), "%s%s", WWW_ROOT, path);

    printf("[http] GET %s → %s\n", req.path, file_path);

    // [5] 파일 전송 시도 → 실패 시 404
    if (send_file(client_fd, file_path) < 0)
    {
        printf("[http] 404 Not Found: %s\n", file_path);
        send_404(client_fd);
    }
}

// =============================================================================
// handle_new_client: 새 연결 수락 후 epoll에 등록
// =============================================================================
static void handle_new_client(int epfd, int listen_fd)
{
    struct sockaddr_in  client_addr;
    socklen_t           addr_len = sizeof(client_addr);
    struct epoll_event  ev;
    char                ip_buf[INET_ADDRSTRLEN];
    int                 client_fd;

    client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        perror("accept");
        return ;
    }

    // 새 클라이언트 fd를 epoll에 등록
    ev.events  = EPOLLIN;       // 읽기 이벤트 감시 (LT 모드)
    ev.data.fd = client_fd;     // 이벤트 발생 시 식별할 fd 저장

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
    {
        perror("epoll_ctl ADD");
        close(client_fd);
        return ;
    }

    inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
    printf("[http] new connection: fd=%d, ip=%s, port=%d\n",
        client_fd, ip_buf, ntohs(client_addr.sin_port));
}

// =============================================================================
// handle_client: 클라이언트로부터 HTTP 요청 수신 후 처리
//
// 이 서버는 Connection: close 방식을 사용한다.
// 요청을 처리한 뒤 즉시 epoll에서 제거하고 소켓을 닫는다.
// =============================================================================
static void handle_client(int epfd, int client_fd)
{
    char    buf[BUF_SIZE];
    ssize_t n;

    n = recv(client_fd, buf, sizeof(buf) - 1, 0);

    if (n <= 0)
    {
        if (n == 0)
            printf("[http] client fd=%d disconnected\n", client_fd);
        else
            perror("recv");

        // epoll에서 제거 후 소켓 닫기
        // (커널 2.6.9+ 에서는 close()만 해도 자동 제거되지만 명시적 삭제 권장)
        epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
        return ;
    }

    buf[n] = '\0';  // 문자열로 처리하기 위해 null-terminate

    // HTTP 요청 처리 (파싱 → 파일 서빙)
    handle_http_request(client_fd, buf);

    // HTTP/1.0 방식: 응답 후 연결 종료
    epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
}

// =============================================================================
// main: 서버 초기화 + epoll 이벤트 루프
// =============================================================================
int main(void)
{
    int                 listen_fd;
    int                 epfd;
    struct sockaddr_in  server_addr;
    struct epoll_event  ev;
    struct epoll_event  events[MAX_EVENTS];
    int                 opt;
    int                 nready;

    // -------------------------------------------------------------------------
    // [1] 리슨 소켓 생성
    // -------------------------------------------------------------------------
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // SO_REUSEADDR: 서버 재시작 시 "Address already in use" 에러 방지
    opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // -------------------------------------------------------------------------
    // [2] bind + listen
    // -------------------------------------------------------------------------
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

    printf("[http] server listening on port %d\n", PORT);
    printf("[http] serving files from: %s\n", WWW_ROOT);

    // -------------------------------------------------------------------------
    // [3] epoll 인스턴스 생성
    //
    // epoll_create1(0): flags=0, 반환된 epfd는 일반 fd → 종료 시 close() 필요
    // -------------------------------------------------------------------------
    epfd = epoll_create1(0);
    if (epfd < 0)
    {
        perror("epoll_create1");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // -------------------------------------------------------------------------
    // [4] listen_fd를 epoll에 등록
    // -------------------------------------------------------------------------
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
    // [5] 이벤트 루프
    // -------------------------------------------------------------------------
    while (1)
    {
        // epoll_wait: 이벤트가 발생한 fd 목록만 events[]에 채워서 반환
        // -1: 타임아웃 없이 무한 대기
        nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready < 0)
        {
            if (errno == EINTR)     // 시그널 인터럽트는 재시도
                continue;
            perror("epoll_wait");
            break;
        }

        // 준비된 fd만 처리 (O(nready), nready << 전체 fd 수)
        for (int i = 0; i < nready; i++)
        {
            int fd = events[i].data.fd;

            if (fd == listen_fd)
                handle_new_client(epfd, listen_fd);     // 새 연결 수락
            else
                handle_client(epfd, fd);                // HTTP 요청 처리
        }
    }

    close(epfd);
    close(listen_fd);
    return (0);
}
