#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"
#include "queue.h"

typedef struct item_t {
    struct item_t *prev, *next;
    int n;
} item_t;

task_t p1, p2, p3, c1, c2;
semaphore_t s_vaga, s_buffer, s_item;
queue_t *buffer;

void produtor(void *arg) {
    while (1) {
        task_sleep(1000);
        // item = random(0..99);
        item_t *item = malloc(sizeof(item_t));
        item->n = rand() % 100;
        item->next = NULL;
        item->prev = NULL;
        
        sem_down(&s_vaga);
        sem_down(&s_buffer);
        // insere item no buffer
        printf("%s produziu %d\n", (char *)arg, item->n);
        queue_append(&buffer, (queue_t *)item);
        sem_up(&s_buffer);

        sem_up(&s_item);
    }
}

void consumidor(void *arg) {
    while (1) {
        sem_down(&s_item);
        sem_down(&s_buffer);
        // retira item do buffer
        item_t *item = (item_t *) buffer;
        queue_remove(&buffer, buffer);
        sem_up(&s_buffer);

        sem_up(&s_vaga);

        // print item
        printf("%s consumiu %d\n", (char *)arg, item->n);
        task_sleep(1000);
    }
}

int main(int argc, char *argv[]) {
    printf("main: inicio\n");

    ppos_init();

    // inicia semaforos
    sem_init(&s_vaga, 5);
    sem_init(&s_buffer, 1);
    sem_init(&s_item, 0);

    // inicia tarefas
    task_init(&p1, produtor, "p1");
    task_init(&p2, produtor, "p2");
    task_init(&p3, produtor, "p3");
    task_init(&c1, consumidor, "\t\tc1");
    task_init(&c2, consumidor, "\t\tc2");
    // aguarda
    task_wait(&p1);

    printf("main: fim\n");
    task_exit(0);
}