#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"
#include "queue.h"
#define STACKSIZE 64 * 1024 /* tamanho de pilha das threads */

task_t *main_task, *curr, *dispatcher;
ucontext_t main_context;

/*define the status codes*/
typedef enum {
    TERMINATED = 3,
    RUNNING = 2,
    SUSPENDED = 1,
    READY = 0,
} status_t;

int pCounter = 0;

void dispatcher_function();

void ppos_init() {
    /* disables the output buffer of the standard output (stdout), used by the printf function */
    setvbuf(stdout, 0, _IONBF, 0);
    main_task = calloc(1, sizeof(task_t));

    /*save the main context*/
    char *stack;
    stack = malloc(STACKSIZE);
    getcontext(&main_context);

    /*fill stack fields*/
    if (stack) {
        main_context.uc_stack.ss_sp = stack;
        main_context.uc_stack.ss_size = STACKSIZE;
        main_context.uc_stack.ss_flags = 0;
        main_context.uc_link = 0;
    } else {
        perror("Erro na criação da pilha: ");
        exit(1);
    }

    /*fill task fields*/
    main_task->next = NULL;
    main_task->prev = NULL;
    main_task->id = pCounter;
    main_task->status = 1;
    main_task->context = main_context;
    /*update the current task to the main*/
    curr = main_task;

    // cria o despatcher usando task_init
    task_init(dispatcher, dispatcher_function, NULL);

    /*append the main task to dispatcher*/
    /*cast task_t to queue_t*/
    queue_append((queue_t **)&dispatcher, (queue_t *)main_task);
#ifdef DEBUG
    printf("\033[4;32m\033[3mppos_init:\033[0m \033[3;34mSystem initiated. Current task executing: %d\033[0m\n", curr->id);
#endif
}

int task_init(task_t *task, void (*start_func)(void *), void *arg) {
    char *stack;
    stack = malloc(STACKSIZE);
    task = calloc(1, sizeof(task_t));
    getcontext(&(task->context));

    /*fill stack fields*/
    if (stack) {
        (task->context).uc_stack.ss_sp = stack;
        (task->context).uc_stack.ss_size = STACKSIZE;
        (task->context).uc_stack.ss_flags = 0;
        (task->context).uc_link = 0;
    } else {
        perror("Error creating Stack: ");
        exit(1);
    }

    /*fill task fields*/
    pCounter += 1;
    task->next = NULL;
    task->prev = NULL;
    task->id = pCounter;
    task->status = 1;

    /*set the function that triggers when the context changes*/
    arg == NULL ? makecontext(&(task->context), (void *)start_func, 0) : makecontext(&(task->context), (void *)start_func, 1, arg);
    /*append the task to dispatcher*/
    /*cast task_t to queue_t*/
    queue_append((queue_t **)&dispatcher, (queue_t *)task);
#ifdef DEBUG
    printf("\033[4;32m\033[3mtask_init:\033[0m \033[3;34mTask initiated with id %d by task %d\033[0m\n", task->id, curr->id);
#endif
    return task->id;
}

int task_id() {
    /*return the current task id*/
    return curr->id;
}

void task_exit(int exit_code) {
#ifdef DEBUG
    printf("\033[4;33m\033[3mtask_exit:\033[0m \033[3;34mFinished task %d with exit code %d\033[0m\n", curr->id, exit_code);
#endif
    /*set the curent task status to the exit code and switch to main task*/
    curr->status = exit_code;
    /*if the current task is the dispatcher the program finish*/
    if (curr == dispatcher) {
        free(dispatcher->context.uc_stack.ss_sp);
        free(dispatcher);
#ifdef DEBUG
        printf("\033[4;33m\033[3mtask_exit:\033[0m \033[3;34mExitting dispatcher\033[0m\n");
#endif
        exit(0);
    }
    /*else, the control is given back to dispatcher*/
    task_switch(dispatcher);
}

int task_switch(task_t *task) {
    /*change pointers and swap context*/
    task_t *aux = curr;
    curr = task;
#ifdef DEBUG
    printf("\033[4;36m\033[3mtask_switch:\033[0m \033[3;34mSwitched from task %d to task %d\033[0m\n", aux->id, task->id);
#endif
    swapcontext(&(aux->context), &(task->context));
    return 0;
}

void task_yield() {
    /*removes que current task from the dispatcher list and inserts again in the end*/
    /*cast task_t to queue_t*/
    queue_remove((queue_t **)&dispatcher, (queue_t *)curr);
    queue_append((queue_t **)&dispatcher, (queue_t *)curr);
#ifdef DEBUG
    printf("\033[4;35m\033[3mtask_yield:\033[0m \033[3;34mTask %d has been moved to the end of the queue\033[0m\n", curr->id);
    printf("\033[4;35m\033[3mtask_yield:\033[0m \033[3;34mGiving back control from task %d to dispatcher\033[0m\n", curr->id);
#endif
    /*give control back to dispatcher*/
    task_switch(dispatcher);
}

task_t *scheduler() {
    return dispatcher->next;
}

void task_delete(task_t *task) {
    /*if the task is running, switch to dispatcher before deleting*/
    if (task == curr) {
        task_switch(dispatcher);
    }
    /*remove task from queue*/
    queue_remove((queue_t **)&dispatcher, (queue_t *)task);
    /*free stack memory*/
    free(task->context.uc_stack.ss_sp);
    free(task);
    /*set task status to TERMINATED*/
    task->status = TERMINATED;
#ifdef DEBUG
    printf("\033[4;31m\033[3mtask_delete:\033[0m \033[3;34mTask %d deleted\033[0m\n", task->id);
#endif
}
void dispatcher_function() {
    task_t *next;                                    // variável para guardar a próxima tarefa a executar
    while (queue_size((queue_t *)dispatcher) > 1) {  // enquanto houverem tarefas de usuário
        next = scheduler();                          // escolhe a próxima tarefa a executar
        if (next != NULL) {                          // escalonador escolheu uma tarefa?
            task_switch(next);                       // transfere controle para a próxima tarefa
            // voltando ao dispatcher, trata a tarefa de acordo com seu estado
            switch (next->status) {
                case READY:
                    task_delete(next);
                    break;
            }
        }
    }
    task_exit(0);  // encerra a tarefa dispatcher
}
