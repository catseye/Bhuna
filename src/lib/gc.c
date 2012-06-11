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

#ifdef POOL_VALUES
#include "pool.h"
#endif

#ifdef DEBUG
extern int trace_gc;
#endif

int gc_trigger = DEFAULT_GC_TRIGGER;
int gc_target;

extern struct activation *a_head;

#ifdef HASH_CONSING
extern struct hc_chain *hc_bucket[HASH_CONS_SIZE];
extern struct hc_chain *hc_c;
#else
extern struct value *v_head;
#endif

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
value_mark(struct value *v)
{
	struct list *l;

	if (v == NULL || v->admin & ADMIN_MARKED || v->admin & ADMIN_PERMANENT)
		return;

#ifdef DEBUG
	if (trace_gc > 1) {
		printf("[GC] MARKING VALUE ");
		value_print(v);
		printf(" AS REACHABLE\n");
	}
#endif

	v->admin |= ADMIN_MARKED;
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

	if (a == NULL)
		return;
	if (a->admin & AR_ADMIN_MARKED || a->admin & AR_ADMIN_ON_STACK) {
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
gc(struct vm *vm)
{
	struct activation *a, *a_next;
	struct activation *ta_head = NULL;
	struct value *v;
	struct value **vsc;
#ifdef HASH_CONSING
	struct hc_chain **hccp;
	struct hc_chain *hc_next, *hc_prev;
#else
	struct value *v_next, *tv_head = NULL;
#endif

	/*
	 * Mark...
	 */
	activation_mark(vm->current_ar);
	for (vsc = vm->vstack; vsc < vm->vstack_ptr; vsc++)
		value_mark(*vsc);

	/*
	 * ...and sweep
	 */
	for (a = a_head; a != NULL; a = a_next) {
		a_next = a->next;
		if (a->admin & AR_ADMIN_MARKED || a->admin & AR_ADMIN_ON_STACK) {
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

#ifdef HASH_CONSING
	for (hccp = hc_bucket; hccp - hc_bucket < HASH_CONS_SIZE; hccp++) {
		hc_prev = NULL;
		for (hc_c = *hccp; hc_c != NULL; hc_c = hc_next) {
			hc_next = hc_c->next;
			v = hc_c->v;
			if (v->admin & ADMIN_MARKED || v->admin & ADMIN_PERMANENT) {
				v->admin &= ~ADMIN_MARKED;
				hc_prev = hc_c;
			} else {
#ifdef DEBUG
				if (trace_gc > 1) {
					printf("[GC] FOUND UNREACHABLE VALUE ");
					value_print(v);
					printf("\n");
				}
#endif
				value_free(v);

				if (hc_prev != NULL)
					hc_prev->next = hc_next;
				else
					*hccp = hc_next;
				bhuna_free(hc_c);
			}
		}
	}
#else
	for (v = v_head; v != NULL; v = v_next) {
		v_next = v->next;
		if (v->admin & ADMIN_MARKED || v->admin & ADMIN_PERMANENT) {
			v->admin &= ~ADMIN_MARKED;
			v->next = tv_head;
			tv_head = v;
		} else {
#ifdef DEBUG
			if (trace_gc > 1) {
				printf("[GC] FOUND UNREACHABLE VALUE ");
				value_print(v);
				printf("\n");
			}
#endif
			value_free(v);
		}
	}

	v_head = tv_head;
#endif
#ifdef POOL_VALUES
	clean_pools();
#endif
}
