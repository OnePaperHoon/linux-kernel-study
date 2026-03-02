/*
 * mq_receiver.c
 *
 * System V Message Queue: 독립 Receiver 프로세스
 * mq_sender보다 먼저 실행 필요
 *
 * 실행 순서:
 *   Terminal 1: ./mq_receiver   ← 먼저 실행
 *   Terminal 2: ./mq_sender
 *
 * msgrcv mtype 동작:
 *   mtype=0  → FIFO (큐에 도착한 순서대로)
 *   mtype=1  → type=1 인 메시지만
 *   mtype=-2 → type ≤ 2 중 가장 낮은 type 먼저 (type=1 우선)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define MQ_PROJ_ID      'M'
#define MSG_TEXT_SIZE   128
#define MSG_COUNT       6
#define RETRY_MAX       30      /* 100ms * 30 = 3초 */

typedef struct {
    long    mtype;
    char    mtext[MSG_TEXT_SIZE];
} t_msg;

int main(void)
{
    key_t   key;
    int     msqid;
    t_msg   msg;
    int     i;
    int     retry;
    ssize_t n;

    printf("=== Message Queue Receiver ===\n");
    printf("[Receiver] PID: %d\n\n", getpid());

    /* 1. Sender와 동일한 키로 큐에 접근 */
    key = ftok(".", MQ_PROJ_ID);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    printf("[Receiver] Queue key: 0x%x\n", (unsigned int)key);
    printf("[Receiver] Waiting for queue...\n");

    /* 2. Sender가 큐를 생성할 때까지 대기 */
    retry = 0;
    while (1) {
        /* IPC_CREAT 없이 접근: 없으면 ENOENT */
        msqid = msgget(key, 0666);
        if (msqid != -1)
            break;
        if (errno != ENOENT) {
            perror("msgget");
            exit(EXIT_FAILURE);
        }
        if (++retry > RETRY_MAX) {
            fprintf(stderr,
                "[Receiver] Timeout: sender not found.\n"
                "           Run ./mq_sender in another terminal.\n");
            exit(EXIT_FAILURE);
        }
        usleep(100000); /* 100ms */
    }

    printf("[Receiver] Queue found (msqid: %d)\n\n", msqid);

    /* 3. 메시지 수신: 긴급(type=1) 먼저, 그다음 일반(type=2)
     *
     *   Sender 전송 순서: type=2,2,1,2,1,2 (혼합)
     *   Receiver 수신 순서: type=1,1,2,2,2,2 (우선순위 순)
     *
     *   ※ mtype=-N (절대값 이하 중 최솟값 우선)은 Linux에서 동작하나
     *      macOS에서는 FIFO처럼 동작할 수 있어 명시적 mtype 지정 사용
     */
    printf("[Receiver] Phase 1: mtype=1 (긴급 작업) 먼저:\n");
    for (i = 0; i < 2; i++) {   /* type=1 메시지 2개 */
        n = msgrcv(msqid, &msg, sizeof(msg.mtext), 1, 0);
        if (n == -1) {
            if (errno == EIDRM) {
                printf("[Receiver] Queue removed by sender.\n");
                goto done;
            }
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        printf("[Receiver] Got  [type=%ld]: %s\n", msg.mtype, msg.mtext);
    }

    printf("\n[Receiver] Phase 2: mtype=2 (일반 작업):\n");
    for (i = 0; i < 4; i++) {   /* type=2 메시지 4개 */
        n = msgrcv(msqid, &msg, sizeof(msg.mtext), 2, 0);
        if (n == -1) {
            if (errno == EIDRM) {
                printf("[Receiver] Queue removed by sender.\n");
                goto done;
            }
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        printf("[Receiver] Got  [type=%ld]: %s\n", msg.mtype, msg.mtext);
    }

done:
    printf("\n[Receiver] All messages received. Exiting.\n");
    printf("[Receiver] (Queue 삭제는 Sender가 담당)\n");

    return 0;
}
