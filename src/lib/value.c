/*
 * value.c
 * Values for Bhuna.
 * $Id$
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "value.h"
#include "ast.h"
#include "list.h"
#include "dict.h"
#include "closure.h"

#include "type.h"

#ifdef POOL_VALUES
#include "pool.h"
#endif

#ifdef DEBUG
extern int trace_valloc;
extern int num_vars_created;
extern int num_vars_cached;
extern int num_vars_freed;
#endif

struct value *v_head = NULL;
int v_count = 0;

/*
 * Hash cons'ing.
 * Not strictly hash consing, since this isn't LISP...
 * but very much in the style of the paper
 * _Implementing Functional Language with Fast Equality,
 * Sets, and Maps: an Exercise in Hash Consing_, Goubault, 1992
 */
#ifdef HASH_CONSING
struct hc_chain *hc_bucket[HASH_CONS_SIZE];
struct hc_chain *hc_c;
int hc_h;

void
hc_init(void)
{
	bzero(hc_bucket, HASH_CONS_SIZE * sizeof(struct hc_chain *));
}
#endif

void
value_dump_global_table(void)
{
#ifdef DEBUG
	struct value *v;
#ifdef HASH_CONSING
	struct hc_chain **hccp;
#endif

	printf("----- BEGIN Global Table of Values -----\n");
#ifdef HASH_CONSING
	for (hccp = hc_bucket; hccp - hc_bucket < HASH_CONS_SIZE; hccp++) {
		for (hc_c = *hccp; hc_c != NULL; hc_c = hc_c->next) {
			v = hc_c->v;
			printf("==> ");
			value_print(v);
			printf("\n");
		}
	}
#else
	for (v = v_head; v != NULL; v = v->next) {
		printf("==> ");
		value_print(v);
		printf("\n");
	}
#endif
	printf("----- END Global Table of Values -----\n");
#endif
}

/*** GENERIC CONSTRUCTOR ***/

static struct value *
value_new(unsigned short int type)
{
	struct value *v;

#ifdef POOL_VALUES
	v = value_allocate();
#else
	v = bhuna_malloc(sizeof(struct value));
#endif
	v->type = type;
	v->admin = 0;

	/*
	 * Link up to GC list.
	 */
	v->next = v_head;
	v_head = v;
	v_count++;

	v->refcount = 0;

#ifdef DEBUG
	if (trace_valloc > 0) {
		num_vars_created++;
	}
#endif

	return(v);
}

/*
 * Assert that this value should not be garbage-collected.
 * Typically used for values in the symbol table, vm program body, etc.
 */
void
value_deregister(struct value *v)
{
	v->admin |= ADMIN_PERMANENT;
}

/*** UNCONDITIONAL DUPLICATOR ***/

/*
 * Returns a deep(-ish) copy of the given value.
 * New strings (char arrays) are created when copying a string;
 * New list spines (struct list *) are created, but values are only grabbed, not dup'ed.
 * Some things are not copied, only the pointers to them.
 *
 * Note that the dup'ed value is 'new', i.e. it has a refcount of 1.
 */
struct value *
value_dup(struct value *v)
{
	struct value *n; /*  *z; */
	struct list *l;
	/* size_t i; */

	switch (v->type) {
	case VALUE_INTEGER:
		return(value_new_integer(v->v.i));
	case VALUE_BOOLEAN:
		return(value_new_boolean(v->v.b));
	case VALUE_STRING:
		return(value_new_string(v->v.s));
	case VALUE_LIST:
		n = value_new_list();
		for (l = v->v.l; l != NULL; l = l->next) {
			value_list_append(&n, l->value);
		}
		/*
		n = value_new(VALUE_LIST);
		n->v.l = list_dup(v->v.l);
		*/
		return(n);
	case VALUE_ERROR:
		return(value_new_error(v->v.e));
	case VALUE_BUILTIN:
		return(value_new_builtin(v->v.bi));
	case VALUE_CLOSURE:
		return(value_new_closure(v->v.k->ast, v->v.k->ar,
		    v->v.k->arity, v->v.k->locals, v->v.k->cc));
	case VALUE_DICT:
		n = value_new(VALUE_DICT);
		n->v.d = dict_dup(v->v.d);
		return(n);
	default:
		return(value_new_error("unknown type"));
	}
}

/*** DESTRUCTOR ***/

void
value_free(struct value *v)
{
	if (v == NULL)
		return;

	/*assert(v->refcount == 0);*/

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] freeing ");
		value_print(v);
		printf("\n");
	}
#endif

	switch (v->type) {
	case VALUE_LIST:
		list_free(&v->v.l);
		break;
	case VALUE_STRING:
		if (v->v.s != NULL)
			bhuna_free(v->v.s);
		break;
	case VALUE_ERROR:
		if (v->v.e != NULL)
			bhuna_free(v->v.e);
		break;
	case VALUE_CLOSURE:
		closure_free(v->v.k);
		break;
	case VALUE_DICT:
		dict_free(v->v.d);
		break;
	}

#ifdef DEBUG
	if (trace_valloc > 0) {
		num_vars_freed++;
	}
#endif

#ifdef POOL_VALUES
	value_deallocate(v);
#else
	bhuna_free(v);
#endif
	v_count--;
}

/*** SPECIFIC CONSTRUCTORS ***/

struct value *
value_new_integer(int i)
{
	struct value *v;

#ifdef HASH_CONSING
	hc_h = (i) % HASH_CONS_SIZE;
	for (hc_c = hc_bucket[hc_h]; hc_c != NULL; hc_c = hc_c->next) {
		if (hc_c->v->type == VALUE_INTEGER && hc_c->v->v.i == i) {
			/*hc_c->v->admin |= ADMIN_SHARED;*/
#ifdef DEBUG
			if (trace_valloc > 0) {
				num_vars_cached++;
			}
			if (trace_valloc > 1) {
				printf("[RC] Cached ");
				value_print(hc_c->v);
				printf("\n");
			}
#endif
			return(hc_c->v);
		}
	}
#endif

	v = value_new(VALUE_INTEGER);
	v->v.i = i;

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

#ifdef HASH_CONSING
	hc_c = malloc(sizeof(struct hc_chain));
	hc_c->next = hc_bucket[hc_h];
	hc_c->v = v;
	hc_bucket[hc_h] = hc_c;
#endif

	return(v);
}

struct value *
value_new_boolean(int b)
{
	struct value *v;

#ifdef HASH_CONSING
	hc_h = (b + 513) % HASH_CONS_SIZE;
	for (hc_c = hc_bucket[hc_h]; hc_c != NULL; hc_c = hc_c->next) {
		if (hc_c->v->type == VALUE_BOOLEAN && hc_c->v->v.b == b) {
			/*hc_c->v->admin |= ADMIN_SHARED;*/
#ifdef DEBUG
			if (trace_valloc > 0) {
				num_vars_cached++;
			}
			if (trace_valloc > 1) {
				printf("[RC] Cached ");
				value_print(hc_c->v);
				printf("\n");
			}
#endif
			return(hc_c->v);
		}
	}
#endif

	v = value_new(VALUE_BOOLEAN);
	v->v.b = b;

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

#ifdef HASH_CONSING
	hc_c = malloc(sizeof(struct hc_chain));
	hc_c->next = hc_bucket[hc_h];
	hc_c->v = v;
	hc_bucket[hc_h] = hc_c;
#endif

	return(v);
}

struct value *
value_new_atom(int atom)
{
	struct value *v;

	v = value_new(VALUE_ATOM);
	v->v.atom = atom;

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_string(char *s)
{
	struct value *v;

	v = value_new(VALUE_STRING);
	v->v.s = strdup(s);

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_list(void)
{
	struct value *v;

	v = value_new(VALUE_LIST);
	v->v.l = NULL;

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_error(char *error)
{
	struct value *v;

	v = value_new(VALUE_ERROR);
	v->v.e = strdup(error);

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_builtin(struct builtin *bi)
{
	struct value *v;

	v = value_new(VALUE_BUILTIN);
	v->v.bi = bi;

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_closure(struct ast *a, struct activation *ar, int arity, int locals, int cc)
{
	struct value *v;

	v = value_new(VALUE_CLOSURE);
	v->v.k = closure_new(a, ar, arity, locals, cc);

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_dict(void)
{
	struct value *v;

	v = value_new(VALUE_DICT);
	v->v.d = dict_new();

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

/*** ACCESSORS ***/

void
value_list_append(struct value **v, struct value *q)
{
#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[list] hello ");
		value_print(*v);
		printf(" <- ");
		value_print(q);
		printf("...\n");
	}
#endif

	if (*v == NULL) {
		*v = value_new_list();
	}

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[list] append ");
		value_print(q);
		printf(" to ");
		value_print(*v);
		printf("\n");
	}
#endif

	list_cons(&((*v)->v.l), q);
}

void
value_dict_store(struct value **v, struct value *k, struct value *d)
{
	if (*v == NULL)
		*v = value_new_dict();

#ifdef DEBUG
	if (trace_valloc > 1) {
		printf("[dict] store ");
		value_print(k);
		printf(",");
		value_print(d);
		printf(" in ");
		value_print(*v);
		printf("\n");
	}
#endif

	dict_store((*v)->v.d, k, d);
}

/*** OPERATIONS ***/

void
value_print(struct value *v)
{
	if (v == NULL) {
		printf("(null)");
		return;
	}
	/*printf("[0x%08lx](x%d)", (unsigned long)v, v->refcount);*/
	switch (v->type) {
	case VALUE_INTEGER:
		printf("%d", v->v.i);
		break;
	case VALUE_BOOLEAN:
		printf("%s", v->v.b ? "true" : "false");
		break;
	case VALUE_ATOM:
		printf("atom<%d>", v->v.atom);
		break;
	case VALUE_STRING:
		printf("\"%s\"", v->v.s);
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
	}
}

int
value_equal(struct value *a, struct value *b)
{
	int c;
	struct list *la, *lb;

	if (a == NULL && b == NULL)
		return(1);
	if (a == NULL || b == NULL)
		return(0);
	if (a->type != b->type)
		return(0);

	switch (a->type) {
	case VALUE_INTEGER:
		return(a->v.i == b->v.i);
	case VALUE_BOOLEAN:
		return(a->v.b == b->v.b);
	case VALUE_ATOM:
		return(a->v.atom == b->v.atom);
	case VALUE_STRING:
		return(strcmp(a->v.s, b->v.s) == 0);
	case VALUE_LIST:
		c = 1;
		for (la = a->v.l, lb = b->v.l;
		     la != NULL && lb != NULL;
		     la = la->next, lb = lb->next) {
			if (!value_equal(la->value, lb->value)) {
				c = 0;
				break;
			}
		}
		return(c);
	case VALUE_ERROR:
		return(strcmp(a->v.e, b->v.e) == 0);
	case VALUE_BUILTIN:
		return(a->v.bi == b->v.bi);
	case VALUE_CLOSURE:
		return(a->v.k == b->v.k);
	case VALUE_DICT:
		return(a->v.d == b->v.d);	/* XXX !!! */
	}
	return(0);
}
