#ifndef _COMPONENTH_
#define _COMPONENTH_

typedef struct {
  index_t nvar, nclause, ntoken;
  real_t bound;
  index_t * vars;
  index_t * clauses;
} Component;

uint hash_update(uint hash, uint value);
uint component_hash(Component * component);
int component_equal(Component * component1, Component * component2);
void component_connected(Component * component, Variable * vars, Clause * clauses, int visitor, int id);
void component_show(Component * c);

#endif
