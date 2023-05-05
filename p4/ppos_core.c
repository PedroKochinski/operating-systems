// GRR20206144 Pedro Henrique Kochinki Silva
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"
#include "queue.h"
#define STACKSIZE 64 * 1024 /* threads stach size */
#define ALPHA -1

task_t *main_task, *curr, *dispatcher, *task_queue;
ucontext_t main_context;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização do timer
struct itimerval timer;

/*define the status codes*/
typedef enum {
    READY = 3,
    RUNNING = 2,
    SUSPENDED = 1,
    TERMINATED = 0,
} status_t;

int pCounter = 0;

#ifdef DEBUG
void print_elem(void *ptr) {
    task_t *elem = ptr;

    if (!elem)
        return;

    elem->prev ? printf("%d", elem->prev->id) : printf("*");
    printf("<%d,%d>", elem->id, elem->dinamic_prio);
    elem->next ? printf("%d", elem->next->id) : printf("*");
}
#endif

void task_setprio(task_t *task, int prio) {
#ifdef DEBUG
    printf("\033[4;33m\033[3mtask_setprio:\n\033[0m");
#endif
    if (prio >= -20 && prio <= 20) {
        task->static_prio = prio;
        task->dinamic_prio = prio;
#ifdef DEBUG
        printf("\033[3;34m\033[3m    Setting task %d to priority %d\n\033[0m", task->id, prio);
        queue_print("\033[3;34m    Task queue before:\033[0m", (queue_t *)task_queue, print_elem);  // cast task_t to queue_t
#endif
    } else
        fprintf(stderr, "%s", "The task priority must belong to the range [-20, 20]\n");
}

int task_getprio(task_t *task) {
    return task == NULL ? curr->static_prio : task->static_prio;
}

task_t *scheduler() {

    task_t *aux = task_queue;
    task_t *next = task_queue;

    for (int i = 0; i < queue_size((queue_t *)task_queue); i++) {
        if (next->dinamic_prio > aux->dinamic_prio) next = aux; //select the highest
        if (aux->dinamic_prio + ALPHA >= -20 && aux != curr) aux->dinamic_prio += ALPHA; //increase priority
        aux = aux->next;
    }
    return next;
}

void task_delete(task_t *task) {
#ifdef DEBUG
    printf("\033[4;31m\033[3mtask_delete:\n\033[0m");
#endif
    /*if the task is running, switch to dispatcher before deleting*/
    if (task == curr) {
        task_switch(dispatcher);
    }
    /*remove task from queue*/
    queue_remove((queue_t **)&task_queue, (queue_t *)task);
#ifdef DEBUG
    printf("\033[3;34m    Deleting task %d\033[0m\n", task->id);
#endif
    /*free stack memory*/
    free(task->context.uc_stack.ss_sp);
    if (task == main_task) free(task);
}

void dispatcher_function() {
    task_t *next;
    /*while there are tasks in the queue*/
    while (queue_size((queue_t *)task_queue) > 0) {
        /*the scheduler select the next task*/
        next = scheduler();
#ifdef DEBUG
        printf("\033[4;36m\033[3mdispatcher_function:\n\033[0m");
        printf("\033[3;34m    Next task is %d. Current task executing: %d\033[0m\n", next->id, task_id());
        queue_print("\033[3;34m    Task queue before:\033[0m", (queue_t *)task_queue, print_elem);  // cast task_t to queue_t
#endif
        if (next != NULL) {
            /*switch tasks*/
            switch (next->status) {
                case TERMINATED:
                    task_delete(next);
                    next = scheduler();
                    break;
            }
            if (next != NULL) task_switch(next);
#ifdef DEBUG
            printf("\033[4;36m\033[3mdispatcher_function:\n\033[0m");
            queue_print("\033[3;34m    Task queue after:\033[0m", (queue_t *)task_queue, print_elem);  // cast task_t to queue_t
#endif
        }
    }
    /*finish dispatcher*/
    task_exit(0);
}

void ppos_init() {
#ifdef DEBUG
    printf("\033[4;32m\033[3mppos_init:\n\033[0m");
#endif
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
    main_task->status = RUNNING;
    main_task->static_prio = 0;
    main_task->dinamic_prio = 0;
    main_task->context = main_context;
    /*update the current task to the main*/
    curr = main_task;

    /*create dispatcher*/
    dispatcher = calloc(1, sizeof(task_t));
    if (!dispatcher) {
        perror("Erro na criação do dispatcher: ");
        exit(1);
    }
    task_init(dispatcher, dispatcher_function, NULL);
    /*append the main task to task_queue*/
    queue_append((queue_t **)&task_queue, (queue_t *)main_task);
#ifdef DEBUG
    printf("\033[3;34m    System initiated. Current task executing: %d\033[0m\n", task_id());
#endif
}

int task_init(task_t *task, void (*start_func)(void *), void *arg) {
#ifdef DEBUG
    printf("\033[4;32m\033[3mtask_init:\n\033[0m");
#endif
    ucontext_t context;
    char *stack;
    stack = malloc(STACKSIZE);
    getcontext(&context);

    /*fill stack fields*/
    if (stack) {
        context.uc_stack.ss_sp = stack;
        context.uc_stack.ss_size = STACKSIZE;
        context.uc_stack.ss_flags = 0;
        context.uc_link = 0;
    } else {
        perror("Error creating Stack: ");
        exit(1);
    }

    /*fill task fields*/
    pCounter += 1;
    task->next = NULL;
    task->prev = NULL;
    task->id = pCounter;
    task->status = SUSPENDED;
    task->static_prio = 0;
    task->dinamic_prio = 0;
    /*set the function that triggers when the context changes*/

    arg == NULL ? makecontext(&context, (void *)start_func, 0) : makecontext(&context, (void *)start_func, 1, arg);
    task->context = context;
    /*append the task to dispatcher*/
    if (task != dispatcher) queue_append((queue_t **)&task_queue, (queue_t *)task);
#ifdef DEBUG
    printf("\033[3;34m    Task initiated with id %d by task %d\033[0m\n", task->id, task_id());
#endif
    return task->id;
}

int task_id() {
    /*return the current task id*/
    return curr->id;
}

void task_exit(int exit_code) {
#ifdef DEBUG
    printf("\033[4;33m\033[3mtask_exit:\n\033[0m");
    printf("\033[3;34m    Finished task %d with exit code %d\033[0m\n", task_id(), exit_code);
#endif
    /*set the curent task status to the exit code and switch to main task*/
    curr->status = exit_code;
    curr->dinamic_prio = curr->static_prio;
    /*if the current task is the dispatcher the program finish*/
    if (curr == dispatcher) {
        free(dispatcher->context.uc_stack.ss_sp);
        free(dispatcher);
#ifdef DEBUG
        printf("\033[3;34m    Exitting dispatcher\033[0m\n");
#endif
        exit(0);
    }
    /*else, the control is given back to dispatcher*/
#ifdef DEBUG
    printf("\033[3;34m    Giving control back to dispatcher\033[0m\n");
#endif
    task_switch(dispatcher);
}

int task_switch(task_t *task) {
    /*change pointers and swap context*/
    task_t *aux = curr;
    // aux->status = SUSPENDED;
    curr = task;
    // curr->status = RUNNING;
#ifdef DEBUG
    printf("\033[4;36m\033[3mtask_switch:\n\033[0m");
    printf("\033[3;34m    Switched from task %d to task %d\033[0m\n", aux->id, task->id);
#endif
    swapcontext(&(aux->context), &(task->context));
    return 0;
}

void task_yield() {
#ifdef DEBUG
    printf("\033[4;35m\033[3mtask_yield:\n\033[0m");
    queue_print("\033[3;34m    Queue before:\033[0m", (queue_t *)task_queue, print_elem);
    printf("\033[3;34m    Task %d has yielded\033[0m\n", task_id());
    printf("\033[3;34m    Giving back control from task %d to dispatcher\033[0m\n", task_id());
#endif
    /*give control back to dispatcher and reseting priority*/
    curr->dinamic_prio = curr->static_prio;
    task_switch(dispatcher);
}