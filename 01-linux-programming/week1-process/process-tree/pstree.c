#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include "process.h"

#define MAX_PROCESSES 100000

typedef struct {
    process_t *processes[MAX_PROCESSES];
    int count;
} process_list_t;

// 모든 proc 읽기
process_list_t *read_all_processes(void)
{
    // list init
    process_list_t *list = calloc(1, sizeof(process_list_t));

    // /proc dir open
    DIR *dir = opendir("/proc");
    
    // valid check
    if (!dir)
    {
        perror("opendir");
        free(list);
        return NULL;
    }

    // cursor dirent
    struct dirent *entry; 

    // read dir
    while((entry = readdir(dir)) != NULL)
    {
        if (!isdigit(entry->d_name[0]))
            continue;

        pid_t pid = atoi(entry->d_name);
        // create pid process_t object
        process_t *proc = process_create(pid);

        // stat read
        if (process_read_stat(proc) == 0)
            list->processes[list->count++] = proc;
        else
            process_free(proc);
    }

    closedir(dir);
    return list;
}

process_t *find_process(process_list_t *list, pid_t pid)
{
    for (int i = 0; i < list->count; i++)
    {
        if (list->processes[i]->pid == pid)
            return list->processes[i];
    }
    return NULL;
}

void build_tree(process_list_t *list)
{
    for (int i = 0; i< list->count; i++)
    {
        process_t *proc = list->processes[i];

        if (proc->ppid == 0) // init process
            continue;

        process_t *parent = find_process(list, proc->ppid);
        if (parent) 
            process_add_child(parent, proc);
    }
}


// 트리 출력 (재귀)
void print_tree(process_t *proc, int depth, int is_last[])
{
    // 들여쓰기
    for (int i = 0; i < depth -1; i++)
    {
        if (is_last[i])
            printf("   ");
        else
            printf("│  ");
    }


    if (depth > 0)
    {
        if (is_last[depth - 1])
            printf("└─ ");
        else
            printf("├─ ");;
    }

    // 프로세스 정보 출력
    printf("%-6d %-6d %c   %s\n",
        proc->pid, proc->ppid, proc->state, proc->comm);

    // 자식 출력
    for (int i = 0; i < proc->num_children; i++)
    {
        is_last[depth] = (i == proc->num_children - 1);
        print_tree(proc->children[i], depth + 1, is_last);
    }
}

int is_in_tree(process_t *proc, process_t *root)
{
    process_t *curr = proc;

    while (curr)
    {
        if (curr == root)
            return 1;
        curr = curr->parent;
    }

    return 0;
}

void cleanup(process_list_t *list)
{
    for (int i = 0; i < list->count; i++)
    {
        free(list->processes[i]->children);
    }

    for (int i = 0; i < list->count; i++)
    {
        free(list->processes[i]);
    }

    free(list);
}

int main(void)
{
    // 1. 모든 프로세스 읽기
    process_list_t *list = read_all_processes();
    if (!list)
    {
        fprintf(stderr, "Failed to read processes\n");
        return 1;
    }

    printf("Foound %d processes\n\n", list->count);

    // 2. 트리 구축
    build_tree(list);

    // 3. 헤더 출력
    printf("PID    PPID   S   CMD\n");
    printf("----------------------------------\n");

    // 4. init 프로세스 (PID 1)부터 출력

    process_t *init = find_process(list, 1);
    if (init)
    {
        int is_last[100] = {};
        print_tree(init, 0, is_last);
    }

    // 5. 메모리 해제
    cleanup(list);
    return 0;
}