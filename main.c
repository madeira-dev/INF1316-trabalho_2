#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_PROGRAM_NAME_LEN 20

typedef struct _queue // fila de processos
{
    struct _queue_node *head;
    struct _queue_node *tail;
} queue;

typedef struct _queue_node // processos
{
    int process_number; /* numero do processo "Processo #<process_number>" */
    int memory_size;    /* quantidade de memoria necessaria para o processo */
    int info_number;    /* numero de informacoes de exec ou io */
    int exec_time[5];   /* array de tempos de execucao ja q pode ter mais um uma operacao de execucao */
    int pid;            /* id do processo */
    int io_time[5];     /* array de tempos de operacao io ja q pode ter mais de uma operacao io */
    struct _queue_node *next;
} queue_node;

void init_queue(queue *q);
int is_queue_empty(queue *q);
void enqueue(queue *q, int process_number, int memory_size, int info_number, int exec_time[], int pid, int io_time[]);
queue_node *dequeue(queue *q);
void print_queue(queue *q);
void free_queue(queue *q);
int count_process_number(queue *q);

int main(void)
{
    FILE *file_ptr;
    char c;
    int num_processes, process_number, memory_size, info_number;
    int exec_time[5] = {-1, -1, -1, -1, -1}, pid = -1, io_time[5] = {-1, -1, -1, -1, -1}, i;
    int count_exec = 0, count_io = 0, read_info = 0;
    queue *ready_queue = (queue *)malloc(sizeof(queue));
    queue *blocked_queue = (queue *)malloc(sizeof(queue));
    init_queue(ready_queue);
    init_queue(blocked_queue);

    file_ptr = fopen("processos.txt", "r");
    while ((c = fgetc(file_ptr)) != EOF)
    {
        if (c == 'P')
        {
            if (fscanf(file_ptr, "rocesso #%d - %dKb\n%d\n", &process_number, &memory_size, &info_number))
            {
                // lendo as informacoes do processo
                while (read_info != info_number)
                {
                    if (fscanf(file_ptr, "exec %d\n", &exec_time[count_exec])) // exec
                    {
                        read_info += 1;
                        count_exec += 1;
                    }
                    else if (fscanf(file_ptr, "io %d\n", &io_time[count_io])) // io
                    {
                        read_info += 1;
                        count_io += 1;
                    }
                }
                // reset values
                read_info = 0;
                count_exec = 0;
                count_io = 0;
            }
            // criando processo na fila de prontos
            enqueue(ready_queue, process_number, memory_size, info_number, exec_time, pid, io_time);

            // reset values
            for (i = 0; i < 5; i++)
            {
                io_time[i] = -1;
                exec_time[i] = -1;
            }
        }
    }
    print_queue(ready_queue);
    fclose(file_ptr);

    // salvando quantidade de processos
    num_processes = count_process_number(ready_queue);

    return 0;
}

void init_queue(queue *q) { q->head = NULL, q->tail = NULL; }

int is_queue_empty(queue *q) { return (q->head == NULL); }

void enqueue(queue *q, int process_number, int memory_size, int info_number, int exec_time[], int pid, int io_time[])
{
    queue_node *new_node = (queue_node *)malloc(sizeof(queue_node));
    new_node->process_number = process_number;
    new_node->memory_size = memory_size;
    new_node->info_number = info_number;

    for (int i = 0; i < 5; i++)
        new_node->exec_time[i] = exec_time[i];

    new_node->pid = pid;

    for (int i = 0; i < 5; i++)
        new_node->io_time[i] = io_time[i];

    new_node->next = NULL;

    if (is_queue_empty(q))
    {
        q->head = new_node;
        q->tail = new_node;
    }
    else
    {
        q->tail->next = new_node;
        q->tail = new_node;
    }
}

queue_node *dequeue(queue *q)
{
    if (is_queue_empty(q))
        return NULL;

    queue_node *removed_node = q->head;
    q->head = q->head->next;

    if (q->head == NULL)
        q->tail = NULL;

    return removed_node;
}

void print_queue(queue *q)
{
    if (is_queue_empty(q))
        return;

    queue_node *current = q->head;

    while (current != NULL)
    {
        printf("num=<%d> ", current->process_number);
        printf("mem=<%d> ", current->memory_size);
        printf("info=<%d> ", current->info_number);
        printf("exec=< ");
        for (int i = 0; i < 5; i++)
            printf("%d ", current->exec_time[i]);
        printf("> ");
        printf("pid=<%d> ", current->pid);
        printf("io=< ");
        for (int i = 0; i < 5; i++)
            printf("%d ", current->io_time[i]);
        printf("> ");
        printf("| ");
        current = current->next;
        printf("\n");
    }
}

void free_queue(queue *q)
{
    queue_node *current = q->head;
    queue_node *next;

    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
    q->head = NULL;
    q->tail = NULL;
}

int count_process_number(queue *q)
{
    queue_node *current = q->head;
    int count = 0;

    while (current != NULL)
    {
        count += 1;
        current = current->next;
    }
    return count;
}
