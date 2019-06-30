#ifndef _HASHTABLEH_
#define _HASHTABLEH_

typedef struct {
  size_t size, capacity;
  void (*free_member)(void*);
  void ** array;
} hashtable_t;

hashtable_t * hashtable_init(int capacity, void (*free_member)(void*));
bool hashtable_put(hashtable_t * hashtable, uint hash, void * value);
void hashtable_remove(hashtable_t * hashtable, uint hash);
void * hashtable_get(hashtable_t * hashtable, size_t hash);
//int hashtable_equal(HashEntry * entry1, HashEntry * entry2);
size_t hashtable_size(hashtable_t * hashtable);
void hashtable_free(hashtable_t * hashtable);

#endif

