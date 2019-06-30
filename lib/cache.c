#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "linkedlist.h"
#include "hashtable.h"
#include "cache.h"

cache_t * cache_init(size_t capacity, size_t memory, void (*free_member)(void*)) {
  cache_t * cache = malloc(sizeof(cache_t));
  cache->capacity = capacity;
  cache->memory = memory;
  cache->linkedlist = linkedlist_init(free_member);
  cache->hashtable = hashtable_init(capacity, NULL);
  return cache;
}

bool cache_put(cache_t * cache, size_t hash, size_t memory, void * value) {
  linkedlistnode_t * result = hashtable_get(cache->hashtable, hash);
  if (result != NULL)
    linkedlist_remove(cache->linkedlist, result);

  while (linkedlist_size(cache->linkedlist) > cache->capacity-1 || cache_memory(cache) > cache->memory-memory) {
    linkedlistnode_t * tail = linkedlist_get_tail(cache->linkedlist);
    hashtable_remove(cache->hashtable, tail->hash);
    linkedlist_remove(cache->linkedlist, tail);
  }

  linkedlistnode_t * node = linkedlist_add_head(cache->linkedlist, hash, memory, value);
  hashtable_put(cache->hashtable, hash, node);
  return true;
}

void cache_recent(cache_t * cache, size_t hash) {
  linkedlistnode_t * result = hashtable_get(cache->hashtable, hash);
  linkedlist_move_head(cache->linkedlist, result);
}

void * cache_get(cache_t * cache, size_t hash) {
  linkedlistnode_t * result = hashtable_get(cache->hashtable, hash);
  if (result == NULL) return NULL;
  //linkedlist_move_head(cache->linkedlist, result);
  return result->value;
}

size_t cache_size(cache_t * cache) {
  return linkedlist_size(cache->linkedlist);
}

size_t cache_memory(cache_t * cache) {
  return linkedlist_memory(cache->linkedlist);
}

void cache_free(cache_t * cache) {
  linkedlist_free(cache->linkedlist);
  hashtable_free(cache->hashtable);
  free(cache);
}

void cache_visit(cache_t * cache, void (*visitor)(void*)) {
  linkedlist_visit(cache->linkedlist, visitor);
}


