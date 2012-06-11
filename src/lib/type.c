#include <assert.h>
#include <stdio.h>

#include "mem.h"
#include "type.h"
#include "report.h"
#include "scan.h"

static struct type *t_head = NULL;

struct type *
type_new(int tclass)
{
	struct type *t;
		
	t = bhuna_malloc(sizeof(struct type));
	t->tclass = tclass;
	t->unifier = NULL;
	t->next = t_head;
	t_head = NULL;

	return(t);
}

struct type *
type_new_list(struct type *contents)
{
	struct type *t = type_new(TYPE_LIST);

	t->t.list.contents = contents;

	return(t);
}

struct type *
type_new_dict(struct type *index, struct type *contents)
{
	struct type *t = type_new(TYPE_DICT);

	t->t.dict.index = index;
	t->t.dict.contents = contents;

	return(t);
}

struct type *
type_new_closure(struct type *domain, struct type *range)
{
	struct type *t = type_new(TYPE_CLOSURE);

	t->t.closure.domain = domain;
	t->t.closure.range = range;

	return(t);
}

struct type *
type_new_arg(struct type *left, struct type *right)
{
	struct type *t = type_new(TYPE_ARG);

	t->t.arg.left = left;
	t->t.arg.right = right;

	return(t);
}

struct type *
type_new_set(struct type *left, struct type *right)
{
	struct type *t;

	/*
	printf("constructing set from:\n1: ");
	type_print(stdout, left);
	printf("\n2: ");
	type_print(stdout, right);
	printf("\n");
	*/

	if (type_equal(type_representative(left), type_representative(right)))
		return(left);

	/* ???
	if (type_is_void(left))
		return(right);
	if (type_is_void(right))
		return(left);
	*/

	t = type_new(TYPE_SET);
	t->t.set.left = left;
	t->t.set.right = right;

	return(t);
}

struct type *
type_new_var(int num)
{
	struct type *t = type_new(TYPE_VAR);

	t->t.var.num = num;

	return(t);
}

static int next_var_num = 10;

struct type *
type_brand_new_var(void)
{
	struct type *t = type_new(TYPE_VAR);

	t->t.var.num = next_var_num++;

	return(t);
}

void
types_free(void)
{
	struct type *t, *t_next;

	for (t = t_head; t != NULL; t = t_next) {
		t_next = t->next;
		bhuna_free(t);
	}
}

/*
 * Structural equivalence - rarely if ever needed now?
 */
int
type_equal(struct type *a, struct type *b)
{
	if (a == NULL && b == NULL)
		return(1);
	if (a == NULL || b == NULL)
		return(0);
	if (a->tclass != b->tclass)
		return(0);

	switch (a->tclass) {
	case TYPE_LIST:
		return(type_equal(a->t.list.contents, b->t.list.contents));
	case TYPE_DICT:
		return(type_equal(a->t.dict.index, b->t.dict.index) &&
		       type_equal(a->t.dict.contents, b->t.dict.contents));
	case TYPE_CLOSURE:
		return(type_equal(a->t.closure.domain, b->t.closure.domain) &&
		       type_equal(a->t.closure.range, b->t.closure.range));
	case TYPE_ARG:
		return(type_equal(a->t.arg.left, b->t.arg.left) &&
		       type_equal(a->t.arg.right, b->t.arg.right));
	case TYPE_SET:
		return(type_equal(a->t.set.left, b->t.set.left) &&
		       type_equal(a->t.set.right, b->t.set.right));
	}
	return(1);
}

/************ TYPE INFERENCE *************/

/*
 * Unification algorithm
 * Shamelessly adapted from the Dragon Book.
 */

/*
 * Find the representative of the equivalence class of a type.
 * This is used by external code to get the concrete type
 * lurking behind a (bound) type variable.
 */
struct type *
type_representative(struct type *q)
{
	struct type *p = q;

	while (p->unifier != NULL) {
		p = p->unifier;
	}

	return(p);
}

/*
 * Merge the two equivalence classes of the two types.
 */
void
type_union(struct type *m, struct type *n)
{
	struct type *s, *t;

	s = type_representative(m);
	t = type_representative(n);

	if (s->tclass != TYPE_VAR) {
		t->unifier = s;
	} else if (t->tclass != TYPE_VAR) {
		s->unifier = t;
	} else {
		s->unifier = t;
	}
}

/*
 * Make two type expressions equal through substitutions.
 */
int
type_unify(struct type *m, struct type *n)
{
	struct type *s, *t;

	s = type_representative(m);
	t = type_representative(n);

	if (s == t) {
		return(1);
	} else if (s->tclass == TYPE_DICT && t->tclass == TYPE_DICT) {
		type_union(s, t);
		return(type_unify(s->t.dict.index, t->t.dict.index) &&
		       type_unify(s->t.dict.contents, t->t.dict.contents));
	} else if (s->tclass == TYPE_LIST && t->tclass == TYPE_LIST) {
		type_union(s, t);
		return(type_unify(s->t.list.contents, t->t.list.contents));
	} else if (s->tclass == TYPE_CLOSURE && t->tclass == TYPE_CLOSURE) {
		type_union(s, t);
		return(type_unify(s->t.closure.domain, t->t.closure.domain) &&
		       type_unify(s->t.closure.range, t->t.closure.range));
	} else if (s->tclass == TYPE_ARG && t->tclass == TYPE_ARG) {
		type_union(s, t);
		return(type_unify(s->t.arg.left, t->t.arg.left) &&
		       type_unify(s->t.arg.right, t->t.arg.right));
	} else if (s->tclass == TYPE_SET && t->tclass == TYPE_SET) {
		/* XXX actually we should also check when one is a set and one isn't,
		  and succeed if the one that isn't the set is *in* the set... */
		type_union(s, t);
		return(type_unify(s->t.set.left, t->t.set.left) &&
		       type_unify(s->t.set.right, t->t.set.right));
	} else if (s->tclass == TYPE_VAR || t->tclass == TYPE_VAR) {
		type_union(s, t);
		return(1);
	} else if (s->tclass == t->tclass) {
		return(1);
	} else {
		return(0);
	}
}

int
type_unify_crit(struct scan_st *sc, struct type *m, struct type *n)
{
	int unified;
	
	if (!(unified = type_unify(m, n))) {
		report(REPORT_ERROR, sc,
		   "Failed to unify types %t and %t",
		    m, n);
	}

	return(unified);
}

/*
 * If the given type is an unbound variable, unify it with a function
 * from a (fresh) unbound variable to another (fresh) unbound variable.
 * This way we can handle unifying just the domain or just the range
 * part of a (variable) type with another type.
 */
void
type_ensure_routine(struct type *t)
{
	struct type *r, *n;

	r = type_representative(t);
	if (r->tclass == TYPE_VAR) {
		n = type_new_closure(type_brand_new_var(), type_brand_new_var());
		r->unifier = n;
	}
}

int
type_is_possibly_routine(struct type *t)
{
	struct type *r;

	r = type_representative(t);
	return(r->tclass == TYPE_VAR || r->tclass == TYPE_CLOSURE);
}

int
type_is_void(struct type *t)
{
	struct type *r;

	r = type_representative(t);
	return(r->tclass == TYPE_VOID);
}

int
type_is_set(struct type *t)
{
	struct type *r;

	r = type_representative(t);
	return(r->tclass == TYPE_SET);
}

int
type_set_contains_void(struct type *t)
{
	struct type *r;

	r = type_representative(t);
	if (r->tclass == TYPE_VOID) {
		return(1);
	} else if (r->tclass == TYPE_SET) {
		return(type_set_contains_void(r->t.set.left) ||
		       type_set_contains_void(r->t.set.right));
	} else {
		return(0);
	}
}

void
type_print(FILE *f, struct type *t)
{
#ifdef DEBUG
	if (t == NULL) {
		fprintf(f, "(?null?)");
		return;
	}
	switch (t->tclass) {
	case TYPE_VOID:
		fprintf(f, "void");
		break;
	case TYPE_INTEGER:
		fprintf(f, "integer");
		break;
	case TYPE_BOOLEAN:
		fprintf(f, "boolean");
		break;
	case TYPE_ATOM:
		fprintf(f, "atom");
		break;
	case TYPE_STRING:
		fprintf(f, "string");
		break;
	case TYPE_LIST:
		fprintf(f, "list of ");
		type_print(f, t->t.list.contents);
		break;
	case TYPE_ERROR:
		fprintf(f, "error");
		break;
	case TYPE_BUILTIN:
		fprintf(f, "builtin");
		break;
	case TYPE_OPAQUE:
		fprintf(f, "opaque");
		break;
	case TYPE_VAR:
		fprintf(f, "Type%d", t->t.var.num);
		if (t->unifier != NULL) {
			fprintf(f, "=(");
			type_print(f, t->unifier);
			fprintf(f, ")");
		}
		break;
	case TYPE_ARG:
		type_print(f, t->t.arg.left);
		fprintf(f, ", ");
		type_print(f, t->t.arg.right);
		break;
	case TYPE_SET:
		fprintf(f, "(");
		type_print(f, t->t.arg.left);
		fprintf(f, " | ");
		type_print(f, t->t.arg.right);
		fprintf(f, ")");
		break;
	case TYPE_DICT:
		fprintf(f, "dict from ");
		type_print(f, t->t.dict.index);
		fprintf(f, " to ");
		type_print(f, t->t.dict.contents);
		break;
	case TYPE_CLOSURE:
		fprintf(f, "fn from ");
		type_print(f, t->t.closure.domain);
		fprintf(f, " to ");
		type_print(f, t->t.closure.range);
		break;
	}
#endif
}
