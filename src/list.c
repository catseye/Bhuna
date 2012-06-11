#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "mem.h"
#include "list.h"
#include "value.h"

void
list_cons(struct list **l, struct value *v)
{
	struct list *n;

	n = bhuna_malloc(sizeof(struct list));
	value_grab(v);
	n->value = v;
	n->next = *l;
	*l = n;
}

struct list *
list_dup(struct list *l)
{
	struct list *n;

	/* ... XXX ... */

	return(n);
}

void
list_free(struct list **l)
{
	struct list *next;

	while ((*l) != NULL) {
		next = (*l)->next;
		value_release((*l)->value);
		bhuna_free((*l));
		(*l) = next;
	}
}

size_t
list_length(struct list *l)
{
	size_t i = 0;

	while(l != NULL) {
		i++;
		l = l->next;
	}

	return(i);
}

/*
 * Full comparison used here.
 */
int
list_contains(struct list *l, struct value *v)
{
	while (l != NULL) {
		if (value_equal(l->value, v))
			return(1);
		l = l->next;
	}

	return(0);
}

void
list_dump(struct list *l)
{
	printf("[");
	while (l != NULL) {
		value_print(l->value);
		if (l->next != NULL)
			printf(",");
		l = l->next;
	}
	printf("]");
}
