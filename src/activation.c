#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "activation.h"

#include "mem.h"
#include "symbol.h"
#include "value.h"

#ifdef DEBUG
extern int trace_activations;
#endif

struct activation *
activation_new(struct symbol_table *stab, struct activation *enclosing)
{
	struct activation *a;
	struct symbol *sym;

	a = bhuna_malloc(sizeof(struct activation));
	a->alist = NULL;
	activation_grab(enclosing);
	a->enclosing = enclosing;
	a->refcount = 1;

	for (sym = stab->head; sym != NULL; sym = sym->next) {
		struct alist *al;

		al = bhuna_malloc(sizeof(struct alist));
		al->next = a->alist;
		al->sym = sym;
		al->value = NULL;
		a->alist = al;
	}

#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[ARC] created ");
		activation_dump(a, -1);
		printf("\n");
	}
#endif

	return(a);
}

void
activation_free(struct activation *a)
{
	struct alist *next;

#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[ARC] freeing ");
		activation_dump(a, -1);
		printf("\n");
	}
#endif

	assert(a->refcount == 0);
	while (a->alist != NULL) {
		next = a->alist->next;
		value_release(a->alist->value);
		bhuna_free(a->alist);
		a->alist = next;
	}
	activation_release(a->enclosing);
	bhuna_free(a);
}

/*** REFCOUNTERS ***/

void
activation_grab(struct activation *a)
{
	if (a == NULL)
		return;
	a->refcount++;
#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[ARC] ");
		activation_dump(a, -1);
		printf(" grabbed, refcount now %d\n", a->refcount);
	}
#endif
}

void
activation_release(struct activation *a)
{
	if (a == NULL)
		return;
	a->refcount--;
#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[ARC] ");
		activation_dump(a, -1);
		printf(" released, refcount now %d\n", a->refcount);
	}
#endif
	if (a->refcount == 0)
		activation_free(a);	
}

static struct alist *
activation_find_sym(struct activation *a, struct symbol *sym)
{
	struct alist *al;

	for (al = a->alist; al != NULL; al = al->next) {
		if (al->sym == sym)
			return(al);
	}

	if (a->enclosing != NULL)
		return(activation_find_sym(a->enclosing, sym));

	return(NULL);
}

struct value *
activation_get_value(struct activation *a, struct symbol *sym)
{
	struct alist *al;

	al = activation_find_sym(a, sym);
	assert(al != NULL);

	return(al->value);
}

void
activation_set_value(struct activation *a, struct symbol *sym, struct value *v)
{
	struct alist *al;

	if ((al = activation_find_sym(a, sym)) == NULL) {
#ifdef DEBUG
	if (trace_activations > 1) {
		printf("WARNING: ");
		symbol_dump(sym, 0);
		printf(" (=");
		value_print(v);
		printf(") not found in ");
		activation_dump(a, -1);
		printf(", adding\n");
	}
#endif

		al = bhuna_malloc(sizeof(struct alist));
		al->next = a->alist;
		al->sym = sym;
		al->value = NULL;
		a->alist = al;
	}

	value_release(al->value);
	value_grab(v);
	al->value = v;
}

void
activation_dump(struct activation *a, int detail)
{
#ifdef DEBUG
	struct alist *al;

	printf("Activation/");
	if (a == NULL) {
		printf("(NULL)/");
		return;
	}
	if (detail == -1) {
		printf("%08lx", (unsigned long)a);
	} else {
		for (al = a->alist; al != NULL; al = al->next) {
			symbol_dump(al->sym, 0);
			if (detail) {
				printf("=");
				value_print(al->value);
			}
			printf("  ");
		}
	}
	if (a->enclosing != NULL) {
		printf(" --> ");
		activation_dump(a->enclosing, detail);
	}
	printf("/");
#endif
}