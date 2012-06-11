#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "value.h"
#include "list.h"
#include "closure.h"

#include "symbol.h"

/*
 * Built-in operations.
 * Each of these allocates a new value and returns it,
 * leaving the passed values untouched.
 * The single parameter `arg' is always a value of type VALUE_LIST.
 */

struct builtin_desc builtins[] = {
	{"Print",	builtin_print},
	{"Return",	builtin_return},
	{"!",		builtin_not},
	{"&",		builtin_and},
	{"|",		builtin_or},
	{"=",		builtin_equ},
	{"!=",		builtin_neq},
	{">",		builtin_gt},
	{"<",		builtin_lt},
	{">=",		builtin_gte},
	{"<=",		builtin_lte},
	{"+",		builtin_add},
	{"-",		builtin_sub},
	{"*",		builtin_mul},
	{"/",		builtin_div},
	{"%",		builtin_mod},
	{"Index",	builtin_index},
	{NULL,		NULL}
};

void
builtin_return(struct value **arg)
{
	/* Deref the list. */
	value_set_from_value(arg, (*arg)->v.l->value);
}

void
builtin_print(struct value **arg)
{
	struct list *l;
	struct value *v;

	for (l = (*arg)->v.l; l != NULL; l = l->next) {
		v = l->value;
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
			builtin_print(&v);
			break;
		case VALUE_STAB:
			symbol_table_dump(v->v.stab, 1);
			break;
		case VALUE_ERROR:
			printf("#ERR<%s>", v->v.e);
			break;
		case VALUE_BUILTIN:
			printf("#FN<%08lx>", (unsigned long)v->v.f);
			break;
		case VALUE_CLOSURE:
			closure_dump(v->v.k);
			break;
		case VALUE_SYMBOL:
			symbol_dump(v->v.sym, 0);
			break;
		default:
			printf("???unknown(%d)???", v->type);
			break;
		}
	}
}

/*** logical ***/

void
builtin_not(struct value **arg)
{
	struct value *v;

	v = (*arg)->v.l->value;

	if (v->type == VALUE_BOOLEAN) {
		value_set_boolean(arg, !v->v.b);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_and(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
		value_set_boolean(arg, l->v.b && r->v.b);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_or(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
		value_set_boolean(arg, l->v.b || r->v.b);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

/*** comparison ***/

void
builtin_equ(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		value_set_boolean(arg, l->v.i == r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_neq(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		value_set_boolean(arg, l->v.i != r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_gt(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		value_set_boolean(arg, l->v.i > r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_lt(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		value_set_boolean(arg, l->v.i < r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_gte(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		value_set_boolean(arg, l->v.i >= r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_lte(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		value_set_boolean(arg, l->v.i <= r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

/*** arithmetic ***/

void
builtin_add(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		value_set_integer(arg, l->v.i + r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_mul(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		value_set_integer(arg, l->v.i * r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_sub(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		value_set_integer(arg, l->v.i - r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_div(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		if (r->v.i == 0)
			value_set_error(arg, "division by zero");
		else
			value_set_integer(arg, l->v.i / r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

void
builtin_mod(struct value **arg)
{
	struct value *l, *r;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
		if (r->v.i == 0)
			value_set_error(arg, "modulo by zero");
		else
			value_set_integer(arg, l->v.i % r->v.i);
	} else {
		value_set_error(arg, "type mismatch");
	}
}

/*** list ***/

void
builtin_index(struct value **arg)
{
	struct value *l, *r;
	int count;
	struct list *li;

	l = (*arg)->v.l->value;
	r = (*arg)->v.l->next->value;

	if (l->type == VALUE_LIST && r->type == VALUE_INTEGER) {
		li = l->v.l;
		for (count = 1; l != NULL && count < r->v.i; count++)
			li = li->next;
		if (li == NULL)
			value_set_error(arg, "no such element");
		else
			value_set_from_value(arg, li->value);
	} else {
		value_set_error(arg, "type mismatch");
	}
}
