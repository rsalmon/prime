#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include "basic.h"
#include "sat.h"
#include "prime.h"
#include "parse.h"

int sat_from_file(FILE * cnf, Sat * sat) {
  if (cnf == NULL) {
    printf("input stream does not exists\n");
    exit(EXIT_FAILURE);
  }

  char buffer[BUFF_SIZE];
  if (nextStringToken(buffer, cnf, "#") < 0) return -1;
  if (nextStringToken(buffer, cnf, "ssat") < 0) return -1;

  float scale = 1.0;
  index_t nvar, nclause;
  if (nextIntToken(buffer, cnf, &nvar) < 0) return -1;
  if (nextIntToken(buffer, cnf, &nclause) < 0) return -1;
  if (nextFloatToken(buffer, cnf, &sat->scale) < 0) return -1;
  //printf("%d %d %f\n", nvar, nclause, sat->scale);

  // SECTION: INIT VARIABLES
  sat->output = 0;
  sat->nvar = nvar + 1;
  sat->ntoken = 0;
  sat->nvalue = 0;
  sat->vars = malloc(sat->nvar * sizeof(Variable));
  sat->order = malloc(sat->nvar * sizeof(Variable*)); // exclude output var

  // SECTION: VARIABLES
  quantifier_t previous = {0,0,0,0,0};
  for (int v = 1; v <= nvar; v++) {
    index_t id, nvalue;

    // read var info
    if (nextToken(buffer, cnf) < 0) return -1;
    char q = buffer[0];
    if (nextIntToken(buffer, cnf, &id) < 0) return -1;
    if (nextIntToken(buffer, cnf, &nvalue) < 0) return -1;

    //printf("var[%d]: %d %d %c\n", v, id, nvalue, tolower(q));
    sat->order[v] = id;
    quantifier_t type = {0,0,0,0,0};
    float sum = 0.0;
    float * prob = NULL;
    switch(tolower(q)) {
      case 'e': type.exist = 1; break;
      case 'a': type.universal = 1; break;
      case 'r': type.randomize = 1;
        prob = malloc(nvalue * sizeof(float));
        for (int i = 0; i < nvalue; i++) {
          if (nextFloatToken(buffer, cnf, &prob[i]) < 0) return -1;
          sum += prob[i];
        }
        break;
      default: return -1;
    }

    if (!quantifier_equal(&previous, &type)) {
      previous = type;
      sat->nblock++;
    }

    sat->nvalue += nvalue;
    Variable * var = &sat->vars[id];
    init_var_default(var, id, nvalue, type, sat->nblock);
    var->prob = prob;
    var->sum = sum;
  }
  sat->order[0] = sat->output;
  init_var_default(&sat->vars[sat->output], sat->output, 2, (quantifier_t) {1,0,0,0,0}, 0);
  sat->nvalue += 2;

  // SECTION: CLAUSES
  sat->nclause = 0;
  sat->clauses = malloc((nclause + 1) * sizeof(Clause));
  if (nclause > 0) {
    token_t stack[sat->nvalue];
    for (int c = 0; c < nclause; c++) {
      index_t ntoken = 0;
      bool skip_clause = false;
      while (nextStringToken(buffer, cnf, "0") < 0) {
        token_t token = read_token(buffer);
        Variable * var = &sat->vars[abs(token.id)];
        bool randomize = var->quantifier.randomize;

        if (token.id >= 0) {
          if (!randomize || var->prob[token.value] > 0.0)
            stack[ntoken++] = token;

          // skip clause -- already satisfied
          skip_clause = skip_clause || randomize && var->prob[token.value] >= var->sum;
        } else {
          index_t n = sat->vars[-token.id].nvalue;
          for (int i = 0; i < n; i++)
            if (i != token.value && (!randomize || var->prob[i] > 0.0)) {
              stack[ntoken++] = (token_t) {-token.id, i};
              skip_clause = skip_clause || randomize && var->prob[i] <= 0;
            }
        }
      }

      if (skip_clause) continue;
      else if (ntoken == 1) {
        ntoken = 2;
        token_t * tokens = malloc(ntoken * sizeof(token_t));
        tokens[0] = stack[0];
        tokens[1] = (token_t) {sat->output, 0};
        sat->clauses[sat->nclause++] = init_clause(ntoken, tokens);
        sat->ntoken += ntoken;
      } else {
        token_t * tokens = malloc(ntoken * sizeof(token_t));
        for (int l = 0; l < ntoken; l++)
          tokens[l] = stack[l];
        sat->clauses[sat->nclause++] = init_clause(ntoken, tokens);
        sat->ntoken += ntoken;
      }
    }
  }

  index_t ntoken = 2;
  token_t * token = malloc(2 * sizeof(token_t));
  token[0] = (token_t) {sat->output, 0};
  token[1] = (token_t) {sat->output, 1};
  sat->clauses[sat->nclause++] = init_clause(ntoken, token);
  sat->ntoken += ntoken;

  var_count_literals(sat->vars, sat->clauses, sat->nvar, sat->nclause);
  return 0;
}

real_t (*solver) (Sat * sat, double * time) = NULL;
int main(int argc, char* argv[]) {
  double time;
  setlocale(LC_ALL, ""); // use user selected locale

  Sat sat = init_sat();
  FILE * input = stdin;
  int err = 0;
  for (int i = argc-1; i > 0; i--) {
    // choose format (3)
    if (strncmp(argv[i], "cnf", 3) == 0) {
      sat_from_file(input, &sat);

    // choose options for solve (2)
    } else if (strncmp(argv[i], "learn", 5) == 0) {
      sat.option.clause_learning = true;
    } else if (strncmp(argv[i], "pure", 4) == 0) {
      sat.option.pure_literal = true;
    } else if (strncmp(argv[i], "subproblem", 10) == 0) {
      sat.option.sub_problem = true;
    } else if (strncmp(argv[i], "cache", 5) == 0) {
      sat.option.cache = true;

    // choose solver (1)
    } else if (strncmp(argv[i], "prime", 5) == 0) {
      solver = &sat_solve;
    } else if (strncmp(argv[i], "solve", 6) == 0) {
      real_t bound = solver(&sat, &time) * sat.scale;
      printf("%e, %d, %d, %e\n", bound, sat.option.ndecision, sat.option.ncache_hit, time);

    // show some statistic (2)
    } else if (strncmp(argv[i], "clause", 6) == 0) {
      show_clause(sat.vars, sat.clauses, sat.nclause);
    } else if (strncmp(argv[i], "var", 3) == 0) {
      show_var(sat.vars, sat.nvar);
    } else if (strncmp(argv[i], "show", 4) == 0) {
      show_var(sat.vars, sat.nvar);
      show_clause(sat.vars, sat.clauses, sat.nclause);

    } else if (strncmp(argv[i], "out", 3) == 0) {
      char buffer[1024];
      while (fgets(buffer, sizeof(buffer), input))
        fprintf(stdout, "%s", buffer);
      rewind(input);
      //fclose(input);
    }

    if (err < 0) {
      printf("Error return code: %d\n", err);
      exit(err);
    }
  }
  sat_free(&sat);
  return err;
}
