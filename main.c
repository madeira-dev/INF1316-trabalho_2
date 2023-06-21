#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_PROGRAM_NAME_LEN 20
#define TOTAL_MEMORY 16

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
    int op_order[5];    /* array para indicar a ordem de execucao de cada operacao (exec ou io)*/
    struct _queue_node *next;
} queue_node;

void init_queue(queue *q);
int is_queue_empty(queue *q);
void enqueue(queue *q, int process_number, int memory_size, int info_number, int exec_time[], int pid, int io_time[], int op_order[]);
queue_node *dequeue(queue *q);
void print_queue(queue *q);
void free_queue(queue *q);
int count_process_number(queue *q);
int check_partition_size(int memory_required);

int main(void)
{
    FILE *file_ptr;
    char c;
    int num_processes, process_number, memory_size, info_number;
    int exec_time[5] = {-1, -1, -1, -1, -1}, pid = -1, io_time[5] = {-1, -1, -1, -1, -1}, op_order[5] = {-1, -1, -1, -1, -1}, i;
    int count_exec = 0, count_io = 0, read_info = 0;
    int op_num = 0;
    queue *ready_queue = (queue *)malloc(sizeof(queue));   // fila de prontos
    queue *blocked_queue = (queue *)malloc(sizeof(queue)); // fila de processos bloqueados por operacao I/O
    queue *ended_queue = (queue *)malloc(sizeof(queue));   // fila para processos que ja terminaram sua execucao
    queue_node *current_process = (queue_node *)malloc(sizeof(queue_node));
    init_queue(ready_queue);
    init_queue(blocked_queue);
    init_queue(ended_queue);

    printf("lendo processos:\n");
    printf("\n");
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
                        op_order[read_info] = 1;
                        read_info += 1;
                        count_exec += 1;
                    }
                    else if (fscanf(file_ptr, "io %d\n", &io_time[count_io])) // io
                    {
                        op_order[read_info] = 2;
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
            enqueue(ready_queue, process_number, memory_size, info_number, exec_time, pid, io_time, op_order);

            // reset values
            for (i = 0; i < 5; i++)
            {
                io_time[i] = -1;
                exec_time[i] = -1;
                op_order[i] = -1;
            }
        }
        sleep(1);
    }
    fclose(file_ptr);

    // salvando quantidade de processos
    num_processes = count_process_number(ready_queue);
    printf("\n%d processos carregados\n", num_processes);
    sleep(1);
    printf("fila de prontos:\n");
    print_queue(ready_queue);
    sleep(1);
    // a partir daqui é o gerenciamento da memoria para os processos
    // implementacao dos algoritmos de First fit Best fit e Worst fit

    // first fit
    /* (por quanto tempo um processo fica bloqueado depois da operacao IO???? vou deixar por 4 segundos por enquanto) */

    // dequeue no primeiro processo e verificar primeira operacao
    printf("\nmemoria total: %dKb\nparticoes de 2Kb, 4Kb e 8Kb\n", TOTAL_MEMORY);
    printf("\n<-Algoritmo First Fit->\n");

    while (count_process_number(ended_queue) != num_processes)
    {
        // loop para alocar as memorias ate terminar todos os processos
        current_process = dequeue(ready_queue); // pegando processo da fila de prontos
        printf("\talocando memoria para o processo %d\n", current_process->process_number);
        printf("\tmemoria utilizada pelo processo %d: %dKb\n", current_process->process_number, current_process->memory_size);
        printf("\tbuscando particao para alocar %dKb...\n", current_process->memory_size);
        sleep(1);

        // buscando primeira particao que possua o tamanho necessario de memoria
        if (check_partition_size(current_process->memory_size) != -1)
            printf("\tparticao a ser utilizada para o processo: %dKb\n", check_partition_size(current_process->memory_size));
        else
        {
            printf("\tnenhuma particao encontrada\n");
            exit(1);
        }

        printf("verificando operacao a ser realizada pelo processo...\n");
        if (get_op_type(current_process) == 1)
        { // exec
          // alocar para o processo do index de get_op_index
        }
        else
        { // io
          // alocar para o processo do index de get_op_index
        }
    }

    /* executar dada operacao por no maximo 4 unidades de tempo, descontar esse
    tempo que passou do tempo total que a operacao precisa e mandar o processo
    para o final da fila */

    // dequeue no novo primeiro processo e seguir com o passo anterior para este processo

    return 0;
}

void init_queue(queue *q) { q->head = NULL, q->tail = NULL; }

int is_queue_empty(queue *q) { return (q->head == NULL); }

void enqueue(queue *q, int process_number, int memory_size, int info_number, int exec_time[], int pid, int io_time[], int op_order[])
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

    for (int i = 0; i < 5; i++)
        new_node->op_order[i] = op_order[i];

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
        printf("op order=< ");
        for (int i = 0; i < 5; i++)
            printf("%d ", current->op_order[i]);
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

int check_partition_size(int memory_required)
{
    int partitions[3] = {2, 4, 8}, i;

    for (i = 0; i < 3; i++)
        if (partitions[i] >= memory_required)
            return partitions[i];
    return -1;
}

int get_op_type(queue_node *current_process)
{
    int index;

    for (index = 0; index < 5; index++)
    {
        if (current_process->op_order[index] != -1)
            return current_process->op_order[index]; // retorna primeiro valor indicando o tipo de operacao
    }
}

int get_op_index(queue_node *current_process, int op_type)
{
    // verifica no array indicado de acordo com o tipo de operacao
    // e retorna o index deste valor em seu array
    int index;

    if (op_type == 1) // tipo de operacao = exec
    {
        for (index = 0; index < 5; index++)
        {
            if (current_process->exec_time[index] != -1) // buscando proxima operacao exec nao terminada
                return index;
        }
    }
    else // tipo de operacao = io
    {
        for (index = 0; index < 5; index++)
        {
            if (current_process->io_time[index] != -1) // buscando proxima operacao io nao terminada
                return index;
        }
    }
}
