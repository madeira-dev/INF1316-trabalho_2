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
    int exec_time;      /* tempo de execucao */
    int pid;            /* id do processo */
    int io_time;        /* tempo de operacao io */
    struct _queue_node *next;
} queue_node;

void init_queue(queue *q);
int is_queue_empty(queue *q);
void enqueue(queue *q, int process_number, int memory_size, int info_number, int exec_time, int pid, int io_time);
queue_node *dequeue(queue *q);
void print_queue(queue *q);
void free_queue(queue *q);

int main(void)
{
    FILE *file_ptr;
    char c;
    int num_processes, process_number, memory_size, info_number, exec_time, pid, io_time;

    file_ptr = fopen("processos.txt", "r");
    while ((c = fgetc(file_ptr)) != EOF)
    {
        if (isdigit(c)) // essa gambiarrazinha é só pra ler o primeiro char
        {
            fseek(file_ptr, -1, SEEK_CUR);
            fscanf(file_ptr, "%d\n", &num_processes);
            break;
        }
    }
    // printf("quantidade de processos: %d\nProcesso #%d - %dKb\n", num_processes, process_number, memory_size);
    fclose(file_ptr);

    return 0;
}

void init_queue(queue *q) { q->head = NULL, q->tail = NULL; }

int is_queue_empty(queue *q) { return (q->head == NULL); }

void enqueue(queue *q, int process_number, int memory_size, int info_number, int exec_time, int pid, int io_time)
{
    queue_node *new_node = (queue_node *)malloc(sizeof(queue_node));
    new_node->process_number = process_number;
    new_node->memory_size = memory_size;
    new_node->info_number = info_number;
    new_node->exec_time = exec_time;
    new_node->pid = pid;
    new_node->io_time = io_time;
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
        printf("%d %d %d %d %d %d", current->process_number, current->memory_size, current->info_number, current->exec_time, current->pid, current->io_time);
        printf(" | ");
        current = current->next;
    }
    printf("\n");
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
