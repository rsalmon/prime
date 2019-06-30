#include <stdlib.h>
#include <stdbool.h>
#include "hashtable.h"

hashtable_t * hashtable_init(int capacity, void (*free_member)(void*)) {
  hashtable_t * hashtable = malloc(sizeof(hashtable_t));
  hashtable->array = calloc(capacity, sizeof(void*));
  hashtable->free_member = free_member;
  hashtable->capacity = capacity;
  hashtable->size = 0;
  return hashtable;
}

bool hashtable_put(hashtable_t * hashtable, uint hash, void * value) {
  size_t key = hash % hashtable->capacity;
  if (hashtable->array[key] != NULL) {
    hashtable->size--;
    if (hashtable->free_member != NULL)
      (*hashtable->free_member)(hashtable->array[key]);
  }

  hashtable->array[key] = value;
  hashtable->size++;
  return true;
}

void hashtable_remove(hashtable_t * hashtable, uint hash) {
  size_t key = hash % hashtable->capacity;
  if (hashtable->array[key] != NULL && hashtable->free_member != NULL)
    hashtable->free_member(hashtable->array[key]);

  hashtable->array[key] = NULL;
  hashtable->size--;
}

void * hashtable_get(hashtable_t * hashtable, size_t hash) {
  size_t key = hash % hashtable->capacity;
  return hashtable->array[key];
}

//int hashtable_equal(HashEntry * entry1, HashEntry * entry2);
size_t hashtable_size(hashtable_t * hashtable) {
  return hashtable->size;
}

void hashtable_free(hashtable_t * hashtable) {
  for (size_t i = 0; i < hashtable->capacity; i++)
    if (hashtable->array[i] != NULL && hashtable->free_member != NULL)
      (*hashtable->free_member)(hashtable->array[i]);
  free(hashtable->array);
  free(hashtable);
}

