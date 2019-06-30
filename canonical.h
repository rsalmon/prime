#ifndef _CANONICALH_
#define _CANONICALH_
#include "component.h"

typedef struct {
  index_t nvar, nclause, ntoken, ntoken_present, choice, id;
  real_t bound;

  index_t *node_size;
  index_t *node_start;
  index_t *node_token;
  uint *color;
} Canonical;

void canonical_generate(Canonical * canonical, Component * component, Variable * vars, Clause * clauses, index_t visited);
Canonical * canonical_init(index_t nvar, index_t nclause, index_t ntoken, index_t npresent);
Canonical * canonical_copy(Canonical * s);
void canonical_show(Canonical * component);
uint canonical_hash(Canonical * component);
int canonical_equal(Canonical * component1, Canonical * component2);
void canonical_free(Canonical * c);

#endif

