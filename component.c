#include "sat.h"
#include "basic.h"
#include "component.h"
#include "prime.h"
#include <stdlib.h>
#include <string.h>

inline uint hash_update(uint hash, uint value) {
  hash += value;
  hash += (hash << 10);
  hash ^= (hash >> 6);
  return hash;
}

uint component_hash(Component * c) {
  uint hash = 13;
  hash = hash_update(hash, c->nvar);
  for(int i = 0; i < c->nvar; ++i)
    hash = hash_update(hash, c->vars[i]);
  hash = hash_update(hash, c->nclause);
  for(int i = 0; i < c->nclause; ++i)
    hash = hash_update(hash, c->clauses[i]);
  return hash;
}

void component_show(Component * c) {
  printf("Component: %d %d\n", c->nvar, c->nclause);
  printf("V:");
  for (int i = 0; i < c->nvar; i++)
    printf(" %d", c->vars[i]);
  printf("\n");

  printf("C:");
  for (int i = 0; i < c->nclause; i++)
    printf(" %d", c->clauses[i]);
  printf("\n");
  printf("\n");
}

int component_equal(Component * component1, Component * component2) {
  if (component1 == NULL || component2 == NULL)
    return false;
  if (component1->nvar != component2->nvar || component1->nclause != component2->nclause)
    return false;
  for (int i = 0; i < component1->nvar; i++)
    if (component1->vars[i] != component2->vars[i])
      return false;
  for (int i = 0; i < component1->nclause; i++)
    if (component1->clauses[i] != component2->clauses[i])
      return false;
  return true;
}

void component_connected(Component * component, Variable * vars, Clause * clauses, int visited, int id) {
  component->vars[component->nvar++] = id;
  Variable * var = &vars[id];
  var->order = component->nvar-1;
  var->nvisit = 1;
  var->visited = visited;
  index_t nvalue = var->nvalue;
  for (int i = 0; i < nvalue; i++) {
    int nactive = var->nactive[i];
    int * actives = var->actives[i];
    for (int j = 0; j < nactive; j++) {
      int c = actives[j];
      Clause * clause = &clauses[c];
      if (clause->nsatisfied > 0 || clause->visited >= visited) continue;

      component->clauses[component->nclause++] = c;
      clause->visited = visited;
      index_t ntoken = clause->ntoken;
      token_t * tokens = clause->token;
      for (int l = 0; l < ntoken; l++) {
        token_t token = tokens[l];
        if (vars[token.id].value < 0) {
          component->ntoken++;
          if (vars[token.id].visited < visited)
            component_connected(component, vars, clauses, visited, token.id);
          else
            vars[token.id].nvisit++;
        }
      }
    }
  }
}

