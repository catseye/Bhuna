/*
 * Copyright (c)2004 Cat's Eye Technologies.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * 
 *   Neither the name of Cat's Eye Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
/*
 * value.c
 * Values for Bhuna.
 * $Id: symbol.c 54 2004-04-23 22:51:09Z catseye $
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "value.h"
#include "symbol.h"
#include "ast.h"
#include "list.h"
#include "closure.h"

#ifdef DEBUG
extern int trace_refcounting;
extern int num_vars_created;
extern int num_vars_grabbed;
extern int num_vars_released;
extern int num_vars_freed;
extern int num_vars_cowed;

struct list *global_table = NULL;
#endif

#ifdef DEBUG
static void
add_value_to_global_table(struct value *v)
{
	struct list *l;

	l = bhuna_malloc(sizeof(struct list));
	l->next = global_table;
	l->value = v;
	global_table = l;
}

static void
remove_value_from_global_table(struct value *v)
{
	struct list *l, *prev = NULL;

	for (l = global_table; l != NULL; l = l->next) {
		if (l->value == v) {
			if (prev != NULL) {
				prev->next = l->next;
			} else {
				global_table = l->next;
			}
			bhuna_free(l);
			return;
		}
		prev = l;
	}
	
	fprintf(stderr, "********* VALUE %08lx WAS NEVER ENTERED INTO GLOBAL TABLE!\n",
	    (unsigned long)v);
}
#endif

void
value_dump_global_table(void)
{
#ifdef DEBUG
	struct list *l;

	printf("----- BEGIN Global Table of Values -----\n");
	for (l = global_table; l != NULL; l = l->next) {
		value_print(l->value);
		printf("\n");
	}
	printf("----- END Global Table of Values -----\n");
#endif
}

/*** CONSTRUCTORS ***/

struct value *
value_new(int type)
{
	struct value *v;

	v = malloc(sizeof(struct value));
	bzero(v, sizeof(struct value));
	v->type = type;
	v->refcount = 1;

#ifdef DEBUG
	if (trace_refcounting > 0) {
		add_value_to_global_table(v);
		num_vars_created++;
	}
#endif

	return(v);
}

struct value *
value_new_integer(int i)
{
	struct value *v;

	v = value_new(VALUE_INTEGER);
	v->v.i = i;

#ifdef DEBUG
	if (trace_refcounting > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_boolean(int b)
{
	struct value *v;

	v = value_new(VALUE_BOOLEAN);
	v->v.b = b;

#ifdef DEBUG
	if (trace_refcounting > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
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
	if (trace_refcounting > 1) {
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
	if (trace_refcounting > 1) {
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
	if (trace_refcounting > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_symbol_table(struct symbol_table *stab)
{
	struct value *v;

	v = value_new(VALUE_STAB);
	v->v.stab = stab;

#ifdef DEBUG
	if (trace_refcounting > 1) {
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
	if (trace_refcounting > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_builtin(void (*f)(struct value **))
{
	struct value *v;

	v = value_new(VALUE_BUILTIN);
	v->v.f = f;

#ifdef DEBUG
	if (trace_refcounting > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_closure(struct ast *a, struct activation *ar)
{
	struct value *v;

	v = value_new(VALUE_CLOSURE);
	v->v.k = closure_new(a, ar);

#ifdef DEBUG
	if (trace_refcounting > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

struct value *
value_new_symbol(struct symbol *sym)
{
	struct value *v;

	v = value_new(VALUE_SYMBOL);
	v->v.sym = sym;

#ifdef DEBUG
	if (trace_refcounting > 1) {
		printf("[RC] created ");
		value_print(v);
		printf("\n");
	}
#endif

	return(v);
}

/*** DUPLICATOR ***/

/*
 * Returns a copy of the given value.
 * The copy is not so 'deep' as it could be, but should be OK w/refcounting.
 * New strings (char arrays) are created when copying a string;
 * New list spines (struct list *) are created, but values are only grabbed, not dup'ed.
 * Symbol tables and AST's are not copied, only the pointers to them.
 *
 * Note that the dup'ed value is 'new', i.e. it has a refcount of 1.
 */
static struct value *
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
		return(n);
	case VALUE_STAB:
		return(value_new_symbol_table(v->v.stab));
	case VALUE_ERROR:
		return(value_new_error(v->v.e));
	case VALUE_BUILTIN:
		return(value_new_builtin(v->v.f));
	case VALUE_CLOSURE:
		/* XXX depth?? */
		return(value_new_closure(v->v.k->ast, v->v.k->ar));
	case VALUE_SYMBOL:
		return(value_new_symbol(v->v.sym));
	default:
		return(value_new_error("unknown type"));
	}
}

/*** SETTING FUNCTIONS ***/

/*
 * If the given value has more than one reference to it,
 * make and return a new copy of it so that it can be
 * safely changed without affecting other users of it.
 */
static void
copy_on_write(struct value **v)
{
	struct value *old;

	if ((*v)->refcount > 1) {
#ifdef DEBUG
		if (trace_refcounting > 1) {
			printf("[RC] CoW of ");
			value_print(*v);
			printf(" w/refcount of %d", (*v)->refcount);
			if ((*v)->refcount > 1)
				printf(", dup'ing");
			printf("\n");
			num_vars_cowed++;
		}
#endif
		old = *v;
		*v = value_dup(*v);
		value_release(old);
	}
}

/*
 * If the given value is a value which can contain other values,
 * such as a list, 'empty' it so that it can take on a new value.
 */
static void
value_empty(struct value **v)
{
	switch ((*v)->type) {
	case VALUE_LIST:
		list_free(&(*v)->v.l);
		break;
	case VALUE_STRING:
		if ((*v)->v.s != NULL)
			bhuna_free((*v)->v.s);
		break;
	case VALUE_ERROR:
		if ((*v)->v.e != NULL)
			bhuna_free((*v)->v.e);
		break;
	/* VALUE_STAB? */
	case VALUE_CLOSURE:
		closure_free((*v)->v.k);
		break;
	}
}

/*
 * These `set' functions implement copy-on-write and emptying.
 */
void
value_set_integer(struct value **v, int i)
{
	if (*v == NULL) {
		*v = value_new_integer(i);
		return;
	}
	copy_on_write(v);
	value_empty(v);
	(*v)->type = VALUE_INTEGER;
	(*v)->v.i = i;
}

void
value_set_boolean(struct value **v, int b)
{
	if (*v == NULL) {
		*v = value_new_boolean(b);
		return;
	}
	copy_on_write(v);
	value_empty(v);
	(*v)->type = VALUE_BOOLEAN;
	(*v)->v.b = b;
}

void
value_set_atom(struct value **v, int atom)
{
	if (*v == NULL) {
		*v = value_new_atom(atom);
		return;
	}
	copy_on_write(v);
	value_empty(v);
	(*v)->type = VALUE_ATOM;
	(*v)->v.atom = atom;
}

void
value_set_string(struct value **v, char *s)
{
	if (*v == NULL) {
		*v = value_new_string(s);
		return;
	}
	copy_on_write(v);
	value_empty(v);
	(*v)->type = VALUE_STRING;
	(*v)->v.s = strdup(s);
}

void
value_set_list(struct value **v)
{
	if (*v == NULL) {
		*v = value_new_list();
		return;
	}
	copy_on_write(v);
	value_empty(v);
	(*v)->type = VALUE_LIST;
	(*v)->v.l = NULL;
}

void
value_list_append(struct value **v, struct value *q)
{
#ifdef DEBUG
	if (trace_refcounting > 1) {
		printf("[list] hello ");
		value_print(*v);
		printf(" <- ");
		value_print(q);
		printf("...\n");
	}
#endif

	if (*v == NULL)
		*v = value_new_list();

	copy_on_write(v);

#ifdef DEBUG
	if (trace_refcounting > 1) {
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
value_set_symbol_table(struct value **v, struct symbol_table *stab)
{
	if (*v == NULL) {
		*v = value_new_symbol_table(stab);
		return;
	}
	copy_on_write(v);
	value_empty(v);
	(*v)->type = VALUE_STAB;
	(*v)->v.stab = stab;
}

void
value_set_error(struct value **v, char *e)
{
	if (*v == NULL) {
		*v = value_new_error(e);
		return;
	}
	copy_on_write(v);
	value_empty(v);
	(*v)->type = VALUE_ERROR;
	(*v)->v.e = strdup(e);
}

void
value_set_builtin(struct value **v, void (*f)(struct value **))
{
	if (*v == NULL) {
		*v = value_new_builtin(f);
		return;
	}
	copy_on_write(v);
	value_empty(v);
	(*v)->type = VALUE_BUILTIN;
	(*v)->v.f = f;
}

void
value_set_closure(struct value **v, struct ast *a, struct activation *ar)
{
	if (*v == NULL) {
		*v = value_new_closure(a, ar);
		return;
	}

	copy_on_write(v);
	value_empty(v);

	(*v)->type = VALUE_CLOSURE;
	(*v)->v.k = closure_new(a, ar);
}

void
value_set_symbol(struct value **v, struct symbol *sym)
{
	if (*v == NULL) {
		*v = value_new_symbol(sym);
		return;
	}
	copy_on_write(v);
	value_empty(v);
	(*v)->type = VALUE_SYMBOL;
	(*v)->v.sym = sym;
}

/*-----*/

void
value_set_from_value(struct value **v, struct value *q)
{
	/*
	 * We are making another reference to q, so we must grab it
	 */
	value_release(*v);
	value_grab(q);
	*v = q;
}

/*** DESTRUCTOR ***/

static void
value_free(struct value *v)
{
	if (v == NULL)
		return;

	assert(v->refcount == 0);

#ifdef DEBUG
	if (trace_refcounting > 1) {
		printf("[RC] freeing ");
		value_print(v);
		printf("\n");
	}
#endif

	switch (v->type) {
	case VALUE_STRING:
		bhuna_free(v->v.s);
		break;
	case VALUE_LIST:
		list_free(&v->v.l);
		break;
	case VALUE_STAB:
		symbol_table_free(v->v.stab);
		break;
	case VALUE_ERROR:
		bhuna_free(v->v.e);
		break;
	case VALUE_CLOSURE:
		closure_free(v->v.k);
		break;
	}

#ifdef DEBUG
	if (trace_refcounting > 0) {
		remove_value_from_global_table(v);
		num_vars_freed++;
	}
#endif

	bhuna_free(v);
}

/*** REFCOUNTERS ***/

void
value_grab(struct value *v)
{
	if (v == NULL)
		return;
	v->refcount++;
#ifdef DEBUG
	if (trace_refcounting > 1) {
		printf("[RC] grabbed ");
		value_print(v);
		printf(", refcount now %d\n", v->refcount);
		num_vars_grabbed++;
	}
#endif
}

void
value_release(struct value *v)
{
	if (v == NULL)
		return;
	v->refcount--;
#ifdef DEBUG
	if (trace_refcounting > 1) {
		printf("[RC] released ");
		value_print(v);
		printf(", refcount now %d\n", v->refcount);
		num_vars_released++;
	}
#endif
	if (v->refcount == 0)
		value_free(v);
}

void
value_print(struct value *v)
{
	if (v == NULL) {
		printf("(null)");
		return;
	}
	printf("[0x%08lx]", (unsigned long)v);
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
		break;
	case VALUE_STAB:
		return(a->v.stab == b->v.stab);
	case VALUE_ERROR:
		return(strcmp(a->v.e, b->v.e) == 0);
	case VALUE_BUILTIN:
		return(a->v.f == b->v.f);
	case VALUE_CLOSURE:
		return(a->v.k->ast == b->v.k->ast);	/* XXX */
	case VALUE_SYMBOL:
		return(a->v.sym == b->v.sym);
	}
	return(0);
}
