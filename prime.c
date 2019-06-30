#include "sat.h"
#include "component.h"
#include "canonical.h"
#include "prime.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include "cache.h"

#define NANO_PER_SEC 1000000000.0

real_t max_solver_duration = 1000.0;
int timeout = false;
void signal_handler(int sig) {
  if (sig == SIGALRM) {
    timeout = true;
  }
}

inline void swap(index_t * x, index_t * y) {
  index_t tmp = *x;
  *x = *y;
  *y = tmp;
}

inline void witness_swap(witness_t * x, witness_t * y) {
  witness_t tmp = *x;
  *x = *y;
  *y = tmp;
}

void show_clause_(Variable * vars, Clause * clauses, int nclause) {
  for (int c = 0; c < nclause; c++) {
    Clause * clause = &clauses[c];

    int satisfied = false;
    for (int l = 0; l < clause->ntoken; l++) {
      token_t token = clause->token[l];
      Variable * var = &vars[token.id];
      if (var->value >= 0 && var->id == token.id && var->value == token.value) {
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

inline bool clause_satisfied(Clause * clause, List * satisfy, index_t c) {
  return stack_index_at(satisfy, clause->satisfied, c);
}

real_t sat_constraint_propagate(Sat * sat, index_t decision, token_t token0) {
  const int show = false;
  Variable * vars = sat->vars;
  Clause * clauses = sat->clauses;
  Stack * assigned = sat->assigned;
  Stack * propagate = sat->propagate;
  List * satisfy = sat->satisfy;
  stack_clear(propagate);
  stack_push(propagate, token0);

  real_t path = 1.0;
  while ( !stack_empty(propagate) ) {
    token_t token = stack_pop(propagate);

    if (show) printf("constraint_propagate: %d.%d\n", token.id, token.value);
    Variable * var = &vars[token.id];
    if (var->value >= 0) {
      if (var->value == token.value) continue;
      return 0.0; // variable does not have free value
    }

    stack_push(assigned, token);
    var->value = token.value;
    var->decision = decision;
    if (var->quantifier.randomize) {
      path *= var->prob[token.value];
      if (show) printf("path: %f\n", path);
    } else {
      sat_eliminate_pure_literal(vars, clauses, propagate, token);
    }

    index_t * clauses_active = var->actives[token.value];
    index_t nactive = var->nactive[token.value];
    for (int i = 0; i < nactive ; i++) {
      index_t c = clauses_active[i];
      Clause * clause = &clauses[c];
      clause->nsatisfied++;
    }

    for (int w = 1; w < var->nvalue; w++) {
      index_t literal = (var->value + w) % var->nvalue;

      array_t * watch = &var->watch[literal];
      for (int l = watch->length-1; l >= 0; l--) {
        index_t c = watch->data[l];
        Clause * clause = &clauses[c];
        token_t w2token = clause->token[clause->w2];

        if (clause->nsatisfied > 0) continue;
        if (w2token.id == token.id)
          swap(&clause->w1, &clause->w2); // postcondition: w1 is token.id

        bool found_replacement = false;
        for (uint t = 1; t <= clause->ntoken; t++) {
          index_t w1 = (clause->w1 + t) % clause->ntoken;
          token_t token_test = clause->token[w1];
          Variable * var_test = &vars[token_test.id];

          if (w1 == clause->w2) continue; // skip over other watch literal
          if (var_test->value < 0) { // variable is free -- found new watch literal
            if (show) printf("clause[%d]: --%d.%d => ++%d.%d\n", c, token.id, literal, token_test.id, token_test.value);
            clause->w1 = w1;
            remove_watch_literal(watch, c); // remove 'c' from 'value' list
            add_watch_literal(&var_test->watch[token_test.value], c); // add 'c' to 'cvalue' list
            found_replacement = true;
            break;
          }
        }

        if (!found_replacement) {
          w2token = clause->token[clause->w2];
          Variable * w2var = &vars[w2token.id];
          if (show) printf("clause[%d]: no replacement found %d.%d\n", c, w2token.id, w2token.value);
          if (!w2var->quantifier.universal && w2var->value < 0) { // consider as unit
            if (show) printf("clause[%d]: unit implication => %d.%d\n", c, w2token.id, w2token.value);
            w2var->antecedent = c;
            stack_push(propagate, w2token);
          } else if (w2var->quantifier.universal || (w2var->value >= 0 && w2var->value != w2token.value)) { /// conflict occurred
            if (show) printf("clause[%d]: conflict => %d.%d\n", c, w2token.id, w2token.value);
            return 0.0;
          }
        }
      }
    }
  }
  return path;
}

inline void sat_eliminate_pure_literal(Variable * vars, Clause * clauses, Stack * propagate, token_t assign) {
  Variable * var = &vars[assign.id];
  index_t nactive_assign = var->nactive[assign.value];
  index_t * actives_assign = var->actives[assign.value];
  for (int i = 0; i < nactive_assign; i++) {
    index_t c = actives_assign[i];
    Clause * assign_clause = &clauses[c];
    // 'assign_clause' is satisfied before current assignment
    if (assign_clause->nsatisfied > 0) continue;

    index_t assign_clause_ntoken = assign_clause->ntoken;
    token_t * assign_clause_tokens = assign_clause->token;
    for (int j = 0; j < assign_clause_ntoken; j++) {
      token_t token = assign_clause_tokens[j]; 
      Variable * var_in_clause = &vars[token.id];
      // skip variables in clause that are already assigned
      if (var_in_clause->value >= 0 || var_in_clause->quantifier.randomize) continue;

      // find clauses being watch
      witness_t w1 = var_in_clause->w1;
      witness_t w2 = var_in_clause->w2;

      // some variable's value has no witness
      if (w1.index < 0 || w2.index < 0) continue;
      index_t clause1_id = var_in_clause->actives[w1.index][w1.clause];
      index_t clause2_id = var_in_clause->actives[w2.index][w2.clause];

      // skip variables that is not watching 'assign_clause'
      if (clause1_id != c && clause2_id != c) continue;

      // afterwords w1 is watching clause 'c' for sure
      if (clause2_id == c) {
        witness_swap(&w1, &w2);
        swap(&clause1_id, &clause2_id);
        witness_swap(&var_in_clause->w1, &var_in_clause->w2);
      }

      bool replacement = false;
      index_t nvalue_clause = var_in_clause->nvalue;
      for (int j = 0; !replacement && j < nvalue_clause; j++) {
        index_t literal = (w1.index + j) % nvalue_clause;
        if (literal == w2.index) continue; // replacement cannot be the same literal as w2
        //index_t literal = 1 - w1.index; // assume binary variable

        index_t nactive_in_clause = var_in_clause->nactive[literal];
        index_t * actives_in_clause = var_in_clause->actives[literal];
        for (int k = 0; k < nactive_in_clause; k++) {
          index_t cr = actives_in_clause[k];
          Clause * replacement_clause = &clauses[cr];
          if (replacement_clause->nsatisfied > 0) continue;

          //var_in_clause->w1 = (witness_t) {cr, literal};
          var_in_clause->w1.index = literal;
          var_in_clause->w1.clause = cr;
          replacement = true;
          break;
        }
      }

      if (!replacement) {
        Clause * clause2 = &clauses[clause2_id];
        if (clause2->nsatisfied == 0) {
          if (var_in_clause->quantifier.exist)
            stack_push(propagate, (token_t) {var_in_clause->id, w2.index});
          else if (var_in_clause->quantifier.universal)
            stack_push(propagate, (token_t) {var_in_clause->id, w1.index});
        }
      }
    }
  }
}

void component_clone(Component * c, Component * e) {
  *e = *c;
  e->vars = malloc(c->nvar * sizeof(index_t));
  memcpy(e->vars, c->vars, c->nvar * sizeof(index_t));

  e->clauses = malloc(c->nclause* sizeof(index_t));
  memcpy(e->clauses, c->clauses, c->nclause * sizeof(index_t));
}

real_t sat_solve_component(Sat * sat, Component * problem, index_t decision, index_t problem_id, index_t choice) {
  const int show = false;
  index_t vars_buffer[problem->nvar];
  index_t visited = ++sat->option.nvisit;
  real_t bound = 1.0;

  Variable * vars = sat->vars;
  Clause * clauses = sat->clauses;
  for (int v = 0; v < problem->nvar; v++) {
    index_t id = problem->vars[v];
    if (vars[id].value >= 0 || vars[id].visited >= visited) continue;
    Component component = {0, 0, 0, 0.0, vars_buffer, problem->clauses};
    component_connected(&component, vars, clauses, visited, id);

    if (component.nclause == 0) continue;
    for (int j = 0; j < component.nvar; j++) {
      int id0 = component.vars[0];
      int idj = component.vars[j];
      if (variable_compare(vars, id0, idj)) {
        component.vars[0] = idj;
        component.vars[j] = id0;
      }
    }

    Canonical * found = NULL;
    canonical_generate(sat->canonical, &component, vars, clauses, visited);
    uint hash = canonical_hash(sat->canonical);
    int component_id = -1;
    int nvar_upper = 0.9 * sat->nvar;
    int nvar_lower = 1 + 0.01 * sat->nvar;
    if ((found = cache_get(sat->cache, hash)) && canonical_equal(sat->canonical, found)) {
      sat->option.ncache_hit++;
      bound *= found->bound;
      component_id = found->id;

    } else {
      Canonical * entry = canonical_copy(sat->canonical);
      entry->id = sat->option.ncomponent++;
      real_t path = sat_solve_prime(sat, &component, decision, entry->id);
      bound *= path;

      entry->bound = path;
      uint nVC = entry->nvar + entry->nclause;
      uint memory = nVC*2 + (entry->ntoken + entry->ntoken_present) + (nVC + entry->ntoken_present);
      cache_put(sat->cache, hash, memory, entry);
      component_id = entry->id;

    }

    if (bound <= 0.0) break;
  }
  return bound;
}

real_t sat_solve_prime(Sat * sat, Component * component, index_t decision, index_t component_id) {
  const int show = false;
  Stack * assigned = sat->assigned;
  Variable * var = &sat->vars[component->vars[0]]; //decide_next_variable(sat, index);
  sat->option.ndecision++;

  const Option option = sat->option;
  const int save_solution = option.save_solution;
  index_t start = stack_size(assigned);
  token_t conflict;
  real_t result = 0.0;
  if (var->quantifier.exist) {

    index_t nvalue = var->nvalue;
    for (int i = 0; i < nvalue; i++) {
      int v = i;
      if (var->values[v] < 0) continue;
      index_t antecedent = -1;;
      real_t path = sat_constraint_propagate(sat, decision, (token_t){var->id, v});
      if (show) printf("%*stryE %d.%d: %e\n", (decision-1)*3, "", var->id, v, path);

      if (path > result) {
        real_t prob = sat_solve_component(sat, component, decision + 1, component_id, v);
        real_t path_prob = path * prob;
        if (path_prob > result) {
          result = path_prob;
        }
        var->score[v] = prob;
      }

      unassign(sat, start);
    }

  } else if (var->quantifier.universal) {

    index_t nvalue = var->nvalue;
    for (int i = 0; i < nvalue; i++) {
      int v = i;
      if (var->values[v] < 0) continue;
      real_t path = sat_constraint_propagate(sat, decision, (token_t){var->id, v});
      if (show) printf("%*stryU %d.%d: %e\n", (decision-1)*3, "", var->id, v, path);

      if (path > 0) {
        real_t prob = sat_solve_component(sat, component, decision + 1, component_id, v);
        real_t path_prob = path * prob;
        if (result <= 0 || path_prob < result) {
          result = path_prob;
        }
        var->score[v] = prob;
      }

      unassign(sat, start);
      if (result <= 0) break; // short-circuit on best action;
    }

  } else if (var->quantifier.randomize) {

    float * distr = var->prob;
    index_t nvalue = var->nvalue;
    for (int i = 0; i < nvalue; i++) {
      int v = i;
      if (var->values[v] < 0) continue;
      real_t path = sat_constraint_propagate(sat, decision, (token_t){var->id, v});
      if (show) printf("%*stryR %d.%d: %e\n", (decision-1)*3, "", var->id, v, path);

      if (path > 0) {
        path *= sat_solve_component(sat, component, decision + 1, component_id, v);
        result += path;
      }

      unassign(sat, start);
    }

  }
  return result;
}

void free_member(void * arg) {
  Canonical * c = arg;
  canonical_free(c);
}

real_t sat_solve(Sat * sat, double * time) {
  sat->option.ndecision = 0;
  sat->error = 0*pow(10, -3);
  Stack assigned;
  stack_init(&assigned, sat->nvalue);
  sat->assigned = &assigned;

  size_t capacity = 200000;
  size_t memory = 111200000;
  sat->cache = cache_init(capacity, memory, &free_member);

  List satisfy;
  list_init(&satisfy, sat->nclause);
  sat->satisfy = &satisfy ;

  Stack propagate;
  stack_init(&propagate, sat->nvalue);
  sat->propagate = &propagate ;


  clock_t start = clock();
  signal(SIGALRM, signal_handler);
  alarm(max_solver_duration);

  index_t decision = 0;
  token_t output = (token_t) {sat->output, 1};
  real_t bound = sat_constraint_propagate(sat, decision, output);
  index_t clauses_id[sat->nclause];
  for (int i = 0; i < sat->nclause; i++) {
    sat->clauses[i].visited = 0;
    clauses_id[i] = i;
  }

  decision = 1;
  Component component = (Component) {sat->nvar-1, sat->nclause, sat->ntoken, 0.0, sat->order + 1, clauses_id};
  Canonical * canonical = canonical_init(component.nvar, component.nclause, component.ntoken, component.ntoken);
  canonical->choice = output.value;
  sat->canonical = canonical;

  if (bound > 0.0) bound *= sat_solve_component(sat, &component, decision, sat->option.ncomponent++, canonical->choice);
  unassign(sat, 0);

  clock_t end = clock();
  *time = ((end - start) * 1000 / CLOCKS_PER_SEC)/1000.0;
  *time *= timeout ? -1 : 1;
  timeout = false;
  alarm(0);

  cache_free(sat->cache);
  canonical_free(sat->canonical);
  stack_free(&assigned);
  stack_free(&propagate);
  list_free(&satisfy);
  return bound;
}
