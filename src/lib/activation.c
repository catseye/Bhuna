#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"
#include "activation.h"
#include "value.h"
#include "list.h"
#include "closure.h"

#ifdef DEBUG
extern int trace_activations;
extern int activations_allocated;
extern int activations_freed;
#endif

struct activation *a_head = NULL;
int a_count = 0;

struct activation *
activation_new_on_heap(int size, struct activation *caller, struct activation *enclosing)
{
	struct activation *a;

	a = bhuna_malloc(sizeof(struct activation) +
	    sizeof(struct value *) * size);
#ifdef BZERO
	bzero(a, sizeof(struct activation) +
	    sizeof(struct value *) * size);
#endif
	a->size = size;
	a->admin = 0;
	a->caller = caller;
	a->enclosing = enclosing;

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
activation_new_on_stack(int size, struct activation *caller,
			struct activation *enclosing, struct vm *vm)
{
	struct activation *a;

	a = (struct activation *)vm->astack_ptr;
	vm->astack_ptr += sizeof(struct activation) + sizeof(struct value *) * size;

	a->size = size;
	a->admin = AR_ADMIN_ON_STACK;
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
#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[ARC] freeing from HEAP ");
		activation_dump(a, -1);
		printf("\n");
	}
	activations_freed++;
#endif

	bhuna_free(a);
	a_count--;
}

void
activation_free_from_stack(struct activation *a, struct vm *vm)
{
#ifdef DEBUG
	if (trace_activations > 1) {
		printf("[ARC] freeing from STACK ");
		activation_dump(a, -1);
		printf("\n");
	}
	activations_freed++;
#endif

	vm->astack_ptr -= (sizeof(struct activation) +
			   sizeof(struct value *) * a->size);
}

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
	struct value *d;

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
	v->refcount++;
	d = VALARY(a, index);
	if (d != NULL)
		d->refcount--;
	VALARY(a, index) = v;
}

void
activation_initialize_value(struct activation *a, int index,
			    struct value *v)
{
	assert(a != NULL);
	assert(index < a->size);
	VALARY(a, index) = v;
}

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
