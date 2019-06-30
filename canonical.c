#include "sat.h"
#include "basic.h"
#include "canonical.h"
#include "prime.h"
#include <stdlib.h>
#include <string.h>

float hash_unfloat(uint x) {
  return *(float*)&x;
}

uint hash_float(float x) {
  return *(unsigned*)&x;
}

void canonical_show(Canonical * c) {
  printf("canonical: %d %d %d %d %d\n", c->id, c->nvar, c->nclause, c->ntoken, c->ntoken_present);
  for (int i = c->nclause; i < c->nclause + c->nvar; i++) {
    uint k = c->color[i];
    index_t order = k >> 24;
    index_t type = 0x00ffffff & k;
    printf("%x %d", k, c->node_size[i]);

    for (int j = 0; j < c->node_size[i];j ++) {
      uint n = c->node_token[c->node_start[i] + j];
      printf(" %d", n);
    }
    if (type == 2) { // randomize
      for (int j = 0; j < c->node_size[i];j ++) {
        uint n = c->node_token[c->node_start[i] + j];
        printf(" %f", hash_unfloat(c->color[n]));
      }
    }
    printf("\n");
  }

  for (int i = 0; i < c->nclause; i++) {
    printf("%d %d", i, c->node_size[i]);
    for (int j = 0; j < c->node_size[i]; j++) {
      uint n = c->node_token[c->node_start[i] + j];
      printf(" %d", n);
    }
    printf("\n");
  }

  printf("\n");
}

uint canonical_hash(Canonical * c) {
  uint hash = 13;
  hash = hash_update(hash, c->nvar);
  hash = hash_update(hash, c->nclause);
  hash = hash_update(hash, c->ntoken);
  hash = hash_update(hash, c->ntoken_present);
  const index_t nvar_nclause = c->nvar + c->nclause;
  for (int i = 0; i < nvar_nclause; i++) {
    index_t start = c->node_start[i];
    index_t ntoken = c->node_size[i];
    hash = hash_update(hash, c->color[i]);
    hash = hash_update(hash, c->node_size[i]);
    for (int j = 0; j < ntoken; j++) {
      index_t index = start + j;
      index_t node = c->node_token[index];
      hash = hash_update(hash, c->color[node]);
    }
  }
  return hash;
}

int canonical_equal(Canonical * c1, Canonical * c2) {
  if (c1 == NULL || c2 == NULL)
    return false;

  if (c1->nvar != c2->nvar || c1->nclause != c2->nclause || c1->ntoken != c2->ntoken || c1->ntoken_present != c2->ntoken_present)
    return false;

  const index_t nvar_nclause = c1->nvar + c1->nclause;
  for (int i = 0; i < nvar_nclause; i++) {
    if (c1->color[i] != c2->color[i] || c1->node_size[i] != c2->node_size[i])
      return false;

    index_t ntoken = c1->node_size[i];
    for (int j = 0; j < ntoken; j++) {
      index_t index1 = c1->node_start[i] + j;
      index_t index2 = c2->node_start[i] + j;
      index_t node1 = c1->node_token[index1];
      index_t node2 = c2->node_token[index2];
      if (c1->color[node1] != c2->color[node2])
        return false;
    }
  }
  return true;
}

void sort_token(index_t * list, index_t start, index_t size) {
  index_t end = start + size;
  for (int i = start; i < end; i++) {
    for (int j = i + 1; j < end; j++) {
      if (list[j] < list[i]) {
        index_t swap = list[i];
        list[i] = list[j];
        list[j] = swap;
      }
    }
  }
}

void canonical_generate(Canonical * canonical, Component * component, Variable * vars, Clause * clauses, index_t visited) {
  index_t node_index = 0;
  canonical->id = 0;
  canonical->ntoken_present = 0;
  canonical->nvar = component->nvar;
  canonical->nclause = component->nclause;
  canonical->ntoken = component->ntoken;
  const index_t nvar_nclause = canonical->nvar + canonical->nclause;
  for (int c = 0; c < component->nclause; c++) {
    Clause * clause = &clauses[component->clauses[c]];
    canonical->node_start[c] = node_index;
    for (int l = 0; l < clause->ntoken; l++) {
      token_t token = clause->token[l];
      Variable * var = &vars[token.id];
      if (var->value >= 0) continue;
      else if (var->value_present[token.value] != visited) {
        var->value_present[token.value] = visited;
        var->value_index[token.value] = canonical->ntoken_present;
        var->value_list[var->nvalue_list++] = token.value;

        uint hash = 0;
        if (var->quantifier.exist)
          hash = 1;
        else if (var->quantifier.randomize)
          hash = hash_float(var->prob[token.value]);

        canonical->color[nvar_nclause + canonical->ntoken_present] = hash;
        canonical->ntoken_present++;
      }
      canonical->node_token[node_index++] = nvar_nclause + var->value_index[token.value];
    }
    canonical->node_size[c] = node_index - canonical->node_start[c];
    canonical->color[c] = canonical->node_size[c];
  }

  uint var_order = 0;
  index_t offset = component->nclause;
  quantifier_t * var_previous_quantifier;
  for (int i = 0; i < component->nvar; i++) {
    Variable * var = &vars[component->vars[i]];

    if (i > 0 && !quantifier_equal(var_previous_quantifier ,&var->quantifier))
      var_order++;
    uint type = var->quantifier.universal ? 0 : (var->quantifier.exist ? 1 : 2);
    var_previous_quantifier = &var->quantifier;

    canonical->color[offset + i] = ((var_order + 1) << 24) + type;
    int nvalue_list = var->nvalue_list;
    int * value_list = var->value_list;

    canonical->node_size[offset + i] = var->nvalue_list;
    canonical->node_start[offset + i] = node_index;
    for (int j = 0; j < nvalue_list; j++) {
      index_t index = var->value_index[var->value_list[j]];
      canonical->node_token[node_index + j] = nvar_nclause + index;
    }
    node_index += canonical->node_size[offset + i];
    var->nvalue_list = 0;
  }
}

Canonical * canonical_copy(Canonical * s) {
  Canonical * d = canonical_init(s->nvar, s->nclause, s->ntoken, s->ntoken_present);
  d->bound = s->bound;
  d->choice = s->choice;
  d->id = s->id;

  const index_t nvar_nclause = s->nvar + s->nclause;
  memcpy(d->node_size, s->node_size, nvar_nclause * sizeof(index_t));
  memcpy(d->node_start, s->node_start, nvar_nclause * sizeof(index_t));
  memcpy(d->node_token, s->node_token, (s->ntoken + s->ntoken_present) * sizeof(uint));
  memcpy(d->color, s->color, (nvar_nclause + s->ntoken_present) * sizeof(uint));
  return d;
}

Canonical * canonical_init(index_t nvar, index_t nclause, index_t ntoken, index_t npresent) {
  Canonical * c = malloc(sizeof(Canonical));
  c->nvar = nvar;
  c->nclause = nclause;
  c->ntoken = ntoken;
  c->ntoken_present = npresent;
  c->choice = -1;
  c->id = 0;

  c->node_size = malloc((nvar + nclause) * sizeof(index_t));
  c->node_start = malloc((nvar + nclause) * sizeof(index_t));
  c->node_token = malloc((ntoken + npresent) * sizeof(index_t));
  c->color = malloc((nvar + nclause + npresent) * sizeof(index_t));
  return c;
}

void canonical_free(Canonical * c) {
  free(c->node_size);
  free(c->node_start);
  free(c->node_token);
  free(c->color);
  free(c);
}
