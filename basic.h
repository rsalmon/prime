#include <stdbool.h> // boolean values

#ifndef _BASICH_
#define _BASICH_

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

typedef int index_t;
typedef double real_t;
typedef unsigned int uint;
typedef struct {
  index_t id, value;
} token_t;

typedef struct {
  index_t index, clause;
} witness_t;

bool token_equal(token_t *t1, token_t *t2);

typedef union quantifier_t {
  struct {
    uint exist : 1;
    uint universal : 1;
    uint randomize : 1;
    uint derive : 1;
    uint from_finite : 1;
  };
  unsigned short all;
} quantifier_t; 

bool quantifier_equal(quantifier_t *q1, quantifier_t *q2);
bool quantifier_compare(quantifier_t *q1, quantifier_t *q2);

typedef struct {
  index_t length, capacity;
  index_t * data;
} array_t;

array_t* array_append(array_t *array, index_t e);
bool array_remove(array_t *array, index_t e);
index_t array_get(array_t *array, index_t i);
index_t array_size(array_t *array);
index_t array_capacity(array_t *array);
array_t array_init(uint capacity);
void array_free(array_t *array);

#endif
