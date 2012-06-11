#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "pool.h"

static struct value_pool *current_vp = NULL;
static struct pooled_value *pv_free = NULL;

extern int trace_pool;

void
value_pool_new(void)
{
	struct value_pool *vp;
	int i;

#ifdef DEBUG
	if (trace_pool > 0) {
		printf("MAKING NEW POOL\n");
		printf("value size        = %4d\n", sizeof(struct value));
		printf("pooled-value size = %4d\n", sizeof(struct pooled_value));
		printf("page size         = %4d\n", PAGE_SIZE);
		printf("header size       = %4d\n", HEADER_SIZE);
		printf("value pool size   = %4d\n", sizeof(struct value_pool));
		printf("values per pool   = %4d\n", VALUES_PER_POOL);
	}
#endif
	assert(sizeof(struct value_pool) <= PAGE_SIZE);
	vp = malloc(PAGE_SIZE);

	/*
	 * Link us up to the parent pool, if any.
	 */
	vp->prev = current_vp;

	/*
	 * Create an initial freelist within the pool,
	 * and link it into our global freelist pv_free.
	 */
	vp->pool[0].pv.free = pv_free;
	for (i = 1; i < VALUES_PER_POOL; i++)
		vp->pool[i].pv.free = &vp->pool[i-1];
	pv_free = &vp->pool[VALUES_PER_POOL - 1];

	current_vp = vp;
}

struct value *
value_allocate(void)
{
	struct value *r;

	/*
	 * Check to see if this is the last remaining slot in the pool.
	 */
	if (pv_free->pv.free == NULL) {
		/*
		 * If so, create a new pool.
		 */
		value_pool_new();
	}
	/*
	 * Find the next pooled value on the freelist,
	 * remove it from the freelist, and return it.
	 */
	r = &(pv_free->pv.v);
	pv_free = pv_free->pv.free;

#ifdef DEBUG
	if (trace_pool > 1) {
		printf("pool: allocate: next free now %08lx\n",
		    (unsigned long)pv_free);
	}
#endif
	return(r);
}

void
value_deallocate(struct value *v)
{
	/*
	 * Tack this value back onto the freelist.
	 */
	((struct pooled_value *)v)->pv.free = pv_free;
	pv_free = (struct pooled_value *)v;
#ifdef DEBUG
	if (trace_pool > 1) {
		printf("pool: deallocate: next free now %08lx\n",
		    (unsigned long)pv_free);
	}
#endif
}
