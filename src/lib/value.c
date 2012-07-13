/*
 * value.c
 * Values for Bhuna.
 * $Id$
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "mem.h"
#include "value.h"
#include "ast.h"
#include "list.h"
#include "dict.h"
#include "closure.h"
#include "utf8.h"
#include "type.h"

#ifdef DEBUG
extern int trace_valloc;
extern int num_vars_created;
extern int num_vars_cached;
extern int num_vars_freed;
#endif

struct s_value *sv_head = NULL;

struct value
value_null(void)
{
	struct value v;

	v.type = VALUE_NULL;
	
	return(v);
}

/*
 * Assert that this value should not be garbage-collected.
 * Typically used for values in the symbol table, vm program body, etc.
 */
void
value_deregister(struct value v)
{
	if (v.type & VALUE_STRUCTURED)
		v.v.s->admin |= ADMIN_PERMANENT;
}

/*
 * Return a deep(ish) copy of the given value.
 * New strings (char arrays) are created when copying a string;
 * New list spines (struct list *) are created, but values are only grabbed, not dup'ed.
 * Some things are not copied, only the pointers to them.
 *
 * Note that the dup'ed value is 'new', i.e. it has a refcount of 1.
 */
struct value
value_dup(struct value v)
{
	struct value n;
	/*struct list *l;*/

	switch (v.type) {
	case VALUE_INTEGER:
		return(value_new_integer(v.v.i));
	case VALUE_BOOLEAN:
		return(value_new_boolean(v.v.b));
	case VALUE_STRING:
		return(value_new_string(v.v.s->v.s));
	case VALUE_LIST:
		n = value_new_list();
	/*
		for (l = v.v.s->v.l; l != NULL; l = l->next) {
			value_list_append(&n, l->value);
		}
	*/
		/*
		n = value_new(VALUE_LIST);
		n->v.l = list_dup(v->v.l);
		*/
		return(n);
	case VALUE_ERROR:
		return(value_new_error(v.v.s->v.e));
	case VALUE_BUILTIN:
		return(value_new_builtin(v.v.bi));
	case VALUE_CLOSURE:
		return(value_new_closure(v.v.s->v.k->ast, v.v.s->v.k->ar,
		    v.v.s->v.k->arity, v.v.s->v.k->locals, v.v.s->v.k->cc));
	case VALUE_DICT:
		n = value_new_dict(); /* XXX */
		n.v.s->v.d = dict_dup(v.v.s->v.d);
		return(n);
	case VALUE_OPAQUE:
		return(value_new_opaque(v.v.ptr));
	default:
		return(value_new_error("unknown type"));
	}
}

/*** DESTRUCTOR ***/

void
s_value_free(struct s_value *sv)
{
	switch (sv->type) {
	case VALUE_LIST:
		list_free(&sv->v.l);
		break;
	case VALUE_STRING:
		if (sv->v.s != NULL)
			bhuna_free(sv->v.s);
		break;
	case VALUE_ERROR:
		if (sv->v.e != NULL)
			bhuna_free(sv->v.e);
		break;
	case VALUE_CLOSURE:
		closure_free(sv->v.k);
		break;
	case VALUE_DICT:
		dict_free(sv->v.d);
		break;
	case VALUE_OPAQUE:
		/* XXX oiks.  user GC "finalizer" ? */
		break;
	}

	bhuna_free(sv);
}

/*** SPECIFIC CONSTRUCTORS ***/
/*** simple values ***/

struct value
value_new_integer(int i)
{
	struct value v;

	v.type = VALUE_INTEGER;
	v.v.i = i;
	
	return(v);
}

struct value
value_new_boolean(int b)
{
	struct value v;

	v.type = VALUE_BOOLEAN;
	v.v.b = b;
	
	return(v);
}

struct value
value_new_atom(int atom)
{
	struct value v;

	v.type = VALUE_ATOM;
	v.v.a = atom;
	
	return(v);
}

struct value
value_new_builtin(struct builtin *bi)
{
	struct value v;

	v.type = VALUE_BUILTIN;
	v.v.bi = bi;
	
	return(v);
}

struct value
value_new_opaque(void *ptr)
{
	struct value v;

	v.type = VALUE_OPAQUE;
	v.v.ptr = ptr;
	
	return(v);
}

/*** structured values ***/

struct s_value *
s_value_new(unsigned char type)
{
	struct s_value *sv;

	sv = bhuna_malloc(sizeof(struct s_value));
	sv->next = sv_head;
	sv_head = sv;
	sv->admin = 0;
	sv->type = type;
	sv->refcount = 0;

	return(sv);
}

struct value
value_new_string(wchar_t *s)
{
	struct value v;

	v.type = VALUE_STRING;
	v.v.s = s_value_new(VALUE_STRING);
	v.v.s->v.s = bhuna_wcsdup(s);

	return(v);
}

struct value
value_new_list(void)
{
	struct value v;

	v.type = VALUE_LIST;
	v.v.s = s_value_new(VALUE_LIST);
	v.v.s->v.l = NULL;

	return(v);
}

struct value
value_new_error(const char *error)
{
	struct value v;

	v.type = VALUE_ERROR;
	v.v.s = s_value_new(VALUE_ERROR);
	v.v.s->v.e = strdup(error);

	return(v);
}

struct value
value_new_closure(struct ast *a, struct activation *ar, int arity, int locals, int cc)
{
	struct value v;

	v.type = VALUE_CLOSURE;
	v.v.s = s_value_new(VALUE_CLOSURE);
	v.v.s->v.k = closure_new(a, ar, arity, locals, cc);

	return(v);
}

struct value
value_new_dict(void)
{
	struct value v;

	v.type = VALUE_DICT;
	v.v.s = s_value_new(VALUE_DICT);
	v.v.s->v.d = dict_new();

	return(v);
}

/*** ACCESSORS ***/

void
value_list_append(struct value v, struct value q)
{
	list_cons(&v.v.s->v.l, q);
}

void
value_dict_store(struct value v, struct value k, struct value d)
{
	dict_store(v.v.s->v.d, k, d);
}

/*** OPERATIONS ***/

void
value_print(struct value v)
{
	/*printf("[0x%08lx](x%d)", (unsigned long)v, v->refcount);*/
	switch (v.type) {
	case VALUE_INTEGER:
		printf("%d", v.v.i);
		break;
	case VALUE_BOOLEAN:
		printf("%s", v.v.b ? "true" : "false");
		break;
	case VALUE_ATOM:
		printf("atom<%d>", v.v.a);
		break;
	case VALUE_BUILTIN:
		printf("#BIF<%08lx>", (unsigned long)v.v.bi);
		break;
	case VALUE_OPAQUE:
		printf("#OPAQUE<%08lx>", (unsigned long)v.v.ptr);
		break;

	case VALUE_STRING:
		printf("\"");
		fputsu8(stdout, v.v.s->v.s);
		printf("\"");
		break;
	case VALUE_LIST:
		list_dump(v.v.s->v.l);
		break;
	case VALUE_ERROR:
		printf("#ERR<%s>", v.v.s->v.e);
		break;
	case VALUE_CLOSURE:
		closure_dump(v.v.s->v.k);
		break;
	case VALUE_DICT:
		dict_dump(v.v.s->v.d);
		break;
	}
}

int
value_equal(struct value a, struct value b)
{
	int c;
	/* struct list *la, *lb; */

	if (a.type != b.type)
		return(0);

	switch (a.type) {
	case VALUE_INTEGER:
		return(a.v.i == b.v.i);
	case VALUE_BOOLEAN:
		return(a.v.b == b.v.b);
	case VALUE_ATOM:
		return(a.v.a == b.v.a);
	case VALUE_STRING:
		return(wcscmp(a.v.s->v.s, b.v.s->v.s) == 0);
	case VALUE_LIST:
		c = 1;
	/*
		for (la = a.v.s->v.l, lb = b.v.s->v.l;
		     la != NULL && lb != NULL;
		     la = la->next, lb = lb->next) {
			if (!value_equal(la->value, lb->value)) {
				c = 0;
				break;
			}
		}
	*/
		return(c);
	case VALUE_ERROR:
		return(strcmp(a.v.s->v.e, b.v.s->v.e) == 0);
	case VALUE_BUILTIN:
		return(a.v.bi == b.v.bi);
	case VALUE_CLOSURE:
		return(a.v.s->v.k == b.v.s->v.k);
	case VALUE_DICT:
		return(a.v.s->v.d == b.v.s->v.d);	/* XXX !!! */
	case VALUE_OPAQUE:
		return(a.v.ptr == b.v.ptr);
	}
	return(0);
}
