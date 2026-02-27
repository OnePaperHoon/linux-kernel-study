/*
 * shm_basic.c
 *
 * POSIX Shared Memory 기본 예제
 * fork()로 생성된 Writer(부모)와 Reader(자식) 간 통신
 *
 * 핵심 API:
 *   shm_open()   - 공유 메모리 객체 생성/열기
 *   ftruncate()  - 공유 메모리 크기 설정
 *   mmap()       - 프로세스 주소 공간에 매핑
 *   munmap()     - 매핑 해제
 *   shm_unlink() - 공유 메모리 객체 삭제
 *
 * Pipe와 차이점:
 *   Pipe  : 커널 버퍼를 통해 복사 (read/write)
 *   Shm   : 같은 물리 메모리를 직접 참조 (복사 없음)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SHM_NAME    "/shm_basic_example"
#define MSG_SIZE    128
#define MSG_COUNT   3

/* 공유 메모리 레이아웃 */
typedef struct {
    char            message[MSG_SIZE];
    volatile int    data_ready;   /* 1: 새 데이터 있음, 0: 소비됨 */
    volatile int    done;         /* 1: writer 완료 */
} t_shm;

void writer_process(t_shm *shm)
{
    const char  *messages[MSG_COUNT] = {
        "Hello from writer!",
        "Shared memory: no kernel copy",
        "IPC via mmap + shm_open",
    };
    int i;

    printf("[Writer] Starting (PID: %d)\n", getpid());

    for (i = 0; i < MSG_COUNT; i++) {
        /* Reader가 이전 메시지를 소비할 때까지 대기 */
        while (shm->data_ready)
            usleep(1000);

        /* 공유 메모리에 직접 쓰기 (복사 없음) */
        strncpy(shm->message, messages[i], MSG_SIZE - 1);
        shm->message[MSG_SIZE - 1] = '\0';
        shm->data_ready = 1;

        printf("[Writer] Sent [%d/%d]: \"%s\"\n", i + 1, MSG_COUNT, messages[i]);
        sleep(1);
    }

    /* 마지막 메시지 소비 대기 후 완료 신호 */
    while (shm->data_ready)
        usleep(1000);
    shm->done = 1;

    printf("[Writer] Done\n");
}

void reader_process(t_shm *shm)
{
    printf("[Reader] Starting (PID: %d)\n", getpid());

    /* done이 설정되고 미읽은 데이터도 없을 때까지 반복 */
    while (!shm->done || shm->data_ready) {
        if (!shm->data_ready) {
            usleep(1000);
            continue;
        }

        /* 공유 메모리에서 직접 읽기 */
        printf("[Reader] Received: \"%s\"\n", shm->message);
        shm->data_ready = 0;
    }

    printf("[Reader] Done\n");
}

int main(void)
{
    int     shm_fd;
    t_shm   *shm;
    pid_t   pid;

    printf("=== POSIX Shared Memory Basic Example ===\n\n");

    /* 1. 공유 메모리 객체 생성 */
    printf("[Main] Creating shared memory: %s\n", SHM_NAME);
    shm_unlink(SHM_NAME);   /* 이전 잔여 객체 제거 */

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    /* 2. 크기 설정 (생성 직후 0바이트이므로 필수) */
    if (ftruncate(shm_fd, (off_t)sizeof(t_shm)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    /* 3. 프로세스 주소 공간에 매핑
     *   MAP_SHARED: 모든 매핑이 같은 물리 페이지 공유
     *   MAP_PRIVATE: 프로세스마다 독립 복사본 (CoW)
     */
    shm = mmap(NULL, sizeof(t_shm),
               PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    /* mmap 이후 fd는 필요 없음 */
    close(shm_fd);

    /* 초기화 */
    memset(shm, 0, sizeof(t_shm));

    printf("[Main] Shared memory mapped (size: %zu bytes)\n", sizeof(t_shm));
    printf("[Main] Address: %p\n\n", (void *)shm);

    /* 4. Fork: 부모/자식이 같은 물리 페이지를 공유
     *   fflush: fork 전 stdout 버퍼 비우기
     *   (버퍼가 자식에게 복제되면 이미 출력된 내용이 중복 출력됨)
     */
    fflush(stdout);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        munmap(shm, sizeof(t_shm));
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* 자식: Reader */
        reader_process(shm);
        munmap(shm, sizeof(t_shm));
        exit(EXIT_SUCCESS);
    } else {
        /* 부모: Writer (Reader가 준비될 시간) */
        sleep(1);
        writer_process(shm);

        /* 자식 종료 대기 */
        waitpid(pid, NULL, 0);
    }

    /* 5. 정리 */
    printf("\n[Main] Cleaning up...\n");
    munmap(shm, sizeof(t_shm));
    shm_unlink(SHM_NAME);

    printf("\n=== Shared Memory Communication Complete ===\n");

    return 0;
}
