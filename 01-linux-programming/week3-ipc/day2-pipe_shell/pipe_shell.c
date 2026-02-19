/*
 * pipe_shell.c
 * 
 * Shell 파이프 구현: ls | grep txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int pipefd[2];
    pid_t pid1, pid2;
    
    printf("=== Implementing: ls | grep .c ===\n\n");
    
    /* Pipe 생성 */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    /* 첫 번째 자식: ls */
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid1 == 0) {
        /* ls 프로세스 */
        printf("[ls] Starting...\n");
        
        /* stdout을 pipe write end로 리다이렉트 */
        dup2(pipefd[1], STDOUT_FILENO);
        
        /* 사용 안하는 fd 닫기 */
        close(pipefd[0]);
        close(pipefd[1]);
        
        /* ls 실행 */
        execlp("ls", "ls", "-l", NULL);
        
        /* 여기 도달하면 에러 */
        perror("execlp ls");
        exit(EXIT_FAILURE);
    }
    
    /* 두 번째 자식: grep */
    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid2 == 0) {
        /* grep 프로세스 */
        printf("[grep] Starting...\n");
        
        /* stdin을 pipe read end로 리다이렉트 */
        dup2(pipefd[0], STDIN_FILENO);
        
        /* 사용 안하는 fd 닫기 */
        close(pipefd[0]);
        close(pipefd[1]);
        
        /* grep 실행 */
        execlp("grep", "grep", ".c", NULL);
        
        /* 여기 도달하면 에러 */
        perror("execlp grep");
        exit(EXIT_FAILURE);
    }
    
    /* 부모 프로세스 */
    /* 부모는 pipe 사용 안함 */
    close(pipefd[0]);
    close(pipefd[1]);
    
    /* 자식들 종료 대기 */
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    
    printf("\n=== Pipeline Complete ===\n");
    
    return 0;
}