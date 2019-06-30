#ifndef _HASHH_
#define _HASHH_

typedef struct {
  size_t state, ntable, * table;
} hash_t;

hash_t * hash_init();
size_t hash_float(float x);
void hash_update(hash_t * hash, size_t value);
void hash_reset(hash_t * hash);
size_t hash_get(hash_t * hash);
void hash_free(hash_t * hash);

#endif

