#include <stdlib.h>
#include "hash.h"

size_t hash_float(float x) {
  return *(unsigned*)&x;
}

hash_t * hash_init() {
  hash_t * hash = malloc(sizeof(hash_t));
  hash->state = 0;
  hash->ntable = 256;
  hash->table = malloc(hash->ntable * sizeof(size_t));
  for (size_t i = 0; i < hash->ntable; i++)
    hash->table[i] = rand();
  return hash;
}

void hash_update(hash_t * hash, size_t value) {
  hash->state ^= hash->table[(value ^ hash->state) % hash->ntable];
}

void hash_reset(hash_t * hash) {
  hash->state = 0;
}

size_t hash_get(hash_t * hash) {
  return hash->state;
}

void hash_free(hash_t * hash) {
  free(hash->table);
  free(hash);
}

