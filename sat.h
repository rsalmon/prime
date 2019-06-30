#include <stdio.h>
#include "basic.h"

#define true 1
#define false 0
#define LEARN_CLAUSE_LIMIT 1000
#define WATCH_LIST_EXTRA_SPACE 5
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

typedef int index_t;
typedef double real_t;
typedef unsigned int uint;
#define DEF_STACK_HEADER(dtype, prefix, name) \
  typedef struct { \
    int length, capacity; \
    dtype * data; \
  } name; \
  void prefix##_init(name * stack, int max);\
  name prefix##_init_static(int max, dtype * data); \
  int prefix##_clear(name * stack);\
  int prefix##_empty(name * stack);\
  int prefix##_full(name * stack);\
  void prefix##_push(name * stack, dtype value);\
  int prefix##_size(name * stack);\
  dtype prefix##_pop(name * stack);\
  dtype prefix##_get(name * stack, int index);\
  void prefix##_free(name * stack);\
  void prefix##_dump(name * stack);\

#define DEF_STACK_IMP(dtype, prefix, name) \
  inline void prefix##_init(name * stack, int max) { \
    *stack = (name){0, max, malloc(max * sizeof(dtype))}; \
  } \
  inline name prefix##_init_static(int max, dtype * data) { \
    return (name){0, max, data}; \
  } \
  inline int prefix##_clear(name * stack) { \
    stack->length = 0; \
  } \
  inline int prefix##_empty(name * stack) { \
    return stack->length == 0; \
  } \
  inline int prefix##_full(name * stack) { \
    return stack->length >= stack->capacity; \
  } \
  inline void prefix##_push(name * stack, dtype value) { \
    if (prefix##_full(stack)) { \
      printf("stack(%d/%d): pushing to full stack!\n", stack->length, stack->capacity); \
      exit(-1); \
    } \
    stack->data[stack->length++] = value; \
  } \
  inline int prefix##_size(name * stack) { \
    return stack->length; \
  } \
  inline dtype prefix##_pop(name * stack) { \
    if (prefix##_empty(stack)) { \
      printf("stack: popping from empty stack!\n"); \
      exit(-1); \
    } \
    return stack->data[--stack->length]; \
  } \
  inline dtype prefix##_get(name * stack, int index) { \
    if (index <= 0 || index >= prefix##_size(stack)) { \
      printf("stack: indexing from too small of a stack!\n"); \
      exit(-1); \
    } \
    return stack->data[index]; \
  } \
  inline void prefix##_free(name * stack) { \
    free(stack->data); \
  } \

#define DEF_QUEUE_HEADER(dtype, prefix, name) \
  typedef struct { \
    int start, count, capacity; \
    dtype * data; \
  } name; \
  void prefix##_init(name * queue, int max);\
  name prefix##_init_static(int max, dtype * data); \
  int prefix##_empty(name * queue);\
  int prefix##_full(name * queue);\
  void prefix##_push(name * queue, dtype value);\
  int prefix##_size(name * queue);\
  dtype prefix##_pop(name * queue);\
  void prefix##_free(name * queue);\
  void prefix##_dump(name * queue);\

#define DEF_QUEUE_IMP(dtype, prefix, name) \
  inline void prefix##_init(name * queue, int max) { \
    *queue = (name){0, 0, max, malloc(max * sizeof(dtype))}; \
  } \
  inline name prefix##_init_static(int max, dtype * data) { \
    return (name){0, 0, max, data}; \
  } \
  inline int prefix##_empty(name * queue) { \
    return queue->count == 0; \
  } \
  inline int prefix##_full(name * queue) { \
    return queue->count >= queue->capacity; \
  } \
  inline void prefix##_push(name * queue, dtype value) { \
    if (prefix##_full(queue)) { \
      printf("queue(%d/%d): adding to full queue!\n", queue->count, queue->capacity); \
      exit(-1); \
    } \
    queue->data[(queue->start + queue->count++) % queue->capacity] = value; \
  } \
  inline int prefix##_size(name * queue) { \
    return queue->count; \
  } \
  inline dtype prefix##_pop(name * queue) { \
    if (prefix##_empty(queue)) { \
      printf("queue: removing from empty queue!\n"); \
      exit(-1); \
    } \
    dtype element = queue->data[queue->start]; \
    queue->start = (queue->start + 1) % queue->capacity; \
    queue->count--; \
    return element; \
  } \
  inline void prefix##_free(name * queue) { \
    free(queue->data); \
  } \

DEF_STACK_HEADER(token_t, stack, Stack)
DEF_STACK_HEADER(index_t, list, List)
DEF_QUEUE_HEADER(token_t, queue, Queue)
void stack_index_back(List * stack, index_t nsat);
bool stack_index_at(List * stack, int index, index_t element);

typedef struct {
  index_t id, literal;
  real_t prob;
} TokenValue;

typedef struct {
  index_t w1, w2, ntoken, satisfied, visited, nsatisfied;
  token_t * token;
} Clause;

typedef struct {
  index_t id, decision, block, value, nvalue, antecedent, nvisit, visited, order;
  index_t nchoice;
  index_t * values;
  quantifier_t quantifier;
  witness_t w1, w2;
  index_t * nactive;
  index_t ** actives;
  float sum; 
  float * score, * prob;
  array_t * watch;
  index_t nvalue_list, *value_present, *value_list, *value_index;
} Variable;

typedef struct {
  unsigned int clause_learning : 1;
  unsigned int pure_literal : 1;
  unsigned int nonchron_backjump : 1;
  unsigned int save_solution : 1;
  unsigned int sub_problem : 1;
  unsigned int cache : 1;
  unsigned int iterative_deepening : 1;
  unsigned int guess_exist : 1;
  int ndecision;
  int nvisit;
  int ncomponent;
  int ncache_hit;
} Option;

typedef struct {
  index_t nvar, nclause, output, ntoken, nvalue, horizon;
  float scale, error;
  Option option;
  index_t * order;
  Variable * vars;
  Clause * clauses;
  index_t nblock;
  Stack * assigned;
  List * satisfy;
  Stack * propagate;
  void * hashtable;
  void * cache;
  void * canonical;
} Sat;

void init_var_default(Variable * var, index_t id, index_t nvalue, quantifier_t type, int quantifier);
token_t read_token(char * ptr);

// ssat stuff
int variable_compare(Variable * vars, int j, int k);
void show_var(Variable * vars, int nvar);
void show_clause(Variable * vars, Clause * clauses, int nclause);
void show_clause_at(Clause * clauses, index_t index);
void sat_free(Sat * sat);

int from_file(int (*accept)(void *, index_t, index_t, float, index_t *, index_t *, char *, float **, index_t *, token_t **), FILE * cnf, void * state);
void var_count_literals(Variable * vars, Clause * clauses, index_t nvar, index_t nclause);

int from_cnf_to_cir(FILE * cnf, FILE * cir);
Clause init_clause(index_t ntoken, token_t * token);
Sat init_sat();
void test();

void show_assignment(Variable * vars, Stack * assigned, index_t max);
void unassign(Sat * sat, index_t start);
array_t * add_watch_literal(array_t * watch, index_t c);
array_t * remove_watch_literal(array_t * watch, index_t c);
void show_witness(index_t nvar, Variable * vars);
int clause_order(const void * varp_, const void * varq_);
int var_order(const void * first1, const void * second1);
int equal_token(token_t x, token_t y);
