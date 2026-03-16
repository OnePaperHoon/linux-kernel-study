// =============================================================================
// TCP Multi Client (스레드 기반 동시 접속 테스트)
//
// 역할: 여러 클라이언트를 동시에 생성해 서버의 동시 처리 능력을 테스트한다.
//
// 왜 스레드(pthread)를 사용하나?
//   - fork()로 프로세스를 만들 수도 있지만, 스레드는 같은 메모리 공간을 공유하여
//     결과 집계(성공 횟수, 실패 횟수, 총 RTT)를 간단하게 처리할 수 있다.
//   - 프로세스 간 데이터 공유는 IPC(파이프, 공유 메모리 등)가 필요해 복잡하다.
//
// 실행 흐름:
//   main → N개의 스레드 생성(pthread_create)
//        → 각 스레드: connect → send → recv → close
//        → 모든 스레드 종료 대기(pthread_join)
//        → 결과 집계 출력
//
// 주의: 이 프로그램은 tcp_echo_server가 실행 중인 상태에서 테스트해야 한다.
// =============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>    // gettimeofday()
#include <arpa/inet.h>   // inet_pton(), htons(), sockaddr_in
#include <pthread.h>     // pthread_create(), pthread_join()

#define SERVER_IP       "127.0.0.1"
#define PORT            8080
#define BUF_SIZE        1024
#define NUM_CLIENTS     10   // 동시에 접속할 클라이언트 수

// -----------------------------------------------------------------------------
// 공유 결과 구조체 + 뮤텍스
//
// 여러 스레드가 동시에 같은 변수를 수정하면 경쟁 조건(race condition)이 발생한다.
// 예: success_count++는 내부적으로 "읽기 → 더하기 → 쓰기" 3단계인데,
//     두 스레드가 동시에 읽으면 하나의 증가만 반영될 수 있다.
// → pthread_mutex_lock/unlock으로 한 번에 하나의 스레드만 접근하도록 보호한다.
// -----------------------------------------------------------------------------
typedef struct s_result
{
	int    success_count; // 성공한 에코 횟수
	int    fail_count;    // 실패한 횟수
	double total_rtt;     // 전체 RTT 합계 (평균 계산용)
}	t_result;

t_result        g_result = {0, 0, 0.0}; // 전역 결과 (스레드 간 공유)
pthread_mutex_t g_mutex  = PTHREAD_MUTEX_INITIALIZER; // 뮤텍스 정적 초기화

// -----------------------------------------------------------------------------
// get_time: 현재 시각을 초 단위 실수로 반환
// -----------------------------------------------------------------------------
double get_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

// -----------------------------------------------------------------------------
// client_thread: 각 스레드가 실행하는 함수
//
// pthread_create()의 함수 포인터 타입은 void *(*)(void *) 이어야 한다.
//   - 인자: void *arg → 스레드 ID(번호)를 전달받는다
//   - 반환: void *    → NULL 반환 (결과는 전역 구조체에 기록)
//
// 각 스레드는 독립적으로:
//   1. 소켓 생성 → 서버 연결
//   2. 고유한 메시지 전송 ("Client N: hello")
//   3. 에코 응답 수신 및 RTT 측정
//   4. 공유 결과 업데이트 (mutex로 보호)
//   5. 소켓 닫기
// -----------------------------------------------------------------------------
void *client_thread(void *arg)
{
	// void *로 받은 인자를 정수로 변환 (스레드 식별 번호)
	// intptr_t를 거치는 이유: 포인터 크기와 int 크기가 다를 수 있기 때문
	int id = (int)(intptr_t)arg;

	// [1] 소켓 생성
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket");
		// mutex로 보호하며 실패 횟수 증가
		pthread_mutex_lock(&g_mutex);
		g_result.fail_count++;
		pthread_mutex_unlock(&g_mutex);
		return NULL;
	}

	// [2] 서버 주소 설정
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port   = htons(PORT);
	inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

	// [3] 서버 연결
	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("connect");
		close(sock);
		pthread_mutex_lock(&g_mutex);
		g_result.fail_count++;
		pthread_mutex_unlock(&g_mutex);
		return NULL;
	}

	// [4] 고유 메시지 생성 및 전송
	char send_buf[BUF_SIZE];
	snprintf(send_buf, sizeof(send_buf), "Client %d: hello", id);

	double start = get_time();

	if (send(sock, send_buf, strlen(send_buf), 0) == -1)
	{
		perror("send");
		close(sock);
		pthread_mutex_lock(&g_mutex);
		g_result.fail_count++;
		pthread_mutex_unlock(&g_mutex);
		return NULL;
	}

	// [5] 에코 수신
	char recv_buf[BUF_SIZE];
	ssize_t received = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
	double  rtt      = get_time() - start;

	if (received <= 0)
	{
		close(sock);
		pthread_mutex_lock(&g_mutex);
		g_result.fail_count++;
		pthread_mutex_unlock(&g_mutex);
		return NULL;
	}

	recv_buf[received] = '\0';

	// 수신 확인 출력 (여러 스레드가 동시에 printf하면 출력이 섞일 수 있어 mutex 사용)
	pthread_mutex_lock(&g_mutex);
	printf("[Client %2d] sent: %-20s | echo: %-20s | RTT: %.3f ms\n",
		id, send_buf, recv_buf, rtt * 1000.0);
	g_result.success_count++;
	g_result.total_rtt += rtt;
	pthread_mutex_unlock(&g_mutex);

	// [6] 소켓 닫기
	close(sock);
	return NULL;
}

int main()
{
	pthread_t threads[NUM_CLIENTS];

	printf("=== TCP Multi Client Test: %d clients → %s:%d ===\n\n",
		NUM_CLIENTS, SERVER_IP, PORT);

	// ---------------------------------------------------------------------
	// 스레드 생성
	//
	// pthread_create(tid, attr, func, arg)
	//   - tid : 생성된 스레드 ID를 저장할 변수
	//   - attr: 스레드 속성 (NULL = 기본값)
	//   - func: 스레드가 실행할 함수
	//   - arg : func에 전달할 인자
	//
	// (void *)(intptr_t)i : 정수 i를 포인터로 캐스팅하는 관용적 방법
	//   → 힙 할당 없이 스레드에 간단한 정수 ID를 전달할 때 사용
	// ---------------------------------------------------------------------
	double test_start = get_time();

	for (int i = 0; i < NUM_CLIENTS; i++)
	{
		if (pthread_create(&threads[i], NULL, client_thread, (void *)(intptr_t)i) != 0)
		{
			perror("pthread_create");
			// 생성 실패한 스레드는 join 대상에서 제외 (0으로 표시)
			threads[i] = 0;
		}
	}

	// ---------------------------------------------------------------------
	// 모든 스레드 종료 대기
	//
	// pthread_join(tid, retval)
	//   - 해당 스레드가 return할 때까지 현재 스레드(main)를 블로킹한다.
	//   - join하지 않으면 main이 먼저 종료되어 자식 스레드가 강제 종료될 수 있다.
	//   - retval: 스레드 반환값을 받을 포인터 (필요 없으면 NULL)
	// ---------------------------------------------------------------------
	for (int i = 0; i < NUM_CLIENTS; i++)
	{
		if (threads[i] != 0)
			pthread_join(threads[i], NULL);
	}

	double test_elapsed = get_time() - test_start;

	// 결과 요약 출력
	printf("\n=== Test Result ===\n");
	printf("Total clients : %d\n", NUM_CLIENTS);
	printf("Success       : %d\n", g_result.success_count);
	printf("Fail          : %d\n", g_result.fail_count);
	if (g_result.success_count > 0)
		printf("Avg RTT       : %.3f ms\n",
			(g_result.total_rtt / g_result.success_count) * 1000.0);
	printf("Total elapsed : %.3f ms\n", test_elapsed * 1000.0);

	return 0;
}
