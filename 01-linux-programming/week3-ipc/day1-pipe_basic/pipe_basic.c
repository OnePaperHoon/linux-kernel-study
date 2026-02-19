/* 
    Pipe를 사용한 부모-자식 프로세스 간 통신
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(void)
{
    int pipefd[2]; // pipefd[0] : read end, pipefd[1] : write end
    pid_t cpid;
    char read_msg[100] = {0};

    printf("Parent process (PID: %d) is creating a pipe...\n", getpid());

    /* 1. pipe 생성 */
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    printf("Pipe created successfully. pipefd[0] = %d, pipefd[1] = %d\n", pipefd[0], pipefd[1]);

    /* 2. Fork */
    cpid = fork();
    if (cpid == -1) 
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) 
    { // Child process
        printf("Child process (PID: %d) is writing to the pipe...\n", getpid());

        /* Write end 닫기 (자식은 읽기만) */
        close(pipefd[1]); // Close unused read end
        
        /* Pipe에서 읽기 */
        printf("Child process is reading from the pipe...\n");
        ssize_t n = read(pipefd[0], read_msg, sizeof(read_msg));
        if (n == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        read_msg[n] = '\0'; // Null-terminate the string
        printf("Child process received message: '%s'\n", read_msg);

        /* Read end 닫기 */
        close(pipefd[0]);
        printf("Child process is exiting...\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        printf("parent process is writing to the pipe...\n");
        /* Read end 닫기*/
        close(pipefd[0]); // Close unused read end

        /* 자식이 준비 되도록 잠시 대기 */
        sleep(1);

        /* Pipe에 쓰기 */
        printf("Parent process is writing message to the pipe...\n");
        ssize_t n = write(pipefd[1], "Hello from parent!", strlen("Hello from parent!"));
        if (n == -1)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        printf("Parent process has written message to the pipe.\n");
        /* Write end 닫기 */
        close(pipefd[1]);
        /* 자식 프로세스가 종료될 때까지 대기 */
        int status;
        waitpid(cpid, &status, 0);

        if (WIFEXITED(status)) {
            printf("Child process exited with status %d\n", WEXITSTATUS(status));
            WEXITSTATUS(status);
        }
    }

    printf("Parent process is exiting...\n");
    return 0;
}
