#include "component.h"

#ifndef _PRIMEH_
#define _PRIMEH_

real_t sat_solve(Sat * sat, double * time);
real_t sat_constraint_propagate(Sat * sat, index_t decision, token_t token0);
void sat_eliminate_pure_literal(Variable * vars, Clause * clauses, Stack * propagate, token_t assign);

real_t sat_solve_component(Sat * sat, Component * component, index_t decision, index_t component_id, index_t choice);
real_t sat_solve_prime(Sat * sat, Component * component, index_t decision, index_t component_id);

bool clause_satisfied(Clause * clause, List * satisfy, index_t c);

#endif
