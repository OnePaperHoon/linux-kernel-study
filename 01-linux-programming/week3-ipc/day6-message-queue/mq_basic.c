/*
 * mq_basic.c
 *
 * System V Message Queue 기본 예제
 * fork()로 생성된 Sender(부모)와 Receiver(자식) 간 통신
 *
 * 핵심 API:
 *   msgget()  - 메시지 큐 생성/접근
 *   msgsnd()  - 메시지 전송
 *   msgrcv()  - 메시지 수신 (mtype으로 우선순위 제어)
 *   msgctl()  - 메시지 큐 제어 (삭제 등)
 *
 * Pipe와 차이점:
 *   Pipe     : 바이트 스트림, 순서 고정 (FIFO)
 *   Msg Queue: 개별 메시지 단위, mtype으로 선택적 수신 가능
 *
 * 이 예제의 핵심: 메시지 타입(mtype)을 이용한 우선순위 수신
 *   - 낮은 타입 번호 = 높은 우선순위
 *   - mtype=0  → FIFO (도착 순서대로)
 *   - mtype=N  → 정확히 타입 N인 것만
 *   - mtype=-N → 절대값 ≤ N 중 가장 낮은 타입부터
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define MSG_TEXT_SIZE   128

/* 메시지 구조체: 첫 필드는 반드시 long mtype */
typedef struct {
    long    mtype;              /* 메시지 타입 (1 이상) */
    char    mtext[MSG_TEXT_SIZE];
} t_msg;

/* 전송할 메시지 목록 (type, text) */
typedef struct {
    long        type;
    const char  *text;
} t_msg_data;

static const t_msg_data g_messages[] = {
    {2, "[LOW]  B 업무 처리 요청"},
    {2, "[LOW]  C 업무 처리 요청"},
    {1, "[HIGH] A 긴급 처리 요청"},
    {2, "[LOW]  D 업무 처리 요청"},
    {1, "[HIGH] E 긴급 처리 요청"},
};
#define MSG_COUNT   (int)(sizeof(g_messages) / sizeof(g_messages[0]))

void sender_process(int msqid)
{
    t_msg   msg;
    int     i;

    printf("[Sender] Starting (PID: %d)\n", getpid());
    printf("[Sender] Sending %d messages (순서대로):\n\n", MSG_COUNT);

    for (i = 0; i < MSG_COUNT; i++) {
        msg.mtype = g_messages[i].type;
        strncpy(msg.mtext, g_messages[i].text, MSG_TEXT_SIZE - 1);
        msg.mtext[MSG_TEXT_SIZE - 1] = '\0';

        if (msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }
        printf("[Sender] Sent  [type=%ld]: %s\n", msg.mtype, msg.mtext);
        usleep(100000); /* 0.1s (전송 순서 확인용) */
    }

    printf("\n[Sender] Done\n");
}

void receiver_process(int msqid)
{
    t_msg   msg;
    int     i;
    ssize_t n;

    printf("[Receiver] Starting (PID: %d)\n", getpid());

    /* 1. Sender가 전부 넣을 때까지 대기 */
    sleep(2);

    /* 2. 우선순위 수신: mtype=1 (긴급) 먼저, 그다음 mtype=2 (일반)
     *
     *   큐에 쌓인 순서: type=2, type=2, type=1, type=2, type=1
     *   수신 순서:      type=1, type=1, type=2, type=2, type=2
     *
     *   Pipe와 달리 도착 순서와 무관하게 원하는 타입을 먼저 꺼낼 수 있음
     *
     *   ※ mtype=-N (절대값 이하 중 최솟값 우선)은 Linux에서 동작하나
     *      macOS에서는 FIFO처럼 동작할 수 있어 명시적 mtype 지정 사용
     */
    printf("[Receiver] Phase 1: mtype=1 (긴급) 먼저 수신:\n");
    for (i = 0; i < 2; i++) {   /* type=1 메시지 2개 */
        n = msgrcv(msqid, &msg, sizeof(msg.mtext), 1, 0);
        if (n == -1) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        printf("[Receiver] Got [type=%ld]: %s\n", msg.mtype, msg.mtext);
    }

    printf("\n[Receiver] Phase 2: mtype=2 (일반) 수신:\n");
    for (i = 0; i < 3; i++) {   /* type=2 메시지 3개 */
        n = msgrcv(msqid, &msg, sizeof(msg.mtext), 2, 0);
        if (n == -1) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        printf("[Receiver] Got [type=%ld]: %s\n", msg.mtype, msg.mtext);
    }

    printf("\n[Receiver] Done\n");
}

int main(void)
{
    int     msqid;
    pid_t   pid;

    printf("=== System V Message Queue Basic Example ===\n");
    printf("    (큐에 type=2,2,1,2,1 순서로 쌓아도\n");
    printf("     mtype=1 먼저 수신하면 도착 순서 무관하게 가져옴)\n\n");

    /* 1. 메시지 큐 생성
     *   IPC_PRIVATE: fork로 연결될 프로세스끼리만 공유
     *   0666: 읽기/쓰기 권한
     */
    msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    printf("[Main] Message queue created (msqid: %d)\n\n", msqid);

    /* 2. Fork */
    fflush(stdout);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        msgctl(msqid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* 자식: Receiver */
        receiver_process(msqid);
        exit(EXIT_SUCCESS);
    } else {
        /* 부모: Sender */
        sender_process(msqid);
        waitpid(pid, NULL, 0);
    }

    /* 3. 메시지 큐 삭제 */
    printf("\n[Main] Removing message queue (msqid: %d)\n", msqid);
    if (msgctl(msqid, IPC_RMID, NULL) == -1)
        perror("msgctl IPC_RMID");

    printf("\n=== Message Queue Communication Complete ===\n");

    return 0;
}
