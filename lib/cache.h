#ifndef _CACHEH_
#define _CACHEH_

#include "linkedlist.h"
#include "hashtable.h"

typedef struct {
  size_t capacity, memory;
  linkedlist_t * linkedlist;
  hashtable_t * hashtable;
} cache_t;

cache_t * cache_init(size_t capacity, size_t memory, void (*free_member)(void*));
bool cache_put(cache_t * cache, size_t hash, size_t size, void * value);
void * cache_get(cache_t * cache, size_t hash);
void cache_recent(cache_t * cache, size_t hash);
size_t cache_size(cache_t * cache);
size_t cache_memory(cache_t * cache);
void cache_free(cache_t * cache);
void cache_visit(cache_t * cache, void (*visitor)(void*));

#endif

