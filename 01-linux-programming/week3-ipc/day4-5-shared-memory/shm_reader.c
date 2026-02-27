/*
 * shm_reader.c
 *
 * POSIX Shared Memory: 독립 Reader 프로세스
 * shm_writer보다 먼저 실행 필요
 *
 * 실행 순서:
 *   Terminal 1: ./shm_reader   ← 먼저 실행
 *   Terminal 2: ./shm_writer
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

#define SHM_NAME    "/day4_shm_ipc"
#define MSG_SIZE    128
#define RETRY_MAX   30      /* 100ms * 30 = 3초 대기 */

/* shm_writer와 동일한 구조체 (약속된 레이아웃) */
typedef struct {
    char            message[MSG_SIZE];
    volatile int    data_ready;
    volatile int    done;
} t_shm;

int main(void)
{
    int     shm_fd;
    t_shm   *shm;
    int     retry;

    printf("=== Shared Memory Reader ===\n");
    printf("[Reader] PID: %d\n\n", getpid());

    printf("[Reader] Waiting for shared memory '%s'...\n", SHM_NAME);

    /* 1. Writer가 공유 메모리를 생성할 때까지 재시도 */
    retry = 0;
    while (1) {
        /* O_CREAT 없이 열기: 존재하지 않으면 ENOENT */
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (shm_fd != -1)
            break;
        if (errno != ENOENT) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
        if (++retry > RETRY_MAX) {
            fprintf(stderr,
                "[Reader] Timeout: writer not found.\n"
                "         Run ./shm_writer in another terminal.\n");
            exit(EXIT_FAILURE);
        }
        usleep(100000); /* 100ms */
    }

    printf("[Reader] Connected to shared memory\n\n");

    /* 2. 매핑 */
    shm = mmap(NULL, sizeof(t_shm),
               PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    close(shm_fd);

    /* 3. 메시지 수신 */
    while (!shm->done || shm->data_ready) {
        if (!shm->data_ready) {
            usleep(1000);
            continue;
        }

        printf("[Reader] Received: \"%s\"\n", shm->message);
        shm->data_ready = 0;    /* 소비 완료 신호 */
    }

    printf("\n[Reader] All messages received. Exiting.\n");

    /* 4. 정리 (unlink는 writer가 담당) */
    munmap(shm, sizeof(t_shm));

    return 0;
}
