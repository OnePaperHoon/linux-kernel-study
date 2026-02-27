/*
 * fifo_basic.c
 * 
 * FIFO (Named Pipe) 기본 예제
 * Writer와 Reader를 하나의 프로그램에 구현
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FIFO_PATH "/tmp/myfifo"

void writer_process(void)
{
    char *messages[] = {
        "Hello from writer!",
        "This is FIFO communication",
        "Named pipes are cool",
        NULL
    };
    
    printf("[Writer] Starting...\n");
    
    /* FIFO 열기 (쓰기 모드) */
    printf("[Writer] Opening FIFO: %s\n", FIFO_PATH);
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    printf("[Writer] FIFO opened (FD: %d)\n", fd);
    
    /* 메시지 전송 */
    for (int i = 0; messages[i] != NULL; i++) {
        printf("[Writer] Sending: \"%s\"\n", messages[i]);
        
        /* null terminator 포함해서 전송 (+1) */
        size_t len = strlen(messages[i]) + 1;
        ssize_t n = write(fd, messages[i], len);
        if (n == -1) {
            perror("write");
            break;
        }
        
        printf("[Writer] Sent %zd bytes (including null)\n", n);
        sleep(1);  // 천친히 전송
    }
    
    /* FIFO 닫기 */
    close(fd);
    printf("[Writer] Closed FIFO\n");
}

void reader_process(void)
{
    char buf[100];
    
    printf("[Reader] Starting...\n");
    
    /* FIFO 열기 (읽기 모드) */
    printf("[Reader] Opening FIFO: %s\n", FIFO_PATH);
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    printf("[Reader] FIFO opened (FD: %d)\n", fd);
    
    /* 메시지 수신 */
    printf("[Reader] Waiting for messages...\n");
    while (1) {
        /* 버퍼 초기화 */
        memset(buf, 0, sizeof(buf));
        
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        
        if (n == -1) {
            perror("read");
            break;
        }
        
        if (n == 0) {
            /* EOF */
            printf("[Reader] EOF received (writer closed)\n");
            break;
        }
        
        printf("[Reader] Received %zd bytes: \"%s\"\n", n, buf);
    }
    
    /* FIFO 닫기 */
    close(fd);
    printf("[Reader] Closed FIFO\n");
}

int main(void)
{
    pid_t pid;
    
    printf("=== FIFO Basic Example ===\n\n");
    
    /* 1. FIFO 생성 */
    printf("[Main] Creating FIFO: %s\n", FIFO_PATH);
    
    /* 기존 FIFO 삭제 (있으면) */
    unlink(FIFO_PATH);
    
    /* FIFO 생성 */
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    
    printf("[Main] FIFO created successfully\n");
    printf("[Main] Check: ls -l %s\n\n", FIFO_PATH);
    
    /* 2. Fork */
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        /* 자식: Reader */
        reader_process();
        exit(EXIT_SUCCESS);
        
    } else {
        /* 부모: Writer */
        sleep(1);  // Reader가 먼저 열리도록
        writer_process();
        
        /* 자식 종료 대기 */
        waitpid(pid, NULL, 0);
    }
    
    /* 3. FIFO 삭제 */
    printf("\n[Main] Removing FIFO: %s\n", FIFO_PATH);
    unlink(FIFO_PATH);
    
    printf("\n=== FIFO Communication Complete ===\n");
    
    return 0;
}