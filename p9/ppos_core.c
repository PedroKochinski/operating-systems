// GRR20206144 Pedro Henrique Kochinki Silva
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ppos.h"
#include "queue.h"
#define STACKSIZE 64 * 1024 /* threads stach size */
#define ALPHA -1
#define QUANTUM 20
task_t *main_task, *curr, *dispatcher, *task_queue, *suspended_queue, *next;
ucontext_t main_context;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;

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
unsigned int tick_counter = 0;

#ifdef DEBUG
void print_elem(void *ptr) {
    task_t *elem = ptr;

    if (!elem)
        return;

    elem->prev ? printf("%d", elem->prev->id) : printf("*");
    printf("<%d,%d>", elem->id, elem->dynamic_prio);
    elem->next ? printf("%d", elem->next->id) : printf("*");
}
#endif

void task_suspend(task_t **queue) {
#ifdef DEBUG
    printf("\033[4;33m\033[3mtask_suspend:\n\033[0m");
#endif
    if (queue_remove((queue_t **)&task_queue, (queue_t *)curr) == 0) {
#ifdef DEBUG
        printf("\033[3;34m    Task %d status is now in suspended queue\033[0m\n", curr->id);
#endif
        if (curr != NULL) {
            queue_append((queue_t **)&suspended_queue, (queue_t *)curr);
            curr->status = SUSPENDED;
            task_yield();
#ifdef DEBUG
            printf("\033[3;34m    Task %d status is now SUSPENDED\033[0m\n", curr->id);
#endif
        } else
            fprintf(stderr, "%s", "The task is NULL\n");
    }
}

void task_resume(task_t *task, task_t **queue) {
#ifdef DEBUG
    printf("\033[4;33m\033[3mtask_resume:\n\033[0m");
#endif
    if (queue) {
        if (queue_remove((queue_t **)&suspended_queue, (queue_t *)task) == 0) {
            queue_append((queue_t **)queue, (queue_t *)task);
            task->status = READY;
            task->wake_up_time = -1;
            task->waiting_id = -1;
#ifdef DEBUG
            printf("\033[3;34m    Task %d status is now in task queue\033[0m\n", task->id);
#endif
#ifdef DEBUG
            printf("\033[3;34m    Task %d status is now READY\033[0m\n", task->id);
#endif
        }
    } else
        fprintf(stderr, "%s", "The queue is NULL\n");
}

void check_suspended_queue() {
    if (suspended_queue != NULL) {
        task_t *aux = suspended_queue->next;
        for (int i = 0; i < queue_size((queue_t *)suspended_queue); i++) {
            if (aux->wake_up_time <= systime() && aux->wake_up_time != -1) {
                task_resume(aux, &task_queue);
            }
            aux = aux->next;
        }
    }
}

void task_sleep(int t) {
    if (t > 0) {
        curr->wake_up_time = systime() + t;
        task_suspend(&suspended_queue);
    }
}

int task_wait(task_t *task) {
#ifdef DEBUG
    printf("\033[4;33m\033[3mtask_wait:\n\033[0m");
#endif
    if (task) {
        if (task && task->status != TERMINATED) {
            curr->waiting_id = task->id;
            task_suspend(&suspended_queue);
        }
    } else {
        return -1;
    }
    return task->exit_code;
}

void task_setprio(task_t *task, int prio) {
#ifdef DEBUG
    printf("\033[4;33m\033[3mtask_setprio:\n\033[0m");
#endif
    if (prio >= -20 && prio <= 20) {
        task->static_prio = prio;
        task->dynamic_prio = prio;
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
#ifdef DEBUG
    printf("\033[4;31m\033[3mscheduler:\n\033[0m");
#endif
    task_t *aux = task_queue;
    task_t *next = task_queue;
    for (int i = 0; i < queue_size((queue_t *)task_queue); i++) {
        if (aux->dynamic_prio + ALPHA > -20 && aux != curr) aux->dynamic_prio += ALPHA;  // increase priority
        if (next->dynamic_prio > aux->dynamic_prio) next = aux;                          // select the highest
        aux = aux->next;
    }
#ifdef DEBUG
    if (next != NULL) printf("\033[3;34m    Selected task %d\033[0m\n", next->id);
#endif
    if (next != NULL) {
        next->quantum = QUANTUM;
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

unsigned int systime() {
    return tick_counter;
}

void handle_clock_tick() {
    if (curr->quantum == 0 && curr != dispatcher) {
        task_yield();
    }
    tick_counter++;
    curr->quantum--;
    curr->proc_time++;
}

void dispatcher_function() {
    /*while there are tasks in the queue*/
    while (queue_size((queue_t *)task_queue) > 0 || queue_size((queue_t *)suspended_queue) > 0) {
        /*the scheduler select the next task*/
#ifdef DEBUG
        printf("\033[4;36m\033[3mdispatcher_function:\n\033[0m");
        queue_print("\033[3;34m    Task queue before:\033[0m", (queue_t *)suspended_queue, print_elem);  // cast task_t to queue_t
#endif
        check_suspended_queue();
        next = scheduler();
        if (next != NULL) {
            /*switch tasks*/
            switch (next->status) {
                case TERMINATED:
                    task_delete(next);
                    next = scheduler();
                    break;
                case SUSPENDED:
                    next = scheduler();
                    break;
            }
            if (next != NULL) task_switch(next);
#ifdef DEBUG
            printf("\033[4;36m\033[3mdispatcher_function:\n\033[0m");
            queue_print("\033[3;34m    Task queue after:\033[0m", (queue_t *)suspended_queue, print_elem);  // cast task_t to queue_t
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
    action.sa_handler = handle_clock_tick;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;     // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec = 0;         // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;  // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec = 0;      // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL
    if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
        perror("Erro em setitimer: ");
        exit(1);
    }

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
    main_task->dynamic_prio = 0;
    main_task->context = main_context;
    main_task->activations = 1;
    main_task->proc_time = 0;
    main_task->init_time = systime();
    main_task->wake_up_time = -1;
    main_task->waiting_id = -1;
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
    task->dynamic_prio = 0;
    task->init_time = systime();
    task->proc_time = 0;
    task->activations = 0;
    task->quantum = QUANTUM;
    task->wake_up_time = -1;
    task->waiting_id = -1;

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
    curr->status = TERMINATED;
    curr->exit_code = exit_code;
    curr->dynamic_prio = curr->static_prio;
    if (queue_size((queue_t *)suspended_queue) > 0) {
        task_t *aux = suspended_queue->next;
        for (int i = 0; i < queue_size((queue_t *)suspended_queue); i++) {
            if (aux->waiting_id == curr->id) task_resume(aux, &task_queue);
            aux = aux->next;
        }
    }
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", curr->id, systime() - curr->init_time, curr->proc_time, curr->activations);
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
    task->activations++;
    curr = task;
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
    curr->dynamic_prio = curr->static_prio;
    task_switch(dispatcher);
}