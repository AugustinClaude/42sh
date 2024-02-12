#include "fifo.h"

#include <stdio.h>
#include <stdlib.h>

struct fifo *fifo_init(void)
{
    struct fifo *fifo = malloc(sizeof(struct fifo));
    if (!fifo)
        return NULL;

    fifo->head = NULL;
    fifo->tail = NULL;
    fifo->size = 0;

    return fifo;
}

void fifo_push(struct fifo *fifo, struct token tok)
{
    struct list *list = malloc(sizeof(struct list));
    if (!list)
        return;

    list->data = tok;

    if (!fifo->head)
    {
        list->next = NULL;
        fifo->tail = list;
        fifo->head = list;
        fifo->size = 1;
        return;
    }

    list->next = NULL;
    fifo->tail->next = list;
    fifo->size++;
}

void fifo_pop(struct fifo *fifo)
{
    if (!fifo->head)
        return;

    if (!fifo->head->next)
    {
        struct list *tmp = fifo->head;
        fifo->head = NULL;
        fifo->tail = NULL;
        fifo->size--;
        free(tmp);
        return;
    }

    struct list *tmp = fifo->head;
    fifo->head = tmp->next;
    fifo->size--;
    free(tmp);
}

void fifo_clear(struct fifo *fifo)
{
    struct list *tmp = fifo->head;

    while (tmp)
    {
        struct list *tmp2 = tmp;
        tmp = tmp->next;
        free(tmp2);
    }

    fifo->head = NULL;
    fifo->tail = NULL;
    fifo->size = 0;
}

void fifo_destroy(struct fifo *fifo)
{
    fifo_clear(fifo);
    free(fifo);
}
