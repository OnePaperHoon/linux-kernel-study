#include "process.h"

process_t *process_create(pid_t pid)
{
    process_t *proc = calloc(1, sizeof(process_t));
    if (!proc)
        return NULL;

    proc->pid = pid;
    proc->children_capacity = 10;
    proc->children = calloc(10, sizeof(process_t *));

    return proc;
}

void process_free(process_t *proc)
{
    if (!proc)
        return;

    for (int i = 0; i < proc->num_children; i++)
        process_free(proc->children[i]);

    free(proc->children);
    free(proc);
}

int process_read_stat(process_t *proc)
{
    char path[256];
    char line[1024];

    snprintf(path, sizeof(path), "/proc/%d/stat", proc->pid);

    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;

    if (!fgets(line, sizeof(line), fp))
    {
        fclose(fp);
        return -1;
    }

    // 파싱 ex) 1 (systemd) S 0 
    //         *로 스킵 comm state ppid
    sscanf(line, "%*d %s %c %d", proc->comm, &proc->state, &proc->ppid);

    // comm (괄호 제거)
    size_t len = strlen(proc->comm);
    if (len > 2 && proc->comm[0] == '(' && proc->comm[len - 1] == ')')
    {
        proc->comm[len-1] = '\0';
        memmove(proc->comm, proc->comm + 1, len - 1);
    }

    fclose(fp);
    return 0;
}

void process_add_child(process_t *parent, process_t *child)
{
    if (parent->num_children >= parent->children_capacity) 
    {
        parent->children_capacity *= 2;
        parent->children = realloc(parent->children,
            parent->children_capacity * sizeof(process_t *));
    }

    parent->children[parent->num_children++] = child;
    child->parent = parent;
}