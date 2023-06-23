#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_PROGRAM_NAME_LEN 20
#define TOTAL_MEMORY 16
#define TIME_SLICE 4
#define DEFAULT_VALUE -1

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
    int op_order[5];    /* array para indicar a ordem de execucao de cada operacao (exec ou io) */
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
int get_op_type(queue_node *current_process);
int get_op_index(queue_node *current_process, int op_type);

int main(void)
{
    FILE *file_ptr;
    char c;
    int num_processes, process_number, memory_size, info_number;
    int exec_time[5] = {DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE}, pid = DEFAULT_VALUE, io_time[5] = {DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE}, op_order[5] = {DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE}, i;
    int count_exec = 0, count_io = 0, read_info = 0, op_type, parent_pid = getpid();
    queue *ready_queue = (queue *)malloc(sizeof(queue));   // fila de prontos
    queue *blocked_queue = (queue *)malloc(sizeof(queue)); // fila de processos bloqueados por operacao I/O
    queue *ended_queue = (queue *)malloc(sizeof(queue));   // fila para processos que ja terminaram sua execucao
    queue_node *current_process = (queue_node *)malloc(sizeof(queue_node));

    init_queue(ready_queue);
    init_queue(blocked_queue);
    init_queue(ended_queue);

    printf("lendo processos:\n");
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
                // resetando valores
                read_info = 0;
                count_exec = 0;
                count_io = 0;
            }
            // criando processo na fila de prontos
            enqueue(ready_queue, process_number, memory_size, info_number, exec_time, pid, io_time, op_order);

            // resetando valores
            for (i = 0; i < 5; i++)
            {
                io_time[i] = DEFAULT_VALUE;
                exec_time[i] = DEFAULT_VALUE;
                op_order[i] = DEFAULT_VALUE;
            }
        }
    }
    fclose(file_ptr);
    sleep(1);

    // salvando quantidade de processos
    num_processes = count_process_number(ready_queue);
    printf("\nprocessos carregados: %d\n", num_processes);
    sleep(1);
    printf("fila de prontos:\n");
    print_queue(ready_queue);
    sleep(1);
    // a partir daqui é o gerenciamento da memoria para os processos
    // implementacao dos algoritmos de First fit Best fit e Worst fit

    // first fit
    // dequeue no primeiro processo e verificar primeira operacao
    printf("\nmemoria total: %dKb\nparticoes de 2Kb, 4Kb e 8Kb\n", TOTAL_MEMORY);
    printf("time slice de execucao: %d segundos\n", TIME_SLICE);
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
        else // acho que dentro desse else na verdade precisa ter os casos de dividir ou dobrar as particoes
        {
            printf("\tnenhuma particao encontrada\n");
            exit(1);
        }

        op_type = get_op_type(current_process); // pegando valor que indica operacao a ser realizada
        printf("\tverificando operacao a ser realizada pelo processo...\n");
        if (op_type == 1) // exec
        {
            int curr_op_index = get_op_index(current_process, op_type); // index da operacao a ser executada

            // alocar para o processo curr_op_index
            printf("\trealizando operacao exec com duracao total de %d segundos\n", current_process->exec_time[curr_op_index]);
            if (current_process->exec_time[curr_op_index] >= TIME_SLICE)
            {
                sleep(TIME_SLICE);
                printf("\t%d segundos restantes\n", (current_process->exec_time[curr_op_index] - TIME_SLICE));
            }
            else
                sleep(TIME_SLICE - current_process->exec_time[curr_op_index]);

            // atualizar tempo restante de execucao
            current_process->exec_time[curr_op_index] -= TIME_SLICE;

            // verificar se acabou seu tempo de execucao total
            if (current_process->exec_time[curr_op_index] <= 0)
            {
                printf("\tDEBUG acabou de executar tudo\n");
                // como operacao terminou, voltando para valor padrao
                current_process->exec_time[curr_op_index] = DEFAULT_VALUE;

                for (i = 0; i < 5; i++)
                {
                    if (current_process->op_order[i] == op_type)
                    {
                        current_process->op_order[i] = DEFAULT_VALUE;
                        break;
                    }
                }

                int next_op_index = curr_op_index + 1;

                // verifica proxima operacao
                if (current_process->op_order[next_op_index] == 1) // exec
                {
                    printf("\tDEBUG proxima operacao=exec\n");
                    enqueue(ready_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->pid, current_process->io_time, current_process->op_order);
                }
                else if (current_process->op_order[next_op_index] == 2) // io
                {
                    printf("\tDEBUG proxima operacao=io\n");
                    current_process->io_time[curr_op_index] = DEFAULT_VALUE; // voltando ao valor padrao para nao considerar a mesma operacao novamente
                    for (i = 0; i < 5; i++)                                  // tambem voltando ao valor padrao
                    {
                        if (current_process->op_order[i] == 2)
                        {
                            current_process->op_order[i] = DEFAULT_VALUE;
                            break;
                        }
                    }

                    // pid = fork();
                    // if (pid == 0) // filho
                    // {
                    // queue_node *tmp_node = (queue_node *)malloc(sizeof(queue_node));
                    // printf("\tIO: enviando operacao io para fila de bloqueados\n");
                    // printf("\tIO: processo na fila de bloqueados por %d segundos\n", current_process->io_time[curr_op_index]);
                    // enqueue(blocked_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->pid, current_process->io_time, current_process->op_order);

                    // sleep(current_process->io_time[curr_op_index]);

                    // tmp_node = dequeue(blocked_queue);

                    // printf("\tIO: operacao io terminou, voltando com o processo para a fila de prontos\n");
                    // current_process->io_time[curr_op_index] = DEFAULT_VALUE; // voltando ao valor padrao para nao considerar a mesma operacao novamente
                    // for (i = 0; i < 5; i++)                                  // tambem voltando ao valor padrao
                    // {
                    //     if (current_process->op_order[i] == 2)
                    //     {
                    //         current_process->op_order[i] = DEFAULT_VALUE;
                    //         break;
                    //     }
                    // }

                    enqueue(ready_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->pid, current_process->io_time, current_process->op_order);

                    printf("prontos:\n");
                    print_queue(ready_queue);
                    printf("bloqueados\n");
                    print_queue(blocked_queue);

                    // exit(0);
                    // }
                }
                else // terminou todas as operacoes
                {
                    printf("\tDEBUG proxima operacao=nenhuma\n");
                    enqueue(ended_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->pid, current_process->io_time, current_process->op_order);
                }
            }

            // nao acabou tempo total de execucao
            else // mandando para o final da fila de prontos
            {
                printf("\tDEBUG nao acabou de executar tudo\n");
                enqueue(ready_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->pid, current_process->io_time, current_process->op_order);
            }
        }
        // else if (op_type == 2) // io
        // {
        //     // mandar processo para a fila de bloqueados pelo tempo do io
        //     // aqui tambem devo precisar usar fork para deixar em paralelo
        //     printf("\toperacao io a ser realizada, enviando para a fila de bloqueados por %d segundos", current_process->io_time[get_op_index(current_process, op_type)]);
        // }
        else
            puts("tipo de operacao invalido");
        printf("\n");
    }
    printf("execucao dos processos finalizadas\n");
    free_queue(ready_queue);
    free_queue(blocked_queue);
    free_queue(ended_queue);
    free(current_process);

    /* executar o primeiro processo da fila de prontos, apos terminar toda sua execucao, verificar
    se proxima operacao é io, se for io colocar na fila de bloqueados pelo tempo indicado
    no arquivo e ir para o proximo processo, se nao for io apenas passar para o proximo processo
    */

    /* depois de executar P1, vai pro proximo processo se tiver algum na fila de pronto */
    /* pode repetir a particao depois de usar (o que acontece com oq resta??) */
    /* o processo fica na fila de bloqueados pelo tempo que o IO precisar, nao considera o time slice de 4 */
    /* se dois processos voltarem juntos para a fila de prontos, o que voltou da fila de bloqueados tem prioridade */

    /* depois de terminar o exec de um programa, verificar se operacao seguinte é de io, se for, manda pra
    fila de bloqueados pelo tempo de io indicado no arquivo, se nao, vai pro final da fila de prontos */

    /* adicionar campo na struct de processo indicando a operacao anterior para adicionar prioridade
    para aqueles que estiverem voltando de io */

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
        if (current_process->op_order[index] != DEFAULT_VALUE)
            return current_process->op_order[index]; // retorna primeiro valor indicando o tipo de operacao
    }
    return DEFAULT_VALUE;
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
            if (current_process->exec_time[index] != DEFAULT_VALUE) // buscando proxima operacao exec nao terminada
                return index;
        }
    }
    else // tipo de operacao = io
    {
        for (index = 0; index < 5; index++)
        {
            if (current_process->io_time[index] != DEFAULT_VALUE) // buscando proxima operacao io nao terminada
                return index;
        }
    }
    return DEFAULT_VALUE;
}
