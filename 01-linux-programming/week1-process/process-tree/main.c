#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

// PID 디렉토리인지 확인
int is_pid_dir(const char *name)
{
    while (*name)
    {
        if (!isdigit(*name))
            return 0;
        name++;
    }
    return 1;
}

// /proc/[pid]/stat 파일 읽기
void read_stat(int pid)
{
    char path[256];
    char line[1024];

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        perror("fopen");
        return;
    }

    if (fgets(line, sizeof(line), fp)) {
        int read_pid, ppid;
        char comm[256], state;

        // 파싱: PID (COMM) STATE PPID ...
        sscanf(line, "%d %s %c %d", &read_pid, comm, &state, &ppid);

        printf("PID: %d, COMM: %s, STATE: %c, PPID: %d\n",
                read_pid, comm, state, ppid);
    }

    fclose(fp);
}

int main(void)
{
    DIR *dir = opendir("/proc");
    if (!dir)
    {
        perror("opendir");
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (is_pid_dir(entry->d_name))
        {
            int pid = atoi(entry->d_name);
            read_stat(pid);
        }
    }

    closedir(dir);
    return 0;
}