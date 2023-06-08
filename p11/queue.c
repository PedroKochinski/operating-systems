// GRR20206144 Pedro Henrique Kochinki Silva
#include "queue.h"

#include <stdio.h>

/*returns number of elements in the queue*/
int queue_size(queue_t *queue) {
    int count = 0;
    queue_t *first = queue;
    queue_t *aux = first;
    /* Verify if the list is empty */
    if (first == NULL) {
        return 0;
    }
    /* Count the number of elements in the list */
    do {
        count++;
        aux = aux->next;
    } while (first != aux);
    return count;
}

/* Prints the contents of the queue */
void queue_print(char *name, queue_t *queue, void print_elem(void *)) {
    printf("%s", name);
    if (queue == NULL) {
        printf("[]");
    } else {
        queue_t *first = queue;
        printf("[");
        /* Iterate over the elements in the queue */
        for (int i = 0; i < queue_size(queue) - 1; i++) {
            /* Print the current element using the given function */
            print_elem(first);
            printf(" ");
            first = first->next;
        }
        /* Print the last element */
        print_elem(first);
        printf("]");
    }
    printf("\n");
}

/* Appends an element to the end of the queue */
int queue_append(queue_t **queue, queue_t *elem) {
    /* Verify if the element exists */
    if (elem == NULL) {
        fprintf(stderr, "%s", "The element does not exist\n");
        return 1;
    }

    /* Verify if the element is already in a queue */
    if (elem->next != NULL || elem->prev != NULL) {
        fprintf(stderr, "%s", "The element is already in a queue\n");
        return 1;
    }

    queue_t *first = *queue; 

    if (first == NULL) {
        /* The element is the first of the queue */
        elem->next = elem;
        elem->prev = elem;
        first = elem;

    } else {
        /* Append the element to the end of the queue */
        elem->next = first;
        elem->prev = first->prev;
        first->prev->next = elem;
        first->prev = elem;
    }
    /* Update the original queue */
    *queue = first;
    return 0;
}

/* Removes an element from the queue */
int queue_remove(queue_t **queue, queue_t *elem) {
    /* Verify if the queue exists */
    if (*queue == NULL) {
        fprintf(stderr, "%s", "The queue does not exist\n");
        return 1;
    }

    /* Verify if the element exists */
    if (elem == NULL) {
        fprintf(stderr, "%s", "The element does not exist\n");
        return 1;
    }

    /* Verify if the element is the first in the queue and the queue has one element*/
    if (elem == *queue && (*queue)->next == *queue) {
        (*queue)->next = NULL;
        (*queue)->prev = NULL;
        *queue = NULL;
        return 0;
    }
    /* Verify if the element is the first in the queue */
    if (elem == *queue) {
        (*queue)->next->prev = (*queue)->prev;
        (*queue)->prev->next = (*queue)->next;
        *queue = (*queue)->next;
        elem->next = NULL;
        elem->prev = NULL;
        return 0;
    }

    queue_t *first = *queue;
    queue_t *aux = first->next;
    int elem_found = 0;

    /* search for the element */
    while (aux != *queue && elem_found == 0) {
        /*found element in the queue*/
        if (elem == aux) {
            elem_found = 1;
            aux->prev->next = aux->next;
            aux->next->prev = aux->prev;
            aux->prev = NULL;
            aux->next = NULL;
        }
        aux = aux->next;
    }

    /* Verify if the element has not been found */
    if (!elem_found) {
        fprintf(stderr, "%s", "The element is not in the queue\n");
        return 1;
    }

    *queue = first;
    return 0;
}
