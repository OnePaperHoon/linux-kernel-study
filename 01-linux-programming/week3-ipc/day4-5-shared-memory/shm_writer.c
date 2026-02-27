/*
 * shm_writer.c
 *
 * POSIX Shared Memory: 독립 Writer 프로세스
 * shm_reader와 별도 터미널에서 실행하여 프로세스 간 통신 확인
 *
 * 실행 순서:
 *   Terminal 1: ./shm_reader   ← 먼저 실행 (shared memory 대기)
 *   Terminal 2: ./shm_writer
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SHM_NAME    "/day4_shm_ipc"
#define MSG_SIZE    128
#define MSG_COUNT   5

/* shm_reader와 동일한 구조체 (약속된 레이아웃) */
typedef struct {
    char            message[MSG_SIZE];
    volatile int    data_ready;   /* 1: 새 데이터 있음, 0: 소비됨 */
    volatile int    done;         /* 1: writer 완료 */
} t_shm;

int main(void)
{
    int         shm_fd;
    t_shm       *shm;
    int         i;

    const char  *messages[MSG_COUNT] = {
        "Message 1: Hello, Reader!",
        "Message 2: Shared memory is fast",
        "Message 3: No kernel copy overhead",
        "Message 4: Direct memory access",
        "Message 5: IPC complete",
    };

    printf("=== Shared Memory Writer ===\n");
    printf("[Writer] PID: %d\n\n", getpid());

    /* 1. 공유 메모리 생성 */
    shm_unlink(SHM_NAME);   /* 이전 잔여 객체 제거 */

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, (off_t)sizeof(t_shm)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    shm = mmap(NULL, sizeof(t_shm),
               PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    close(shm_fd);

    memset(shm, 0, sizeof(t_shm));

    printf("[Writer] Shared memory '%s' created\n", SHM_NAME);
    printf("[Writer] Waiting for reader to connect... (2s)\n\n");
    sleep(2);   /* Reader 연결 대기 */

    /* 2. 메시지 전송 */
    for (i = 0; i < MSG_COUNT; i++) {
        /* Reader가 이전 메시지를 소비할 때까지 대기 */
        while (shm->data_ready)
            usleep(1000);

        strncpy(shm->message, messages[i], MSG_SIZE - 1);
        shm->message[MSG_SIZE - 1] = '\0';
        shm->data_ready = 1;

        printf("[Writer] Sent [%d/%d]: \"%s\"\n", i + 1, MSG_COUNT, messages[i]);
        sleep(1);
    }

    /* 3. 마지막 메시지 소비 대기 후 완료 신호 */
    while (shm->data_ready)
        usleep(1000);
    shm->done = 1;

    printf("\n[Writer] All messages sent. Reader will exit.\n");

    /* Reader가 done 플래그를 확인하고 정리할 시간 */
    sleep(1);

    /* 4. 공유 메모리 정리 */
    munmap(shm, sizeof(t_shm));
    shm_unlink(SHM_NAME);

    printf("[Writer] Shared memory removed\n");

    return 0;
}
