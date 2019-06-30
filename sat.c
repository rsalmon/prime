#include "sat.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "parse.h"
#include <assert.h>
#include <setjmp.h>
#include <math.h>
#include <ctype.h>

DEF_STACK_IMP(token_t, stack, Stack)
DEF_STACK_IMP(index_t, list, List)

inline void stack_index_back(List * stack, index_t nsat) {
  stack->length = nsat;
}

inline bool stack_index_at(List * stack, int index, index_t element) {
  return index < stack->length && stack->data[index] == element;
}

void stack_dump(Stack * stack) {
  printf("stack dump:\n");
  for (int i = 0; i < stack_size(stack); i++)
    printf("stack[%d]: %2d.%d\n", i, stack_get(stack, i).id, stack_get(stack, i).value);
  printf("\n");
}

void stack_test() {
  Stack stack;
  index_t nvar = 5;
  stack_init(&stack, nvar);
  stack_pop(&stack);
  stack_dump(&stack);
  stack_push(&stack, (token_t){100,2});
  stack_dump(&stack);
  stack_pop(&stack);
  stack_push(&stack, (token_t){100,4});
  stack_dump(&stack);
  stack_push(&stack, (token_t){100,5});
  stack_dump(&stack);
  stack_free(&stack);
}

float max(float a, float b) {  
  return a > b ? a : b;
}

float min(float a, float b) {  
  return a < b ? a : b;
}

void swap(index_t * x, index_t * y) {
  index_t tmp = *x;
  *x = *y;
  *y = tmp;
}

int equal_token(token_t x, token_t y) {
  return x.id == y.id && x.value == y.value;
}

void show_witness(index_t nvar, Variable * vars) {
  for (int v = 1; v <= nvar; v++) {
    Variable * var = &vars[v];
    witness_t w1 = var->w1;
    witness_t w2 = var->w2;
    printf("var[%2d] %d.%d %d.%d |", var->id, w1.index, w1.clause, w2.index, w2.clause);
    for (int i = 0; i < var->nvalue; i++)
      for (int j = 0; j < var->nactive[i]; j++)
        printf(" %d.%d", var->actives[i][j], i);
    printf("\n");
  }
}

void show_var(Variable * vars, int nvar) {
  for (int v = 0; v < nvar; v++) {
    Variable * var = &vars[v];
    if (var->quantifier.exist) {
      if (var->value >= 0)
        printf("E[%d] = %d/%d @ %d(%d)\n", var->id, var->value, var->nvalue, var->block, var->decision);
      else
        printf("E[%d] = ?/%d @ %d(%d)\n", var->id, var->nvalue, var->block, var->decision);

      for (int w = 0; w < var->nvalue; w++) {
        array_t * watch = &var->watch[w];
        printf("%d: ", w);
        for (int c = 0; c < watch->length; c++)
          printf("%2d%s", watch->data[c], c + 1 < watch->length ? " " : "");
        printf("\n");
      }

    } else if (var->quantifier.universal) {
      if (var->value >= 0)
        printf("A[%d] = %d/%d @ %d(%d)\n", var->id, var->value, var->nvalue, var->block, var->decision);
      else
        printf("A[%d] = ?/%d @ %d(%d)\n", var->id, var->nvalue, var->block, var->decision);

      for (int w = 0; w < var->nvalue; w++) {
        array_t * watch = &var->watch[w];
        printf("%d: ", w);
        for (int c = 0; c < watch->length; c++)
          printf("%2d%s", watch->data[c], c + 1 < watch->length ? " " : "");
        printf("\n");
      }

    } else if (var->quantifier.randomize) {
      printf("R[%d", var->id);
      index_t size = 1;
      if (var->value >= 0)
        printf("] = %d/%d @ %d(%d)\n", var->value, var->nvalue, var->block, var->decision);
      else
        printf("] = ?/%d @ %d(%d)\n", var->nvalue, var->block, var->decision);

      for (int w = 0; w < var->nvalue; w++) {
        printf("%02d [%f]: ", w, var->prob[w]);
        array_t * watch = &var->watch[w];
        for (int c = 0; c < watch->length; c++)
          printf("%2d%s", watch->data[c], c + 1 < watch->length ? " " : "");
        printf("\n");
      }
    }
    printf("\n");
  }
  printf("\n");
}

void show_clause(Variable * vars, Clause * clauses, int nclause) {
  for (int c = 0; c < nclause; c++) {
    Clause * clause = &clauses[c];

    int satisfied = false;
    for (int l = 0; l < clause->ntoken; l++) {
      token_t token = clause->token[l];
      Variable * var = &vars[abs(token.id)];
      if (var->value >= 0 &&
          ((var->id == token.id && var->value == token.value) ||
          (var->id == -token.id && var->value != token.value))) {
        satisfied = true;
        break;
      }
    }

    if (satisfied) continue;
    printf("%02d. %d.%d |", c, clause->w1, clause->w2);
    for (int l = 0; l < clause->ntoken; l++) {
      token_t token = clause->token[l];
      Variable * var = &vars[abs(token.id)];
      if (var->value < 0)
        printf(" %d.%d", token.id, token.value);
    }
    printf("\n");
  }
  printf("\n");
}

void show_clause_at(Clause * clauses, index_t index) {
  Clause * clause = &clauses[index];
  printf("%d. %d.%d %d |", index, clause->w1, clause->w2, clause->ntoken);
  for (int l = 0; l < clause->ntoken; l++)
    printf(" %d.%d", clause->token[l].id, clause->token[l].value);
  printf("\n");
}

void sat_free(Sat * sat) {
  for (int v = 0; v < sat->nvar; v++) {
    Variable * var = &sat->vars[v];
    for (int i = 0; i < var->nvalue; i++) {
      free(var->actives[i]);
      array_free(&var->watch[i]);
    }

    free(var->values);
    free(var->watch);
    free(var->nactive);
    free(var->actives);
    free(var->score);
    free(var->value_present);
    free(var->value_index);
    free(var->value_list);
    if (var->quantifier.randomize)
      free(var->prob);
  }

  for (int c = 0; c < sat->nclause; c++) {
    free(sat->clauses[c].token);
  }
 
  free(sat->vars);
  free(sat->order);
  free(sat->clauses);
}

Sat init_sat() {
  Sat sat = (const Sat) {0};
  sat.scale = 1.0;
  return sat;
}

Variable init_var(index_t id) {
  Variable empty = (const Variable) {0};
  empty.id = id;
  empty.value = -1;
  empty.antecedent = -1;
  return empty;
}

inline int variable_compare(Variable * vars, int j, int k) {
  int best_by_visit = vars[k].nvisit > vars[j].nvisit;
  int best_by_quant_visit = (vars[k].block < vars[j].block) || (vars[k].block == vars[j].block && best_by_visit);
  return best_by_quant_visit;
}

Clause init_clause(index_t ntoken, token_t * token) {
  Clause clause = (const Clause) {0};
  clause.satisfied = 0;
  clause.nsatisfied = 0;
  clause.visited = 0;

  clause.w1 = 0;
  clause.w2 = ntoken/2;
  clause.ntoken = ntoken;
  clause.token = token;
  return clause;
}

float verify_assignment(Variable * vars, Stack * assigned) {
  float p = 1.0;
  for (int i = 0; i < stack_size(assigned); i++) {
    token_t token = stack_get(assigned, i);
    Variable * var = &vars[token.id];
    if (var->quantifier.randomize)
      p *= var->prob[token.value];
  }
  return p;
}

void show_assignment(Variable * vars, Stack * assigned, index_t max) {
  float p = verify_assignment(vars, assigned);
  printf("stack(%f):", p);
  for (int i = 0; i < stack_size(assigned); i++) {
    Variable * var = &vars[stack_get(assigned, i).id];
    if (var->value < 0) continue;
    if (var->id <= max) {
      if (var->antecedent >= 0)
        printf(" %d.%d[%d => %d]", var->id, var->value, var->decision, var->antecedent);
      else
        printf(" %d.%d[%d]", var->id, var->value, var->decision);
    }
  }
  printf("\n");
}

void var_count_literals(Variable * vars, Clause * clauses, index_t nvar, index_t nclause) {
  for (int c = 0; c < nclause; c++) {
    Clause * clause = &clauses[c];
    // tally score for pure literals and variable ordering
    for (int l = 0; l < clause->ntoken; l++) {
      token_t token = clause->token[l];
      if (token.id >= 0) {
        Variable * var = &vars[token.id];
        var->nactive[token.value]++;
        var->score[token.value] += var->quantifier.randomize ? var->prob[token.value] : 1;
      } else {
        Variable * var = &vars[-token.id];
        if (var->nvalue == 2) {
          token = (token_t) {-token.id, 1 - token.value};
          clause->token[l] = token;
          var->nactive[token.value]++;
          var->score[token.value] += var->quantifier.randomize ? var->prob[token.value] : 1;
        } else {
          for (int v = 0; v < var->nvalue; v++)
            if (v != token.value) {
              var->nactive[v]++;
              var->score[v] += var->quantifier.randomize ? var->prob[v] : 1;
            }
        }
      }
    }

    // tally literal counts for watch list
    token_t w[] = {clause->token[clause->w1], clause->token[clause->w2]};
    int limit = clause->ntoken == 1 ? 1 : 2; // edge case of single literal in clause
    for (int i = 0; i < limit; i++) {
      if (w[i].id >= 0)
        vars[w[i].id].watch[w[i].value].capacity++;
      else {
        Variable * var = &vars[-w[i].id];
        var->watch[(w[i].value + 1) % var->nvalue].capacity++;
      }
    }
  }
  
  const index_t minimum = 1;
  for (int v = 0; v < nvar; v++) {
    Variable * var = &vars[v];
    for (int i = 0; i < var->nvalue; i++) {
      var->actives[i] = malloc(var->nactive[i] * sizeof(index_t));
      var->nactive[i] = 0;
      var->watch[i].capacity += minimum;
      var->watch[i].data = malloc(var->watch[i].capacity * sizeof(index_t));
    }
  }

  for (int c = 0; c < nclause; c++) {
    Clause * clause = &clauses[c];
    token_t w[] = {clause->token[clause->w1], clause->token[clause->w2]};
    int limit = clause->ntoken == 1 ? 1 : 2; // edge case of single literal in clause

    for (int i = 0; i < limit; i++) {
      array_t * watch;
      if (w[i].id >= 0)
        watch = &vars[w[i].id].watch[w[i].value];
      else {
        Variable * var = &vars[-w[i].id];
        watch = &var->watch[(w[i].value + 1) % var->nvalue];
      }
      watch->data[watch->length++] = c;
    }

    for (int l = 0; l < clause->ntoken; l++) {
      token_t token = clause->token[l];
      if (token.id >= 0) {
        Variable * var = &vars[token.id];
        var->actives[token.value][var->nactive[token.value]++] = c;
      } else {
        Variable * var = &vars[-token.id];
        for (int v = 0; v < var->nvalue; v++)
          if (v != token.value)
            var->actives[v][var->nactive[v]++] = c;
      }
    }
  }

  // set active witness values
  for (int v = 0; v < nvar; v++) {
    Variable * var = &vars[v];
    var->w1 = var->w2 = (witness_t) {-1, -1};
    for (int i = 0; i < var->nvalue; i++)
      if (var->nactive[i] > 0) {
        var->w1.index = i;
        var->w1.clause = 0;
        break;
      }
    for (int i = var->nvalue - 1; i >= 0; i--)
      if (var->nactive[i] > 0) {
        var->w1.index = i;
        var->w1.clause = var->nactive[i] - 1;
        break;
      }
  }
}

token_t read_token(char * ptr) {
  token_t token;
  token.id = strtol(ptr, &ptr, 10);
  ptr = strchr(ptr, '.') + 1;
  token.value = strtol(ptr, &ptr, 10);
  return token;
}

int var_order(const void * first1, const void * second1) {
  Variable * first = *((Variable **)first1);
  Variable * second = *((Variable **)second1);
  real_t score1 = first->block;
  real_t score2 = second->block;
  int comp = (int)(score1 - score2);
  if (comp != 0) return comp;

  for (int i = 0; i < first->nvalue; i++)
    score1 += first->score[i];
  for (int i = 0; i < second->nvalue; i++)
    score2 += second->score[i];
  return (int)(score2 - score1);
}

int clause_order(const void * varp_, const void * varq_) {
  token_t * varp = (token_t *)varp_;
  token_t * varq = (token_t *)varq_;
  int comp = (int)(varp->id - varq->id);
  if (comp != 0) return comp;
  return (int)(varp->value - varq->value);
}

array_t * remove_watch_literal(array_t * watch, index_t c) {
  for (int l = 0; l < watch->length; l++)
    if (array_get(watch, l) == c) {
      watch->data[l] = watch->data[--watch->length];
      break;
    }
  return watch;
}

array_t * add_watch_literal(array_t * watch, index_t c) {
  if (watch->length >= watch->capacity) {
    // increase length
    index_t length = watch->capacity;
    watch->capacity += WATCH_LIST_EXTRA_SPACE;
    index_t * literals = (index_t *) malloc(watch->capacity * sizeof(index_t));
    for (int l = 0; l < length; l++)
      literals[l] = watch->data[l];
    free(watch->data);
    watch->data = literals;
  }

  watch->data[watch->length++] = c;
  return watch;
}

void stack_dump_line(Stack * stack, char * name) {
  printf("stack %s:\t", name);
  for (int i = 0; i < stack_size(stack); i++)
    printf("%3d.%d", stack_get(stack, i).id, stack_get(stack, i).value);
  printf("\n");
}

void var_path_count(Clause * clauses, Variable * vars, char * hash, token_t * tokens, index_t ntoken, index_t * unique_implication_point_id, index_t id_skip) {
  for (int l = 0; l < ntoken; l++) {
    index_t id = tokens[l].id;
    index_t antecedent = vars[id].antecedent;
    hash[id]--;
    if (hash[id] < hash[*unique_implication_point_id]) *unique_implication_point_id = id;
    if (id != id_skip && antecedent >= 0) {
      Clause * clause = &clauses[antecedent];
      var_path_count(clauses, vars, hash, clause->token, clause->ntoken, unique_implication_point_id, id);
    }
  }
}

void unassign(Sat * sat, index_t start) {
  Variable * vars = sat->vars;
  Clause * clauses = sat->clauses;
  Stack * assigned = sat->assigned;
  while (stack_size(assigned) > start) {
    token_t token = stack_pop(assigned);
    Variable * var = &vars[token.id];
    var->antecedent = -1;
    var->value = -1;

    index_t * clauses_active = var->actives[token.value];
    index_t nactive = var->nactive[token.value];
    for (int i = 0; i < nactive ; i++)
      clauses[clauses_active[i]].nsatisfied--;
  }
}

void init_var_default(Variable * var, index_t id, index_t nvalue, quantifier_t quantifier, int block) {
  // initialize other parameters
  *var = (const Variable) {0};

  var->id = id;
  var->value = -1;
  var->antecedent = -1;
  var->decision = 0;

  var->block = block;
  var->nvalue = nvalue;
  var->nchoice = nvalue;
  var->quantifier = quantifier;
  var->values = malloc(var->nvalue * sizeof(index_t));
  var->watch = malloc(var->nvalue * sizeof(array_t));
  var->nactive = malloc(var->nvalue * sizeof(index_t));
  var->actives = malloc(var->nvalue * sizeof(index_t *));
  var->score = malloc(var->nvalue * sizeof(float));

  var->nvalue_list = 0;
  var->value_present = malloc(var->nvalue * sizeof(index_t));
  var->value_index = malloc(var->nvalue * sizeof(index_t));
  var->value_list = malloc(var->nvalue * sizeof(index_t));
  for (int i = 0; i < var->nvalue; i++) {
    var->nactive[i] = 0;
    var->actives[i] = NULL;
    var->score[i] = 0.0;
    var->values[i] = 1;
    var->watch[i] = (array_t){0, 0, NULL};
    var->value_present[i] = -1;
  }
}

