/*
 * mq_sender.c
 *
 * System V Message Queue: 독립 Sender 프로세스
 * mq_receiver와 별도 터미널에서 실행
 *
 * 실행 순서:
 *   Terminal 1: ./mq_receiver   ← 먼저 실행
 *   Terminal 2: ./mq_sender
 *
 * Queue 키 공유 방법:
 *   ftok(".", 'M') → 경로 + ID로 동일한 key_t 생성
 *   두 프로세스가 같은 경로+ID를 사용하면 같은 큐에 접근
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MQ_PROJ_ID  'M'
#define MSG_TEXT_SIZE   128
#define MSG_COUNT   6

typedef struct {
    long    mtype;
    char    mtext[MSG_TEXT_SIZE];
} t_msg;

/* 전송 메시지: 타입 1(긴급) + 타입 2(일반) 혼합 */
static const struct {
    long        type;
    const char  *text;
} g_messages[MSG_COUNT] = {
    {2, "일반 작업 A"},
    {2, "일반 작업 B"},
    {1, "긴급 작업 X"},
    {2, "일반 작업 C"},
    {1, "긴급 작업 Y"},
    {2, "일반 작업 D"},
};

int main(void)
{
    key_t   key;
    int     msqid;
    t_msg   msg;
    int     i;

    printf("=== Message Queue Sender ===\n");
    printf("[Sender] PID: %d\n\n", getpid());

    /* 1. ftok으로 수신자와 공유할 키 생성 */
    key = ftok(".", MQ_PROJ_ID);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    printf("[Sender] Queue key: 0x%x\n", (unsigned int)key);

    /* 2. 메시지 큐 생성 (없으면 생성, 있으면 접근) */
    msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    printf("[Sender] Queue created (msqid: %d)\n", msqid);
    printf("[Sender] Receiver가 준비될 시간... (1s)\n\n");
    sleep(1);

    /* 3. 메시지 전송 */
    printf("[Sender] Sending %d messages:\n", MSG_COUNT);
    for (i = 0; i < MSG_COUNT; i++) {
        msg.mtype = g_messages[i].type;
        strncpy(msg.mtext, g_messages[i].text, MSG_TEXT_SIZE - 1);
        msg.mtext[MSG_TEXT_SIZE - 1] = '\0';

        if (msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
            perror("msgsnd");
            msgctl(msqid, IPC_RMID, NULL);
            exit(EXIT_FAILURE);
        }
        printf("[Sender] Sent  [type=%ld]: %s\n", msg.mtype, msg.mtext);
        sleep(1);
    }

    printf("\n[Sender] All messages sent.\n");
    printf("[Sender] Receiver 종료 대기 중... (4s)\n");
    sleep(4);

    /* 4. 메시지 큐 삭제 (Sender가 담당) */
    if (msgctl(msqid, IPC_RMID, NULL) == -1)
        perror("msgctl IPC_RMID");
    else
        printf("[Sender] Queue removed\n");

    return 0;
}
