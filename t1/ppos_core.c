#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"
#define STACKSIZE 64 * 1024 /* tamanho de pilha das threads */

task_t *main_task, *curr; int pCounter = 0;

void ppos_init() {
    /* disables the output buffer of the standard output (stdout), used by the printf function */
    setvbuf(stdout, 0, _IONBF, 0);
    task_t main;
    ucontext_t main_context;

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
    main.next = NULL;
    main.prev = NULL;
    pCounter++;
    main.id = pCounter;
    main.status = 1;
    main.context = main_context;
    /*update the current task to the main*/
    curr = &main;
    main_task = &main;

#ifdef DEBUG
    printf("ppos_init: sistema iniciado. Tarefa em execução: %d\n", curr->id);
#endif
}

int task_init(task_t *task, void (*start_func)(void *), void *arg) {
    char *stack;
    stack = malloc(STACKSIZE);
    getcontext(&(task->context));

    /*fill stack fields*/
    if (stack) {
        (task->context).uc_stack.ss_sp = stack;
        (task->context).uc_stack.ss_size = STACKSIZE;
        (task->context).uc_stack.ss_flags = 0;
        (task->context).uc_link = 0;
    } else {
        perror("Erro na criação da pilha: ");
        exit(1);
    }

    /*fill task fields*/
    pCounter++;
    task->next = NULL;
    task->prev = NULL;
    task->id = pCounter;
    task->status = 1;

    /*set the function that triggers when the context changes*/
    makecontext(&(task->context), (void *)start_func, 1, arg);
#ifdef DEBUG
    printf("task_init: iniciada tarefa %d\n", task->id);
#endif
    return task->id;
}

int task_id() {
    /*return the current task id*/
    return curr->id;
}

void task_exit(int exit_code) {
    /*set the curent task status to the exit code*/
    curr->status = exit_code;
    task_switch(main_task);
}

int task_switch(task_t *task) {
    task_t *aux = curr;
    curr = task;
#ifdef DEBUG
    printf("task_switch: trocou da tarefa %d para a tarefa %d\n", aux->id, task->id);
#endif
    swapcontext(&(aux->context), &(task->context));
    return 0;
}