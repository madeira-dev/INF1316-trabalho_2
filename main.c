/*
    Membros do grupo:
        Gabriel Madeira 2111471
        Juliana Pinheiro 2110516
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>

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
    int io_time[5];     /* array de tempos de operacao io ja q pode ter mais de uma operacao io */
    int op_order[5];    /* array para indicar a ordem de execucao de cada operacao (exec ou io) */
    int process_start_time;
    int process_end_time;
    struct _queue_node *next;
} queue_node;

typedef struct _thread_args
{
    queue *ready_queue;
    queue *blocked_queue;
    int curr_op_index;
    int next_op_index;
} thread_args;

void init_queue(queue *q);
int is_queue_empty(queue *q);
void enqueue(queue *q, int process_number, int memory_size, int info_number, int exec_time[], int io_time[], int op_order[], int process_start_time, int process_end_time);
queue_node *dequeue(queue *q);
void print_queue(queue *q);
void free_queue(queue *q);
int count_process_number(queue *q);
void get_partition_size_first_fit(int memory_required, int partitions[]);
void get_partition_size_best_fit(int memory_required, int partitions[]);
void get_partition_size_worst_fit(int memory_required, int partitions[]);
int get_op_type(queue_node *current_process);
int get_op_index(queue_node *current_process, int op_type);
void *io_thread_func(void *arg);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("informe qual algoritmo deseja utilizar\n");
        printf("0->first fit\n1->best fit\n2->worst fit\n");
        return 1;
    }
    int argument = atoi(argv[1]);

    FILE *file_ptr;
    char c;
    int num_processes, process_number, memory_size, info_number;
    int exec_time[5] = {DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE}, io_time[5] = {DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE}, op_order[5] = {DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE}, i;
    int count_exec = 0, count_io = 0, read_info = 0, op_type /* indica qual operacao a ser executada */;
    int start_time = DEFAULT_VALUE, end_time = DEFAULT_VALUE;
    int partitions[3] = {0, 4, 0};
    queue *ready_queue = (queue *)malloc(sizeof(queue));   // fila de prontos
    queue *blocked_queue = (queue *)malloc(sizeof(queue)); // fila de processos bloqueados por operacao I/O
    queue *ended_queue = (queue *)malloc(sizeof(queue));   // fila para processos que ja terminaram sua execucao
    queue_node *current_process = (queue_node *)malloc(sizeof(queue_node));
    struct timeval current_time;

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
            enqueue(ready_queue, process_number, memory_size, info_number, exec_time, io_time, op_order, start_time, end_time);

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

    // first fit
    // dequeue no primeiro processo e verificar primeira operacao
    printf("\nmemoria total: %dKb\nparticoes iniciais: 4 de 4Kb\n", TOTAL_MEMORY);
    printf("time slice de execucao: %d segundos\n", TIME_SLICE);

    if (argument == 0) // first fit
        printf("\n<-Algoritmo First Fit->\n");
    else if (argument == 1) // best fit
        printf("\n<-Algoritmo Best Fit->\n");
    else if (argument == 2) // worst fit
        printf("\n<-Algoritmo Worst Fit->\n");
    else
    {
        printf("valor invalido para selecionar algoritmo\n0->first fit\n1->best fit\n2->worst fit\n");
        exit(1);
    }
    printf("\tparticoes: 2Kb:[%d] 4Kb:[%d] 8Kb:[%d]\n", partitions[0], partitions[1], partitions[2]);
    gettimeofday(&current_time, NULL);
    int program_start_time = current_time.tv_sec;
    printf("\ttempo de inicio do programa: %d\n", program_start_time);
    printf("\n");

    while (count_process_number(ended_queue) != num_processes)
    {
        // cobrindo caso de ainda existir processo bloqueado mesmo quando todos os outros ja terem terminado
        if (is_queue_empty(ready_queue))
        {
            while (is_queue_empty(ready_queue))
            {
                ;
            }
        }

        // loop para alocar as memorias ate terminar todos os processos
        current_process = dequeue(ready_queue); // pegando processo da fila de prontos
        gettimeofday(&current_time, NULL);
        printf("\tprocesso %d selecionado\n", current_process->process_number);

        if (current_process->process_start_time == DEFAULT_VALUE)
        {
            printf("\ttempo de inicio da execucao: %ld\n", (current_time.tv_sec));
            current_process->process_start_time = (current_time.tv_sec);
        }

        printf("\talocando memoria para o processo %d\n", current_process->process_number);
        printf("\tmemoria utilizada pelo processo %d: %dKb\n", current_process->process_number, current_process->memory_size);
        printf("\tbuscando particao para alocar %dKb...\n", current_process->memory_size);
        sleep(1);

        if (argument == 0) // first fit
            get_partition_size_first_fit(current_process->memory_size, partitions);
        else if (argument == 1) // best fit
            get_partition_size_best_fit(current_process->memory_size, partitions);
        else if (argument == 2) // worst fit
            get_partition_size_worst_fit(current_process->memory_size, partitions);

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
            {
                sleep(TIME_SLICE - current_process->exec_time[curr_op_index]);
            }

            // atualizar tempo restante de execucao
            current_process->exec_time[curr_op_index] -= TIME_SLICE;

            // verificar se acabou seu tempo de execucao total
            if (current_process->exec_time[curr_op_index] <= 0)
            {
                printf("\toperacao exec finalizada\n");

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

                    printf("\tproxima operacao=exec\n");
                    enqueue(ready_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->io_time, current_process->op_order, current_process->process_start_time, current_process->process_end_time);
                }
                else if (current_process->op_order[next_op_index] == 2) // io
                {

                    printf("\tproxima operacao=io\n");

                    printf("\tIO: enviando processo com operacao io para a fila de bloqueados\n");
                    printf("\tIO: processo na fila de bloqueados por %d segundos\n", current_process->io_time[curr_op_index]);
                    enqueue(blocked_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->io_time, current_process->op_order, current_process->process_start_time, current_process->process_end_time);

                    pthread_t thread_id;
                    thread_args args;
                    args.ready_queue = ready_queue;
                    args.blocked_queue = blocked_queue;
                    args.curr_op_index = curr_op_index;
                    args.next_op_index = next_op_index;

                    pthread_create(&thread_id, NULL, io_thread_func, (void *)&args);
                }

                else // terminou todas as operacoes ou operacao seguinte ja foi realizada
                {
                    // esse else cobre um caso muito especifico de pular uma operacao io:
                    // tirando o caso esperado de a operacao seguinte ja ter sido feita
                    // a variavel remaining_op eh usada junto do for e if seguintes para
                    // verificar o caso de a operacao seguinte estar feita porem a operacao
                    // depois dessa nao. Nesse caso o processo seria enviado para a fila de
                    // finalizados incorretamente e uma operacao ficaria de ser feita no fim
                    // do programa.

                    // esse caso ocorre com a sequencia de operacoes: io exec io.
                    int remaining_op = 0;
                    for (i = 0; i < 5; i++)
                    {
                        if (current_process->op_order[i] != -1)
                        {
                            remaining_op = 1;
                        }
                    }
                    if (remaining_op == 1)
                    {
                        printf("\tainda nao terminou todas as operacoes\n");
                        enqueue(ready_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->io_time, current_process->op_order, current_process->process_start_time, current_process->process_end_time);
                    }
                    else
                    {
                        printf("\tfinalizou todas as operacoes\n");
                        gettimeofday(&current_time, NULL);
                        printf("\ttempo de fim do processo %d: %ld\n", current_process->process_number, current_time.tv_sec);
                        current_process->process_end_time = current_time.tv_sec;
                        printf("\ttempo total de execucao: %d\n", (current_process->process_end_time - current_process->process_start_time));
                        enqueue(ended_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->io_time, current_process->op_order, current_process->process_start_time, current_process->process_end_time);
                    }
                }
            }

            // nao acabou tempo total de execucao
            else // mandando para o final da fila de prontos
            {
                printf("\ttempo da operacao exec nao terminou dentro do time slice\n\tfenviando para final da fila\n");
                enqueue(ready_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->io_time, current_process->op_order, current_process->process_start_time, current_process->process_end_time);
            }
        }
        else if (op_type == 2) // apenas para caso começar em io
        {
            int curr_op_index = get_op_index(current_process, op_type); // index da operacao a ser executada
            int next_op_index = curr_op_index + 1;
            printf("\toperacao io a ser realizada, enviando para a fila de bloqueados por %d segundos\n", current_process->io_time[get_op_index(current_process, op_type)]);

            printf("\tIO: processo na fila de bloqueados por %d segundos\n", current_process->io_time[curr_op_index]);
            enqueue(blocked_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->io_time, current_process->op_order, current_process->process_start_time, current_process->process_end_time);

            pthread_t thread_id;
            thread_args args;
            args.ready_queue = ready_queue;
            args.blocked_queue = blocked_queue;
            args.curr_op_index = curr_op_index;
            args.next_op_index = next_op_index;

            pthread_create(&thread_id, NULL, io_thread_func, (void *)&args);
        }
        else // caso de ja ter terminado todas as operacoes e o tipo ser -1
        {
            printf("\ttodas as instrucoes do processo %d finalizadas\n", current_process->process_number);
            gettimeofday(&current_time, NULL);
            printf("\ttempo de fim do processo %d: %ld\n", current_process->process_number, current_time.tv_sec);
            current_process->process_end_time = current_time.tv_sec;
            printf("\ttempo total de execucao: %d\n", (current_process->process_end_time - current_process->process_start_time));
            enqueue(ended_queue, current_process->process_number, current_process->memory_size, current_process->info_number, current_process->exec_time, current_process->io_time, current_process->op_order, current_process->process_start_time, current_process->process_end_time);
        }
        printf("\n");

        printf("printf das filas no final:\n");
        printf("prontos:\n");
        print_queue(ready_queue);
        printf("bloqueados:\n");
        print_queue(blocked_queue);
        printf("finalizados:\n");
        print_queue(ended_queue);

        printf("\n");
    }
    printf("execucao dos processos finalizada\n");
    free_queue(ready_queue);
    free_queue(blocked_queue);
    free_queue(ended_queue);
    free(current_process);

    return 0;
}

void init_queue(queue *q) { q->head = NULL, q->tail = NULL; }

int is_queue_empty(queue *q) { return (q->head == NULL); }

void enqueue(queue *q, int process_number, int memory_size, int info_number, int exec_time[], int io_time[], int op_order[], int process_start_time, int process_end_time)
{
    queue_node *new_node = (queue_node *)malloc(sizeof(queue_node));
    new_node->process_number = process_number;
    new_node->memory_size = memory_size;
    new_node->info_number = info_number;

    for (int i = 0; i < 5; i++)
        new_node->exec_time[i] = exec_time[i];

    for (int i = 0; i < 5; i++)
        new_node->io_time[i] = io_time[i];

    for (int i = 0; i < 5; i++)
        new_node->op_order[i] = op_order[i];

    new_node->process_start_time = process_start_time;
    new_node->process_end_time = process_end_time;

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
        printf("io=< ");
        for (int i = 0; i < 5; i++)
            printf("%d ", current->io_time[i]);
        printf("> ");
        printf("op order=< ");
        for (int i = 0; i < 5; i++)
            printf("%d ", current->op_order[i]);
        printf("> ");
        printf("start_time=<%d> ", current->process_start_time);
        printf("end_time=<%d> ", current->process_end_time);
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

void get_partition_size_first_fit(int memory_required, int partitions[])
{
    if (memory_required <= 2)
    {
        if (partitions[0] >= 1)
        {
            printf("\talocou 2kb de memoria usando 1 particao de 2kb\n");
        }
        else if (partitions[1] >= 1)
        {
            printf("\talocou 2kb de memoria usando 1 particao de 4kb\n");
        }
        else if (partitions[2] >= 1)
        {
            printf("\talocou 2kb de memoria usando 1 particao de 8kb\n");
        }
    }

    else if (memory_required <= 4)
    {
        if (partitions[1] >= 1)
        {
            printf("\talocou 4kb de memoria usando 1 particao de 4kb \n");
        }
        else if (partitions[2] >= 1)
        {
            printf("\talocou 4kb de memoria usando 1 particao de 8kb\n");
        }
        else // caso so tenha particao de dois
        {
            partitions[0] -= 2;
            partitions[1] += 1;
            printf("\talocou 4kb de memoria usando 2 particoes de 2kb\n");
        }
    }

    else if (memory_required <= 8)
    {
        if (partitions[2] >= 1)
        {
            printf("\talocou 8kb de memoria usando 1 particao de 8kb \n");
        }
        else if (partitions[1] >= 2)
        {
            partitions[1] -= 2;
            partitions[2] += 1;
            printf("\talocou 8kb de memoria usando 2 particoes de 4kb\n");
        }
        else
        {
            partitions[0] -= 4;
            partitions[2] += 1;
            printf("\talocou 8kb de memoria usando 4 particoes de 2kb\n");
        }
    }
    else
    {

        printf("\talocou 16kb de memoria, usou %d particoes de 2kb, %d particoes de 4kb e %d particoes de 8kb \n", partitions[0], partitions[1], partitions[2]);
        partitions[0] = 0;
        partitions[1] = 0;
        partitions[2] = 2;
    }
    printf("\tparticoes de 2: [%d]; particoes de 4:[%d]; particoes de 8:[%d]\n", partitions[0], partitions[1], partitions[2]);
}

void get_partition_size_best_fit(int memory_required, int partitions[])
{
    // processo precisa de no maximo 2Kb
    if (memory_required <= 2)
    {
        // verifico quantidade de particoes de tamanho 2
        if (partitions[0] >= 1)
        {
            // otimo joia sucesso
            printf("\talocou usou 2Kb ja tinha particao de 2kb\n");
        }
        else
        {
            if (partitions[1] >= 1)
            {
                // quebra 1 de 4 em 2 de 2
                partitions[1] -= 1;
                partitions[0] += 2;
                printf("\talocou usou 2Kb quebrou 4kb em 2 particoes de 2kb\n");
            }
            else
            {
                // quebra 1 de 8 em 2 de 2
                partitions[2] -= 1; // quebra 1 particao em 2 de 4
                partitions[1] += 1; // quebra 1 das particoes em 2 de 2
                partitions[0] += 2;
                printf("\talocou usou 2Kb quebrou 8kb em 2 particoes de 2kb\n");
            }
        }
    }

    // processo precisa de no maximo 4Kb
    else if (memory_required <= 4)
    {
        // verificar quantidade de particoes de tamanho 4
        if (partitions[1] >= 1)
        {
            printf("\talocou usou 4Kb ja tinha particao de 4kb\n");
        }
        else if (partitions[2] >= 1)
        {
            partitions[2] -= 1;
            partitions[1] += 2;
            printf("\talocou usou 4Kb quebrou 8kb em 2 particoes de 4kb\n");
        }
        else // juntar particoes de 2 em uma de 4
        {
            partitions[0] -= 2;
            partitions[1] += 1;
            printf("\talocou usou 4Kb juntou 2 particoes de 2kb em 1 de 4kb\n");
        }
    }

    // processo precisa de mais de 4Kb
    else if (memory_required <= 8)
    {
        // verificar quantidade de particoes de tamanho 8
        if (partitions[2] >= 1)
        {
            // otimo joia sucesso
            printf("\talocou usou 8Kb ja tinha particao de 8kb\n");
        }
        // verificar quantidade de particoes de tamanho 4
        else if (partitions[1] >= 2)
        {
            partitions[1] -= 2;
            partitions[2] += 1;
            printf("\talocou usou 8Kb juntou 2 particoes de 4kb em 1 de 8kb\n");
        }
        else
        {
            partitions[0] -= 4;
            partitions[2] += 1;
            printf("\talocou usou 8Kb juntou 4 particoes de 2kb em 1 de 8kb \n");
        }
    }
    else
    {
        printf("\talocou usou 16kb juntou %d particoes de 2kb, %d particoes de 4kb e %d particoes de 8kb para usar os 16kb\n", partitions[0], partitions[1], partitions[2]);
        partitions[0] = 0;
        partitions[1] = 0;
        partitions[2] = 2;
    }
    printf("\tparticoes de 2: [%d]; particoes de 4:[%d]; particoes de 8:[%d]\n", partitions[0], partitions[1], partitions[2]);
}

void get_partition_size_worst_fit(int memory_required, int partitions[])
{
    if (memory_required <= 8)
    {
        if (partitions[2] >= 1)
        {
            printf("\tusou 8kb ja tinha uma particao de 8kb\n");
        }
        else
        {
            int partition_2_in_use = partitions[0];
            int partition_4_in_use = partitions[1];

            if (partition_2_in_use * 2 >= 8)
            {
                partitions[0] -= 4; // diminui 8 Kb das particoes de 2
                partitions[2] += 1; // aumenta 8 Kb nas particoes de 8
                printf("\tusou 8kb juntou 4 particoes de 2kb\n");
            }
            else if (partition_4_in_use * 4 >= 8)
            {
                partitions[1] -= 2; // diminui 8 Kb das particoes de 4
                partitions[2] += 1; // aumenta 8 Kb nas particoes de 8
                printf("\tusou 8kb juntou 2 particoes de 4kb\n");
            }
        }
    }
    else
    {
        if (partitions[2] == 2)
        {
            printf("\tusou 16kb ja tinham 2 particoes de 8kb\n");
        }
        else
        {
            printf("\tusou 16kb juntou %d particoes de 2kb, %d particoes de 4kb e %d particoes de 8kb para usar 16kb\n", partitions[0], partitions[1], partitions[2]);
            partitions[0] = 0;
            partitions[1] = 0;
            partitions[2] = 2;
        }
    }
    printf("\tparticoes de 2: [%d]; particoes de 4:[%d]; particoes de 8:[%d]\n", partitions[0], partitions[1], partitions[2]);
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

void *io_thread_func(void *arg)
{
    thread_args *args = (thread_args *)arg;
    queue_node *tmp_node = (queue_node *)malloc(sizeof(queue_node));

    queue *ready_queue = args->ready_queue;
    queue *blocked_queue = args->blocked_queue;
    int curr_op_index = args->curr_op_index;
    int next_op_index = args->next_op_index;

    while (!is_queue_empty(blocked_queue))
    {
        tmp_node = dequeue(blocked_queue); // retira processo da fila de bloqueados
        sleep(tmp_node->io_time[curr_op_index]);

        printf("\tIO: operacao io do processo %d terminou, voltando com o processo para a fila de prontos\n", tmp_node->process_number);

        tmp_node->io_time[curr_op_index] = DEFAULT_VALUE; // voltando ao valor padrao para nao considerar a mesma operacao novamente
        if (tmp_node->op_order[curr_op_index] == 2)       // entao a operacao atual eh io (comeca em io ou operacoes io seguidas)
        {
            for (int i = 0; i < 5; i++) // tambem voltando ao valor padrao
            {
                if (tmp_node->op_order[i] == tmp_node->op_order[curr_op_index])
                {
                    tmp_node->op_order[i] = DEFAULT_VALUE;
                    break;
                }
            }
        }

        else // entao eh o caso da operacao seguinte ser io e a atual exec
        {
            for (int i = 0; i < 5; i++) // tambem voltando ao valor padrao
            {
                if (tmp_node->op_order[i] == tmp_node->op_order[next_op_index])
                {
                    tmp_node->op_order[i] = DEFAULT_VALUE;
                    break;
                }
            }
        }

        enqueue(ready_queue, tmp_node->process_number, tmp_node->memory_size, tmp_node->info_number, tmp_node->exec_time, tmp_node->io_time, tmp_node->op_order, tmp_node->process_start_time, tmp_node->process_end_time);
    }

    pthread_exit(NULL);
}
