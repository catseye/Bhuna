#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "value.h"
#include "list.h"
#include "dict.h"
#include "closure.h"
#include "activation.h"

/*
 * Built-in operations.
 */

struct builtin builtins[] = {
	{"Print",	builtin_print,	1,	0,	1,	0},
	{"!",		builtin_not,	1,	1,	1,	1},
	{"&",		builtin_and,	2,	1,	1,	2},
	{"|",		builtin_or,	2,	1,	1,	3},
	{"=",		builtin_equ,	2,	1,	1,	4},
	{"!=",		builtin_neq,	2,	1,	1,	5},
	{">",		builtin_gt,	2,	1,	1,	6},
	{"<",		builtin_lt,	2,	1,	1,	7},
	{">=",		builtin_gte,	2,	1,	1,	8},
	{"<=",		builtin_lte,	2,	1,	1,	9},
	{"+",		builtin_add,	2,	1,	1,	10},
	{"-",		builtin_sub,	2,	1,	1,	11},
	{"*",		builtin_mul,	2,	1,	1,	12},
	{"/",		builtin_div,	2,	1,	1,	13},
	{"%",		builtin_mod,	2,	1,	1,	14},
	{"Cons",	builtin_cons,	2,	1,	1,	15},
	{"Index",	builtin_index,	2,	1,	1,	16},
	{NULL,		NULL,		0,	0,	0,	17}
};

void
builtin_print(struct activation *ar, struct value **q)
{
	struct value *v = activation_get_value(ar, 0, 0);

	switch (v->type) {
	case VALUE_INTEGER:
		printf("%d", v->v.i);
		break;
	case VALUE_BOOLEAN:
		printf("%s", v->v.b ? "true" : "false");
		break;
	case VALUE_STRING:
		printf("%s", v->v.s);
		break;
	case VALUE_LIST:
		list_dump(v->v.l);
		break;
	case VALUE_ERROR:
		printf("#ERR<%s>", v->v.e);
		break;
	case VALUE_BUILTIN:
		printf("#BIF<%08lx>", (unsigned long)v->v.bi);
		break;
	case VALUE_CLOSURE:
		closure_dump(v->v.k);
		break;
	case VALUE_DICT:
		dict_dump(v->v.d);
		break;
	default:
		printf("???unknown(%d)???", v->type);
		break;
	}

	value_set_from_value(q, v);
}

/*** logical ***/

void
builtin_not(struct activation *ar, struct value **v)
{
	struct value *q = activation_get_value(ar, 0, 0);

	if (q->type == VALUE_BOOLEAN) {
		return(value_set_boolean(v, !q->v.b));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_and(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
		return(value_set_boolean(v, l->v.b && r->v.b));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_or(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
		return(value_set_boolean(v, l->v.b || r->v.b));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

/*** comparison ***/

void
builtin_equ(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		return(value_set_boolean(v, l->v.i == r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_neq(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		return(value_set_boolean(v, l->v.i != r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_gt(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		return(value_set_boolean(v, l->v.i > r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_lt(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		return(value_set_boolean(v, l->v.i < r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_gte(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		return(value_set_boolean(v, l->v.i >= r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_lte(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		return(value_set_boolean(v, l->v.i <= r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

/*** arithmetic ***/

void
builtin_add(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		return(value_set_integer(v, l->v.i + r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_mul(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

#if 0
	printf("IN MUL, L = ");
	value_print(l);
	printf("  R = ");
	value_print(r);
	printf("\n");
#endif

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		return(value_set_integer(v, l->v.i * r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_sub(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		return(value_set_integer(v, l->v.i - r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_div(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		if (r->v.i == 0)
			return(value_set_error(v, "division by zero"));
		else
			return(value_set_integer(v, l->v.i / r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_mod(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		if (r->v.i == 0)
			return(value_set_error(v, "modulo by zero"));
		else
			return(value_set_integer(v, l->v.i % r->v.i));
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

/*** list ***/

void
builtin_cons(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);

	if (l->type == VALUE_LIST) {
		list_cons(&l->v.l, r);
		value_set_from_value(v, l);
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_index(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);
	int count;
	struct list *li;

	if (l->type == VALUE_LIST && r->type == VALUE_INTEGER) {
		li = l->v.l;
		for (count = 1; l != NULL && count < r->v.i; count++)
			li = li->next;
		if (li == NULL)
			return(value_set_error(v, "no such element"));
		else {
			value_set_from_value(v, li->value);
		}
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}
