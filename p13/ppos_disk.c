#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "disk.h"
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

struct request_t {
    struct request_t *prev, *next;
    int block;
    void *buffer;
    int write;
};

struct task_t *disk_manager;
struct queue_t *disk_queue;
struct queue_t *disk_suspended_queue;
semaphore_t s_disk;

struct sigaction disk_action;

extern struct queue_t *task_queue;
extern struct queue_t *suspended_queue;
extern struct task_t *dispatcher;
extern struct task_t *curr;

void diskDriverBody(void *args) {
    for (;;) {
        sem_down(&s_disk);
        // se foi acordado devido a um sinal do disco
        // acorda a tarefa cujo pedido foi atendido
        // tratado no tratador :)

        // se o disco estiver livre e houver pedidos de E/S na fila
        if ((disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE) &&
            (queue_size(disk_queue) > 0)) {
            // escolhe na fila o pedido a ser atendido, usando FCFS
            // solicita ao disco a operação de E/S, usando disk_cmd()
            struct request_t *first = (struct request_t *)disk_queue;
            first->write ? disk_cmd(DISK_CMD_WRITE, first->block, first->buffer) : disk_cmd(DISK_CMD_READ, first->block, first->buffer);
            queue_remove(&disk_queue, (struct queue_t *)first);
            // queue_append(&disk_suspended_queue, (struct queue_t *)first);
        }

        sem_up(&s_disk);
        task_yield();
    }
}

void disk_interruption(int signum) {
    if (signum == SIGUSR1) {
        task_resume((struct task_t *)disk_suspended_queue, (struct task_t **)&disk_suspended_queue);
    }
}

int disk_mgr_init(int *numBlocks, int *blockSize) {
    disk_action.sa_handler = disk_interruption;
    sigemptyset(&disk_action.sa_mask);
    disk_action.sa_flags = 0;
    if (sigaction(SIGUSR1, &disk_action, 0) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }

    if (disk_cmd(DISK_CMD_INIT, 0, 0)) return -1;
    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    sem_init(&s_disk, 1);
    disk_manager = malloc(sizeof(struct task_t));
    task_init(disk_manager, diskDriverBody, 0);
    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer) {
    sem_down(&s_disk);
    // inclui o pedido na fila_disco
    struct request_t *request = malloc(sizeof(struct request_t));
    request->next = NULL;
    request->prev = NULL;
    request->block = block;
    request->buffer = buffer;
    request->write = 0;

    queue_append(&disk_queue, (struct queue_t *)request);
    
    if (disk_manager->status == SUSPENDED) {
        task_resume(disk_manager, (struct task_t **)&suspended_queue);
    }

    sem_up(&s_disk);
    task_suspend((struct task_t **)&disk_suspended_queue);
    task_yield();
    return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer) {
    sem_down(&s_disk);
    struct request_t *request = malloc(sizeof(struct request_t));
    request->next = NULL;
    request->prev = NULL;
    request->block = block;
    request->buffer = buffer;
    request->write = 1;

    queue_append(&disk_queue, (struct queue_t *)request);

    if (disk_manager->status == SUSPENDED) {
        task_resume(disk_manager, (struct task_t **)&suspended_queue);
    }

    sem_up(&s_disk);
    task_suspend((struct task_t **)&disk_suspended_queue);
    task_yield();
    return 0;
}
