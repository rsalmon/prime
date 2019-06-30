#include <stdlib.h>
#include <stdio.h>
#include "basic.h"

quantifier_t quantifier_default = {0,0,0,0,0};
bool token_equal(token_t *t1, token_t *t2) {
  return t1->id == t2->id && t2->value == t2->value;
}

bool quantifier_equal(quantifier_t *q1, quantifier_t *q2) {
  return q1->exist == q2->exist && q1->universal == q2->universal && q1->randomize == q2->randomize;
}

bool quantifier_compare(quantifier_t *q1, quantifier_t *q2) {
  return q1->exist > q2->exist || q1->exist == q2->exist && (q1->universal > q2->universal || q1->universal == q2->universal && (q1->randomize > q2->randomize));
}

bool array_remove(array_t *array, index_t e) {
  for (int l = 0; l < array->length; l++)
    if (array->data[l] == e) {
      array->data[l] = array->data[--array->length];
      return true;
    }
  return false;
}

array_t* array_append(array_t *array, index_t e) {
  // increase capacity
  if (array->length >= array->capacity) {
    index_t capacity = array->capacity;
    const int WATCH_LIST_EXTRA_SPACE = 3;
    array->capacity += WATCH_LIST_EXTRA_SPACE;
    index_t * data = (index_t *) malloc(array->capacity * sizeof(index_t));
    for (int l = 0; l < capacity; l++)
      data[l] = array->data[l];
    free(array->data);
    array->data = data;
  }

  array->data[array->length++] = e;
  return array;
}

index_t array_get(array_t *array, index_t i) {
  return array->data[i];
}

index_t array_size(array_t *array) {
  return array->length;
}

index_t array_capacity(array_t *array) {
  return array->capacity;
}

void array_free(array_t *array) {
  free(array->data);
}

array_t array_init(uint capacity) {
  return (array_t) {0, capacity, malloc(capacity * sizeof(index_t))};
}
