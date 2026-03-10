/*
 * signal_basic.c
 *
 * 시그널 기초: signal() vs sigaction(), 주요 시그널 핸들링
 *
 * 학습 내용:
 *   1. signal()   - 단순하지만 이식성 낮음 (핸들러 자동 리셋 등 플랫폼 차이)
 *   2. sigaction() - POSIX 표준, 정밀한 제어 가능 (권장)
 *   3. SIGINT      - Ctrl+C (3번에 종료)
 *   4. SIGUSR1     - 사용자 정의 시그널 1
 *   5. SIGALRM     - alarm() 타이머
 *
 * 실행 후 테스트:
 *   - Ctrl+C    → SIGINT 핸들러 호출 (3번에 종료)
 *   - kill -USR1 <PID> → SIGUSR1 핸들러 호출
 *   - kill -ALRM <PID> → SIGALRM 핸들러 호출
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

/* 시그널 핸들러에서 안전하게 쓸 수 있는 전역 변수
 * (volatile sig_atomic_t: 시그널 핸들러와 main 간 공유) */
static volatile sig_atomic_t g_sigint_count = 0;
static volatile sig_atomic_t g_sigusr1_count = 0;
static volatile sig_atomic_t g_sigalrm_count = 0;
static volatile sig_atomic_t g_running = 1;

/* ─── 시그널 핸들러 ─────────────────────────────────── */

/* SIGINT (Ctrl+C): 3번째에 종료 */
static void handle_sigint(int sig)
{
    g_sigint_count++;
    /* write()는 시그널 핸들러에서 async-signal-safe */
    write(STDOUT_FILENO, "\n[SIGINT] Ctrl+C received!\n", 26);
    if (g_sigint_count >= 3) {
        write(STDOUT_FILENO, "[SIGINT] 3번 수신 → 프로그램 종료\n", 34);
        g_running = 0;
    } else {
        /* 남은 횟수 출력은 main에서 처리 */
    }
    (void)sig;
}

/* SIGUSR1: 사용자 정의 이벤트 처리 예시 */
static void handle_sigusr1(int sig)
{
    g_sigusr1_count++;
    write(STDOUT_FILENO, "[SIGUSR1] User-defined signal received!\n", 40);
    (void)sig;
}

/* SIGALRM: alarm() 타이머 만료 */
static void handle_sigalrm(int sig)
{
    g_sigalrm_count++;
    write(STDOUT_FILENO, "[SIGALRM] Timer expired!\n", 25);
    alarm(3);   /* 3초마다 반복 */
    (void)sig;
}

/* ─── 시그널 등록 ────────────────────────────────────── */

/*
 * signal() 방식 (구식, 비권장):
 *   - 핸들러 자동 리셋 여부가 플랫폼마다 다름
 *   - 핸들러 실행 중 같은 시그널 블로킹 보장 없음
 */
static void register_with_signal(void)
{
    if (signal(SIGUSR1, handle_sigusr1) == SIG_ERR) {
        perror("signal SIGUSR1");
        exit(EXIT_FAILURE);
    }
    printf("[setup] SIGUSR1 registered via signal()\n");
}

/*
 * sigaction() 방식 (권장):
 *   - sa_mask: 핸들러 실행 중 추가 블로킹할 시그널
 *   - SA_RESTART: 시그널로 인해 중단된 syscall 자동 재시작
 *   - SA_RESETHAND: 핸들러 1회 실행 후 SIG_DFL로 리셋
 */
static void register_with_sigaction(void)
{
    struct sigaction sa;

    /* SIGINT */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }
    printf("[setup] SIGINT  registered via sigaction()\n");

    /* SIGALRM */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigalrm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction SIGALRM");
        exit(EXIT_FAILURE);
    }
    printf("[setup] SIGALRM registered via sigaction()\n");
}

/* ─── Main ──────────────────────────────────────────── */

int main(void)
{
    printf("=== Signal Basic Example ===\n");
    printf("[Main] PID: %d\n\n", getpid());

    /* 시그널 핸들러 등록 */
    register_with_sigaction();  /* SIGINT, SIGALRM */
    register_with_signal();     /* SIGUSR1 */

    /* SIGALRM 타이머 시작: 3초 후 첫 발생 */
    alarm(3);
    printf("\n[Main] Running... (Ctrl+C 3번으로 종료)\n");
    printf("[Main] 다른 터미널에서: kill -USR1 %d\n\n", getpid());

    /* 이벤트 루프: 시그널을 기다리며 상태 출력 */
    while (g_running) {
        /* pause(): 시그널이 올 때까지 프로세스 중단 (CPU 낭비 없음) */
        pause();

        /* 핸들러 실행 후 여기서 상태 안전하게 출력
         * (printf는 async-signal-unsafe이므로 핸들러 밖에서) */
        if (g_sigint_count > 0 && g_sigint_count < 3) {
            printf("[Main] SIGINT %d/3 수신 (앞으로 %d번 더)\n",
                   (int)g_sigint_count, 3 - (int)g_sigint_count);
        }
        if (g_sigusr1_count > 0) {
            printf("[Main] SIGUSR1 총 %d회 수신\n", (int)g_sigusr1_count);
            g_sigusr1_count = 0;    /* 카운트 리셋 */
        }
        if (g_sigalrm_count > 0) {
            printf("[Main] SIGALRM 총 %d회 발생 (3초마다)\n",
                   (int)g_sigalrm_count);
        }
    }

    /* alarm 취소 */
    alarm(0);

    printf("\n[Main] Exiting (SIGINT %d회, SIGUSR1 %d회, SIGALRM %d회)\n",
           (int)g_sigint_count, (int)g_sigusr1_count, (int)g_sigalrm_count);

    return 0;
}
