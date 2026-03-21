// =============================================================================
// benchmark.c — select / poll / epoll 서버 성능 비교 벤치마크
//
// 목적: 동일한 부하를 각 서버에 가해서 처리량(RPS)과 평균 RTT를 비교한다.
//
// 사용법:
//   서버를 먼저 실행한 뒤 이 프로그램을 실행한다.
//
//   ./select_echo_server &   (port 8001)
//   ./poll_echo_server &     (port 8002)
//   ./epoll_echo_server &    (port 8003, Linux only)
//
//   ./benchmark select       → port 8001 벤치마크
//   ./benchmark poll         → port 8002 벤치마크
//   ./benchmark epoll        → port 8003 벤치마크
//   ./benchmark all          → 세 서버 순서대로 모두 측정 후 비교
//
// 측정 지표:
//   - 총 소요 시간 (elapsed)
//   - 평균 RTT (total_rtt / success)
//   - 초당 처리 요청 수 (RPS = success / elapsed)
//   - 성공/실패 횟수
//
// 전략:
//   - NUM_CLIENTS개 스레드를 동시에 생성해 서버를 압박한다.
//   - 각 스레드는 연결 1개에서 NUM_REQUESTS번 반복 send/recv (keep-alive 패턴).
//   - tcp_multi_client.c의 RTT 측정 방식을 재활용한다.
// =============================================================================

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_IP       "127.0.0.1"
#define SELECT_PORT     8001
#define POLL_PORT       8002
#define EPOLL_PORT      8003
#define BUF_SIZE        64
#define NUM_CLIENTS     100  // 동시 클라이언트 수
#define NUM_REQUESTS    10   // 클라이언트당 반복 요청 수

// -----------------------------------------------------------------------------
// 공유 결과 구조체
// -----------------------------------------------------------------------------
typedef struct s_result
{
    int    success;
    int    fail;
    double total_rtt; // 단위: 초
}   t_result;

static t_result         g_result;
static pthread_mutex_t  g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int              g_port;

// -----------------------------------------------------------------------------
// get_time: 현재 시각을 초 단위 double로 반환
// -----------------------------------------------------------------------------
static double get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

// -----------------------------------------------------------------------------
// client_thread: 각 스레드가 실행하는 함수
//
// - 소켓 1개로 서버에 연결한 뒤 NUM_REQUESTS번 반복 에코 요청
// - 연결 단위 RTT가 아니라 요청 단위 RTT를 측정
// - 결과는 mutex로 보호하며 g_result에 기록
// -----------------------------------------------------------------------------
static void *client_thread(void *arg)
{
    int id = (int)(intptr_t)arg;

    // 소켓 생성
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        pthread_mutex_lock(&g_mutex);
        g_result.fail += NUM_REQUESTS;
        pthread_mutex_unlock(&g_mutex);
        return NULL;
    }

    // 서버 연결
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(g_port);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sock);
        pthread_mutex_lock(&g_mutex);
        g_result.fail += NUM_REQUESTS;
        pthread_mutex_unlock(&g_mutex);
        return NULL;
    }

    // NUM_REQUESTS번 반복 에코 (keep-alive: 연결 유지한 채 반복)
    for (int r = 0; r < NUM_REQUESTS; r++)
    {
        char    send_buf[BUF_SIZE];
        char    recv_buf[BUF_SIZE];
        ssize_t received;

        snprintf(send_buf, sizeof(send_buf), "C%d-R%d", id, r);

        double start = get_time();

        if (send(sock, send_buf, strlen(send_buf), 0) < 0)
        {
            pthread_mutex_lock(&g_mutex);
            g_result.fail++;
            pthread_mutex_unlock(&g_mutex);
            continue;
        }

        received = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
        double rtt = get_time() - start;

        if (received <= 0)
        {
            pthread_mutex_lock(&g_mutex);
            g_result.fail++;
            pthread_mutex_unlock(&g_mutex);
            continue;
        }

        pthread_mutex_lock(&g_mutex);
        g_result.success++;
        g_result.total_rtt += rtt;
        pthread_mutex_unlock(&g_mutex);
    }

    close(sock);
    return NULL;
}

// -----------------------------------------------------------------------------
// run_benchmark: 벤치마크 수행 및 결과 출력
// -----------------------------------------------------------------------------
static void run_benchmark(const char *label, int port)
{
    pthread_t threads[NUM_CLIENTS];
    int       total_req = NUM_CLIENTS * NUM_REQUESTS;

    // 전역 결과 초기화
    g_port          = port;
    g_result.success    = 0;
    g_result.fail       = 0;
    g_result.total_rtt  = 0.0;

    printf("\n[benchmark] %s (port %d) — %d clients × %d requests = %d total\n",
        label, port, NUM_CLIENTS, NUM_REQUESTS, total_req);

    double start = get_time();

    // 스레드 생성
    for (int i = 0; i < NUM_CLIENTS; i++)
    {
        if (pthread_create(&threads[i], NULL, client_thread, (void *)(intptr_t)i) != 0)
        {
            perror("pthread_create");
            threads[i] = 0;
        }
    }

    // 모든 스레드 완료 대기
    for (int i = 0; i < NUM_CLIENTS; i++)
    {
        if (threads[i] != 0)
            pthread_join(threads[i], NULL);
    }

    double elapsed = get_time() - start;

    // 결과 출력
    double avg_rtt_ms = (g_result.success > 0)
        ? (g_result.total_rtt / g_result.success) * 1000.0
        : 0.0;
    double rps = (elapsed > 0.0) ? g_result.success / elapsed : 0.0;

    printf("  success  : %d / %d\n", g_result.success, total_req);
    printf("  fail     : %d\n", g_result.fail);
    printf("  avg RTT  : %.3f ms\n", avg_rtt_ms);
    printf("  elapsed  : %.3f ms\n", elapsed * 1000.0);
    printf("  RPS      : %.1f req/sec\n", rps);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <select|poll|epoll|all>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "select") == 0)
        run_benchmark("select", SELECT_PORT);
    else if (strcmp(argv[1], "poll") == 0)
        run_benchmark("poll", POLL_PORT);
    else if (strcmp(argv[1], "epoll") == 0)
        run_benchmark("epoll", EPOLL_PORT);
    else if (strcmp(argv[1], "all") == 0)
    {
        // 각 서버가 미리 실행 중이어야 한다
        run_benchmark("select", SELECT_PORT);
        run_benchmark("poll",   POLL_PORT);
        run_benchmark("epoll",  EPOLL_PORT);

        printf("\n=== summary ===\n");
        printf("  (위 결과를 비교하세요)\n");
    }
    else
    {
        fprintf(stderr, "unknown target: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
