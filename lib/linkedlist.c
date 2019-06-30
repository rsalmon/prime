#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"

linkedlist_t * linkedlist_init(void (*free_member)(void*)) {
  linkedlist_t * list = malloc(sizeof(linkedlist_t));
  list->head = malloc(sizeof(linkedlistnode_t));
  list->tail = malloc(sizeof(linkedlistnode_t));
  list->free_member = free_member;
  list->size = 0;
  list->memory = 0;
  list->head->next = NULL;
  list->head->prev = list->tail;
  list->tail->next = list->head;
  list->tail->prev = NULL;
  return list;
}

void * linkedlist_add_head(linkedlist_t * list, size_t hash, size_t memory, void * value) {
  linkedlistnode_t * node = malloc(sizeof(linkedlistnode_t));
  node->hash = hash;
  node->memory = memory;
  node->value = value;
  node->next = list->head;
  node->prev = list->head->prev;
  list->head->prev->next = node;
  list->head->prev = node;

  list->size++;
  list->memory += memory;
  return node;
}

void * linkedlist_get(linkedlist_t * list, size_t hash) {
  linkedlistnode_t * node = list->tail->next;
  while (node != list->head) {
    if (node->hash == hash)
      return node;
    node = node->next;
  }
  return NULL;
}

void * linkedlist_get_tail(linkedlist_t * list) {
  linkedlistnode_t * node = list->tail->next;
  if (node != list->head)
    return node;
  return NULL;
}

void linkedlist_remove(linkedlist_t * list, linkedlistnode_t * node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  list->size--;
  list->memory -= node->memory;
  if (list->free_member != NULL)
    list->free_member(node->value);
  free(node);
}

int linkedlist_size(linkedlist_t * list) {
  return list->size;
}

int linkedlist_memory(linkedlist_t * list) {
  return list->memory;
}

void linkedlist_free(linkedlist_t * list) {
  linkedlistnode_t * node = list->tail->next;
  while (node != list->head) {
    if (list->free_member != NULL)
      list->free_member(node->value);
    node = node->next;
    free(node->prev);
  }
  free(list->tail);
  free(list->head);
  free(list);
}

void linkedlist_move_head(linkedlist_t * list, linkedlistnode_t * node) {
  // remove from list
  node->prev->next = node->next;
  node->next->prev = node->prev;

  // add to head
  node->next = list->head;
  node->prev = list->head->prev;
  list->head->prev->next = node;
  list->head->prev = node;
}

void linkedlist_remove_tail(linkedlist_t * list) {
  linkedlistnode_t * node = list->tail->next;
  if (node == list->head) return;

  // remove from list
  node->prev->next = node->next;
  node->next->prev = node->prev;
  list->size--;
  list->memory -= node->memory;
  if (list->free_member != NULL)
    list->free_member(node->value);
  free(node);
}

void linkedlist_visit(linkedlist_t * list, void (*visitor)(void*)) {
  linkedlistnode_t * node = list->tail->next;
  while (node != list->head) {
    visitor(node->value);
    node = node->next;
  }
}
