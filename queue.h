#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdio.h>
#include <stdlib.h>

#include "quadtree.h"

typedef quadtreenode_t Item;

typedef struct QueueNode {
  Item elem;
  struct QueueNode *next;
} QueueNode;

typedef struct Queue {
  QueueNode *front;
  QueueNode *rear;
  long size;
} Queue;

Queue *createQueue(void) {
  Queue *q = (Queue *)malloc(sizeof(Queue));
  q->front = NULL;
  q->rear = NULL;
  q->size = 0;
  return q;
}

int isQueueEmpty(Queue *q) {
  if (q == NULL || q->front == NULL) return 1;
  return 0;
}

void enqueue(Queue *q, Item elem) {
  QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
  node->elem = elem;
  node->next = NULL;
  if (isQueueEmpty(q) == 1) {
    q->front = q->rear = node;
    q->size++;
    return;
  }
  q->rear->next = node;
  q->rear = node;
  q->size++;
  return;
}

Item front(Queue *q) {
  if (isQueueEmpty(q) == 1) exit(1);
  return q->front->elem;
}

void dequeue(Queue *q) {
  if (q == NULL || q->front == NULL || q->rear == NULL) return;
  QueueNode *temp = q->front;
  q->front = q->front->next;
  if (q->front == NULL) q->rear = NULL;
  free(temp);
  q->size--;
}

void destroyQueue(Queue *q) {
  QueueNode *temp;
  while (q->size) {
    temp = q->front;
    q->front = q->front->next;
    free(temp);
    q->size--;
  }
  free(q);
}

#endif

