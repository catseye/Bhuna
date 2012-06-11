#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "value.h"
#include "list.h"
#include "dict.h"
#include "closure.h"
#include "activation.h"
#include "type.h"

/*
 * Built-in operations.
 */

struct builtin builtins[] = {
	{"Print",	builtin_print,	btype_print,		-1, 0, 1, 0},
	{"!",		builtin_not,	btype_unary_logic,	 1, 1, 1, 1},
	{"&",		builtin_and,	btype_binary_logic,	 2, 1, 1, 2},
	{"|",		builtin_or,	btype_binary_logic,	 2, 1, 1, 3},
	{"=",		builtin_equ,	btype_compare,		 2, 1, 1, 4},
	{"!=",		builtin_neq,	btype_compare,		 2, 1, 1, 5},
	{">",		builtin_gt,	btype_compare,		 2, 1, 1, 6},
	{"<",		builtin_lt,	btype_compare,		 2, 1, 1, 7},
	{">=",		builtin_gte,	btype_compare,		 2, 1, 1, 8},
	{"<=",		builtin_lte,	btype_compare,		 2, 1, 1, 9},
	{"+",		builtin_add,	btype_arith,		 2, 1, 1, 10},
	{"-",		builtin_sub,	btype_arith,		 2, 1, 1, 11},
	{"*",		builtin_mul,	btype_arith,		 2, 1, 1, 12},
	{"/",		builtin_div,	btype_arith,		 2, 1, 1, 13},
	{"%",		builtin_mod,	btype_arith,		 2, 1, 1, 14},
	{"List",	builtin_list,	btype_list,		-1, 1, 1, 15},
	{"Fetch",	builtin_fetch,	btype_fetch,		 2, 1, 1, 16},
	{"Store",	builtin_store,	btype_store,		 3, 0, 1, 17},
	{"Dict",	builtin_dict,	btype_dict,		-1, 1, 1, 18},
	{NULL,		NULL,		NULL,			 0, 0, 0, 19}
};

void
builtin_print(struct activation *ar, struct value **q)
{
	int i;
	/*struct list *l;*/
	struct value *v = NULL;

	for (i = 0; i < ar->size; i++) {
		v = activation_get_value(ar, i, 0);
		if (v == NULL) {
			printf("(null)");
			continue;
		}

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
			/*
			printf("[");
			for (l = v->v.l; l != NULL; l = l->next) {
			*/
				
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
builtin_list(struct activation *ar, struct value **v)
{
	int i;
	struct value *x = NULL;

	value_set_list(v);

	for (i = ar->size - 1; i >= 0; i--) {
		x = activation_get_value(ar, i, 0);
		value_list_append(v, x);
		value_release(x);
	}
}

void
builtin_fetch(struct activation *ar, struct value **v)
{
	struct value *l = activation_get_value(ar, 0, 0);
	struct value *r = activation_get_value(ar, 1, 0);
	struct value *q;
	int count;
	struct list *li;

	if (l->type == VALUE_CLOSURE && r->type == VALUE_INTEGER) {
		int i = r->v.i - 1;
		/*
		 * This is _EVIL_!
		 */
		if (i >= 0 && i < l->v.k->ar->size) {
			q = activation_get_value(l->v.k->ar, i, 0);
			value_set_from_value(v, q);
		} else {
			value_set_error(v, "out of bounds");
		}
	} else if (l->type == VALUE_DICT) {
		q = dict_fetch(l->v.d, r);
		value_set_from_value(v, q);
		value_release(q);
	} else if (l->type == VALUE_LIST && r->type == VALUE_INTEGER) {
		li = l->v.l;
		for (count = 1; li != NULL && count < r->v.i; count++)
			li = li->next;
		if (li == NULL)
			return(value_set_error(v, "out of bounds"));
		else {
			value_set_from_value(v, li->value);
		}
	} else {
		return(value_set_error(v, "type mismatch"));
	}
}

void
builtin_store(struct activation *ar, struct value **v)
{
	struct value *d = activation_get_value(ar, 0, 0);
	struct value *i = activation_get_value(ar, 1, 0);
	struct value *p = activation_get_value(ar, 2, 0);
	int count;
	struct list *li;

	if (d->type == VALUE_DICT) {
		dict_store(d->v.d, i, p);
		value_set_from_value(v, d);
	} else if (d->type == VALUE_LIST && i->type == VALUE_INTEGER) {
		li = d->v.l;
		for (count = 1; li != NULL && count < i->v.i; count++)
			li = li->next;
		if (li == NULL)
			value_set_error(v, "no such element");
		else {
			value_set_from_value(&li->value, p);
			value_set_from_value(v, d);
		}
	} else {
		value_set_error(v, "type mismatch");
	}
}

void
builtin_dict(struct activation *ar, struct value **v)
{
	int i;
	struct value *key = NULL, *val = NULL;

	value_set_dict(v);

	if (ar->size % 2 != 0) {
		value_set_error(v, "number of argument must be even");
	} else {
		for (i = 0; i < ar->size; i += 2) {
			key = activation_get_value(ar, i, 0);
			val = activation_get_value(ar, i + 1, 0);
			value_dict_store(v, key, val);
			value_release(key);
			value_release(val);
		}
	}
}

struct type *
btype_print(void)
{
	return(
	  type_new_closure(
	    type_new_var(1),
	    type_new(TYPE_VOID)
	  )
	);
}

struct type *
btype_unary_logic(void)
{
	return(
	  type_new_closure(
	    type_new(TYPE_BOOLEAN),
	    type_new(TYPE_BOOLEAN)
	  )
	);
}

struct type *
btype_binary_logic(void)
{
	return(
	  type_new_closure(
	    type_new_arg(type_new(TYPE_BOOLEAN), type_new(TYPE_BOOLEAN)),
	    type_new(TYPE_BOOLEAN)
	  )
	);
}

struct type *
btype_compare(void)
{
	return(
	  type_new_closure(
	    type_new_arg(type_new(TYPE_INTEGER), type_new(TYPE_INTEGER)),
	    type_new(TYPE_BOOLEAN)
	  )
	);
}

struct type *
btype_arith(void)
{
	return(
	  type_new_closure(
	    type_new_arg(type_new(TYPE_INTEGER), type_new(TYPE_INTEGER)),
	    type_new(TYPE_INTEGER)
	  )
	);
}

struct type *
btype_list(void)
{
	return(
	  type_new_closure(
	    type_new_var(2),
	    type_new_list(type_new(TYPE_INTEGER))
	  )
	);
}

struct type *
btype_fetch(void)
{
	return(
	  type_new_closure(
	    type_new_arg(type_new_var(5), type_new(TYPE_INTEGER)),
	    type_new_var(5)
	  )
	);
}

struct type *
btype_store(void)
{
	return(
	  type_new_closure(
	    type_new_arg(
		type_new_var(6),
		type_new_arg(type_new_var(7), type_new_var(8))
	    ),
	    type_new(TYPE_VOID)
	  )
	);
}

struct type *
btype_dict(void)
{
	return(NULL);
}
