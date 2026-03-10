/*
 * signal_parent_child.c
 *
 * 시그널을 이용한 부모-자식 프로세스 간 통신
 *
 * 흐름:
 *   부모 ──SIGUSR1──▶ 자식 (작업 시작 신호)
 *   자식 ──SIGUSR2──▶ 부모 (작업 완료 응답)
 *   N 라운드 후 부모 ──SIGTERM──▶ 자식 종료
 *
 * 핵심 개념:
 *   - kill(pid, sig): 다른 프로세스에 시그널 전송
 *   - getppid():      부모 PID 얻기 (자식 → 부모 방향 전송에 사용)
 *   - sigwaitinfo() 대신 pause() + volatile flag 방식 사용
 *   - SIGCHLD:        부모가 자식 종료를 감지하는 시그널
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#define ROUNDS  5   /* 총 라운드 수 */

/* 공유하지 않는 전역 변수 (fork 후 각자 독립) */
static volatile sig_atomic_t g_got_usr1  = 0;
static volatile sig_atomic_t g_got_usr2  = 0;
static volatile sig_atomic_t g_got_term  = 0;
static volatile sig_atomic_t g_got_chld  = 0;

/* ─── 시그널 핸들러 ─────────────────────────────────── */

static void handle_sigusr1(int sig)
{
    g_got_usr1 = 1;
    (void)sig;
}

static void handle_sigusr2(int sig)
{
    g_got_usr2 = 1;
    (void)sig;
}

static void handle_sigterm(int sig)
{
    g_got_term = 1;
    (void)sig;
}

static void handle_sigchld(int sig)
{
    g_got_chld = 1;
    (void)sig;
}

/* ─── 핸들러 등록 (sigaction) ───────────────────────── */

static void setup_signal(int signum, void (*handler)(int))
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(signum, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

/* ─── 자식 프로세스 ─────────────────────────────────── */

static void child_process(pid_t parent_pid)
{
    int round;

    printf("[Child] Started (PID: %d, Parent PID: %d)\n",
           getpid(), (int)parent_pid);

    setup_signal(SIGUSR1, handle_sigusr1);
    setup_signal(SIGTERM, handle_sigterm);

    for (round = 1; round <= ROUNDS; round++) {
        /* 부모의 SIGUSR1 대기 */
        printf("[Child] Round %d: Waiting for SIGUSR1...\n", round);
        while (!g_got_usr1) {
            if (g_got_term) {
                printf("[Child] SIGTERM received. Exiting.\n");
                exit(EXIT_SUCCESS);
            }
            pause();
        }
        g_got_usr1 = 0;

        /* 작업 수행 (모의) */
        printf("[Child] Round %d: SIGUSR1 received! Working...\n", round);
        usleep(200000); /* 0.2s 작업 */

        /* 완료 신호: 부모에게 SIGUSR2 전송 */
        printf("[Child] Round %d: Sending SIGUSR2 to parent\n", round);
        if (kill(parent_pid, SIGUSR2) == -1) {
            perror("kill SIGUSR2");
            exit(EXIT_FAILURE);
        }
    }

    /* 부모의 SIGTERM 대기 */
    printf("[Child] All rounds done. Waiting for SIGTERM...\n");
    while (!g_got_term)
        pause();

    printf("[Child] SIGTERM received. Exiting cleanly.\n");
    exit(EXIT_SUCCESS);
}

/* ─── 부모 프로세스 ─────────────────────────────────── */

static void parent_process(pid_t child_pid)
{
    int round;

    printf("[Parent] Started (PID: %d, Child PID: %d)\n",
           getpid(), (int)child_pid);

    setup_signal(SIGUSR2, handle_sigusr2);
    setup_signal(SIGCHLD, handle_sigchld);

    /* 자식이 핸들러를 등록할 시간 */
    usleep(100000);

    for (round = 1; round <= ROUNDS; round++) {
        /* 작업 신호: 자식에게 SIGUSR1 전송 */
        printf("[Parent] Round %d: Sending SIGUSR1 to child\n", round);
        if (kill(child_pid, SIGUSR1) == -1) {
            perror("kill SIGUSR1");
            exit(EXIT_FAILURE);
        }

        /* 자식의 SIGUSR2 응답 대기 */
        printf("[Parent] Round %d: Waiting for SIGUSR2...\n", round);
        while (!g_got_usr2) {
            if (g_got_chld) {
                /* 자식이 예상치 못하게 종료됨 */
                printf("[Parent] Child exited unexpectedly!\n");
                goto cleanup;
            }
            pause();
        }
        g_got_usr2 = 0;

        printf("[Parent] Round %d: SIGUSR2 received! ✓\n\n", round);
        usleep(100000);
    }

    /* 자식 종료 지시 */
    printf("[Parent] All rounds complete. Sending SIGTERM to child\n");
    kill(child_pid, SIGTERM);

cleanup:
    /* 자식 종료 대기 */
    waitpid(child_pid, NULL, 0);
    printf("[Parent] Child exited. Done.\n");
}

/* ─── Main ──────────────────────────────────────────── */

int main(void)
{
    pid_t   pid;
    pid_t   my_pid;

    printf("=== Signal Parent-Child Communication ===\n");
    printf("    부모 ──SIGUSR1──▶ 자식\n");
    printf("    자식 ──SIGUSR2──▶ 부모\n\n");

    my_pid = getpid();

    fflush(stdout);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* 자식 */
        child_process(my_pid);
    } else {
        /* 부모 */
        parent_process(pid);
    }

    return 0;
}
