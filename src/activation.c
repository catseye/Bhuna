#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"
#include "activation.h"
#include "value.h"
#include "list.h"
#include "closure.h"

#define VALARY(a,i)	\
	((struct value **)((char *)a + sizeof(struct activation)))[i]

#ifdef DEBUG
extern int trace_activations;
extern int activations_allocated;
extern int activations_freed;
#endif

extern struct activation *current_ar;
extern int gc_trigger;
extern int gc_target;

static struct activation *a_head = NULL;
static int a_count = 0;

#define A_STACK_SIZE	65536

unsigned char a_stack[A_STACK_SIZE];
unsigned char *a_sp = a_stack;

struct activation *
activation_new_on_heap(int size, struct activation *caller, struct activation *enclosing)
{
	struct activation *a;

	if (a_count > gc_target) {
		/*printf("GC!\n");*/
#ifdef DEBUG
		if (trace_activations > 1) {
			printf("[ARC] GARBAGE COLLECTION STARTED on %d activation records!!!\n",
				a_count);
			activation_dump(current_ar, 0);
			printf("\n");
		}
#endif
		activation_gc();
#ifdef DEBUG
		if (trace_activations > 1) {
			printf("[ARC] GARBAGE COLLECTION FINISHED, now %d activation records!!!\n",
				a_count);
			activation_dump(current_ar, 0);
			printf("\n");
		}
#endif
		/*
		 * Slide the target to account for the fact that there
		 * are now 'a_count' activation records in existence.
		 * Only GC when there are gc_trigger *more* ar's.
		 */
		gc_target = a_count + gc_trigger;
	}

	a = bhuna_malloc(sizeof(struct activation) +
	    sizeof(struct value *) * size);
	/*bzero(a, sizeof(struct activation) +
	    sizeof(struct value *) * size);*/
	a->size = size;
	a->caller = caller;
	a->enclosing = enclosing;
	a->marked = 0;

	/*
	 * Link up to our GC list.
	 */
	a->next = a_head;
	a_head = a;

#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[ARC] created on HEAP");
		activation_dump(a, -1);
		printf("\n");
	}
	activations_allocated++;
#endif

	a_count++;
	return(a);
}

struct activation *
activation_new_on_stack(int size, struct activation *caller, struct activation *enclosing)
{
	struct activation *a;

	a = (struct activation *)a_sp;
	a_sp += sizeof(struct activation) + sizeof(struct value *) * size;

	a->size = size;
	a->caller = caller;
	a->enclosing = enclosing;

#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[ARC] created on STACK ");
		activation_dump(a, -1);
		printf("\n");
	}
	activations_allocated++;
#endif

	return(a);
}

void
activation_free_from_heap(struct activation *a)
{
	int i;

#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[ARC] freeing from HEAP ");
		activation_dump(a, -1);
		printf("\n");
	}
	activations_freed++;
#endif

	for (i = 0; i < a->size; i++)
		value_release(VALARY(a, i));

	bhuna_free(a);
	a_count--;
}

int
activation_is_on_stack(struct activation *a)
{
	return ((unsigned char *)a >= a_stack &&
		(unsigned char *)a < (a_stack + A_STACK_SIZE));
}

void
activation_free_from_stack(struct activation *a)
{
	int i;

#ifdef DEBUG
		if (trace_activations > 1) {
			printf("[ARC] freeing from STACK ");
			activation_dump(a, -1);
			printf("\n");
		}
		activations_freed++;
#endif

	for (i = 0; i < a->size; i++)
		value_release(VALARY(a, i));

	a_sp -= (sizeof(struct activation) + sizeof(struct value *) * a->size);
}

/*#ifndef REFCOUNTING_MACROS*/
struct value *
activation_get_value(struct activation *a, int index, int upcount)
{
	assert(a != NULL);
	for (; upcount > 0; upcount--) {
		a = a->enclosing;
		assert(a != NULL);
	}
#ifdef DEBUG
	assert(index < a->size);
#endif
	return(((struct value **)((char *)a + sizeof(struct activation)))[index]);
}

void
activation_set_value(struct activation *a, int index, int upcount,
		     struct value *v)
{
	assert(a != NULL);
	for (; upcount > 0; upcount--) {
		a = a->enclosing;
		assert(a != NULL);
	}

#ifdef DEBUG
/*
	printf("set: index = %d, a->size = %d in \n", index, a->size);
	activation_dump(a, 1);
	printf("\n");
*/
	assert(index < a->size);
#endif
	value_release(VALARY(a, index));
	value_grab(v);
	VALARY(a, index) = v;
}

void
activation_initialize_value(struct activation *a, int index,
			    struct value *v)
{
	assert(a != NULL);
	assert(index < a->size);
	value_grab(v);
	VALARY(a, index) = v;
}
/*#endif*/

void
activation_dump(struct activation *a, int detail)
{
#ifdef DEBUG
	int i;

	printf("A/");
	if (a == NULL) {
		printf("(NULL)/");
		return;
	}
	printf("%08lx:%d", (unsigned long)a, a->size);
	
	if (detail > 0) {
		for (i = 0; i < a->size; i++) {
			printf(" ");
			if (VALARY(a, i) != NULL && VALARY(a, i)->type == VALUE_CLOSURE) {
				printf("(closure) ");
			} else {
				value_print(VALARY(a, i));
			}
		}
	}
	
	if (a->enclosing != NULL) {
		printf(" --> {");
		activation_dump(a->enclosing, 0);
		printf("}");
	}
	if (a->caller != NULL) {
		printf(" +++> {");
		activation_dump(a->caller, 0);
		printf("}");
	}
	printf("/");
#endif
}

/*
 * Garbage collector.  Not just the cheesy little reference counter, but
 * the real meat-and-potatoes mark-and-sweep.  (Which we need,
 * because an activation record can contain a closure which contain
 * an activation record, and refcounts can't handle that cycle.)
 *
 * This is not particularly sophisticated; I'm more concerned with
 * correctness than performance here.
 */
static void activation_mark(struct activation *a);

static void
value_mark(struct value *v)
{
	struct list *l;

	if (v == NULL)
		return;
	switch (v->type) {
	case VALUE_LIST:
		for (l = v->v.l; l != NULL; l = l->next) {
			value_mark(l->value);
		}
		break;
	case VALUE_CLOSURE:
		activation_mark(v->v.k->ar);
		break;
	case VALUE_DICT:
		/* XXX for each key in v->v.d, value_mark(d[k]) */
		break;
	default:
		/*
		 * No need to go through other values as they
		 * are not containers.
		 */
		break;
	}
}

static void
activation_mark(struct activation *a)
{
	int i;

	if (a == NULL || a->marked)
		return;

#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[GC] MARKING AS REACHABLE: ");
		activation_dump(a, 0);
		printf("\n");
	}
#endif

	a->marked = 1;
	activation_mark(a->caller);
	activation_mark(a->enclosing);
	for (i = 0; i < a->size; i++) {
		value_mark(VALARY(a, i));
	}
}

void
activation_gc(void)
{
	struct activation *a, *next;
	struct activation *t_head = NULL;

	/*
	 * Mark...
	 */
	activation_mark(current_ar);

	/*
	 * ...and sweep
	 */
	for (a = a_head; a != NULL; a = next) {
		next = a->next;
		if (a->marked) {
			a->marked = 0;
			a->next = t_head;
			t_head = a;
		} else {
#ifdef DEBUG
			if (trace_activations > 1) {
				printf("[GC] FOUND UNREACHABLE: ");
				activation_dump(a, 0);
				printf("\n");
			}
#endif
			activation_free_from_heap(a);
		}
	}

	a_head = t_head;
}
