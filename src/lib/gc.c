#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"
#include "activation.h"
#include "value.h"

#include "list.h"
#include "closure.h"
#include "gc.h"
#include "vm.h"
#include "process.h"

#ifdef DEBUG
extern int trace_gc;
#endif

int gc_trigger = DEFAULT_GC_TRIGGER;
int gc_target;

extern struct activation *a_head;

extern struct s_value *sv_head;

/*
 * Garbage collector.  Not a cheesy little reference counter, but
 * the real meat-and-potatoes mark-and-sweep.  (Which we need,
 * because an activation record can contain a closure which contain
 * an activation record, and refcounts can't handle that cycle.)
 *
 * This is not particularly sophisticated; I'm more concerned with
 * correctness than performance here.
 */
static void activation_mark(struct activation *a);

static void
value_mark(struct value v)
{
	struct list *l;

	if (!(v.type & VALUE_STRUCTURED) || v.v.s->admin & ADMIN_MARKED)
		return;

#ifdef DEBUG
	if (trace_gc > 1) {
		printf("[GC] MARKING VALUE ");
		value_print(v);
		printf(" AS REACHABLE\n");
	}
#endif

	v.v.s->admin |= ADMIN_MARKED;
	switch (v.type) {
	case VALUE_LIST:
		for (l = v.v.s->v.l; l != NULL; l = l->next) {
			value_mark(l->value);
		}
		break;
	case VALUE_CLOSURE:
		activation_mark(v.v.s->v.k->ar);
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

	if (a == NULL)
		return;
	if (a->admin & AR_ADMIN_MARKED) {
#ifdef DEBUG
		if (trace_gc > 1) {
			printf("[GC] ar ");
			activation_dump(a, 0);
			printf(" aready marked\n");
		}
#endif
		return;
	}

#ifdef DEBUG
	if (trace_gc > 1) {
		printf("[GC] MARKING AR ");
		activation_dump(a, 0);
		printf(" AS REACHABLE\n");
	}
#endif

	a->admin |= AR_ADMIN_MARKED;
	activation_mark(a->caller);
	activation_mark(a->enclosing);
	for (i = 0; i < a->size; i++) {
		value_mark(VALARY(a, i));
	}
}

void
gc(void)
{
	struct process *p;
	struct activation *a, *a_next;
	struct activation *ta_head = NULL;

	struct value *vsc;
	struct s_value *sv, *sv_next, *tsv_head = NULL;

	/*
	 * Mark...
	 */
	for (p = run_head; p != NULL; p = p->next) {
		activation_mark(p->vm->current_ar);
		for (vsc = p->vm->vstack; vsc < p->vm->vstack_ptr; vsc++)
			value_mark(*vsc);
	}

	/*
	 * ...and sweep
	 */
	for (a = a_head; a != NULL; a = a_next) {
		a_next = a->next;
		if (a->admin & AR_ADMIN_MARKED) {
			a->admin &= ~AR_ADMIN_MARKED;
			a->next = ta_head;
			ta_head = a;
		} else {
#ifdef DEBUG
			if (trace_gc > 1) {
				printf("[GC] FOUND UNREACHABLE AR ");
				activation_dump(a, 0);
				printf("\n");
			}
#endif
			activation_free_from_heap(a);
		}
	}

	a_head = ta_head;

	for (sv = sv_head; sv != NULL; sv = sv_next) {
		sv_next = sv->next;
		if (sv->admin & ADMIN_MARKED || sv->admin & ADMIN_PERMANENT) {
			sv->admin &= ~ADMIN_MARKED;
			sv->next = tsv_head;
			tsv_head = sv;
		} else {
#ifdef DEBUG
			if (trace_gc > 1) {
				printf("[GC] FOUND UNREACHABLE VALUE ");
				/*value_print(v);*/
				printf("\n");
			}
#endif
			s_value_free(sv);
		}
	}

	sv_head = tsv_head;
}
