#ifdef POOL_VALUES
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "pool.h"

static struct value_pool *current_vp = NULL;
static struct value *pv_free = NULL;

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
		printf("page size         = %4d\n", PAGE_SIZE);
		printf("header size       = %4d\n", sizeof(struct lifeguard));
		printf("value pool size   = %4d\n", sizeof(struct value_pool));
		printf("values per pool   = %4d\n", VALUES_PER_POOL);
	}
#endif
	assert(sizeof(struct value_pool) <= PAGE_SIZE);
	vp = malloc(PAGE_SIZE);

	/*
	 * Link us up to the parent pool, if any.
	 */
	if (current_vp != NULL)
		current_vp->lg.next = vp;
	vp->lg.prev = current_vp;
	vp->lg.next = NULL;

	/*
	 * Create an initial freelist within the pool,
	 * and link it into our global freelist pv_free.
	 */
	vp->pool[0].admin = ADMIN_FREE;
	vp->pool[0].next = pv_free;
	for (i = 1; i < VALUES_PER_POOL; i++) {
		vp->pool[i].admin = ADMIN_FREE;
		vp->pool[i].next = &vp->pool[i-1];
	}
	pv_free = &vp->pool[VALUES_PER_POOL - 1];

	/*printf("pv_free = %08lx\n", (unsigned long)pv_free);*/

	current_vp = vp;
}

struct value *
value_allocate(void)
{
	struct value *r;

	/*
	 * Check to see if this is the last remaining slot in the pool.
	 */
	if (pv_free->next == NULL) {
		/*
		 * If so, create a new pool.
		 */
		value_pool_new();
	}
	/*
	 * Find the next pooled value on the freelist,
	 * remove it from the freelist, and return it.
	 */
	r = pv_free;
	pv_free = pv_free->next;

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
	v->next = pv_free;
	v->admin = ADMIN_FREE;
	pv_free = v;
#ifdef DEBUG
	if (trace_pool > 1) {
		printf("pool: deallocate: next free now %08lx\n",
		    (unsigned long)pv_free);
	}
#endif
}

void
pool_report(void)
{
#ifdef DEBUG
	struct value_pool *vp;
	int pool_no = 1;
	int i, nf;

	for (vp = current_vp; vp != NULL; vp = vp->lg.prev) {
		nf = 0;
		for (i = 0; i < VALUES_PER_POOL; i++) {
			if (vp->pool[i].admin & ADMIN_FREE)
				nf++;
		}
		printf("Pool #%d: %5d/%5d\n", pool_no++, nf, VALUES_PER_POOL);
	}
#endif
}

void
clean_pools(void)
{
	struct value_pool *vp, *vp_prev;
	int pool_no = 1, i, nf;
	struct value *save = NULL;

	/* re-thread the free list! */

	pv_free = NULL;
	for (vp = current_vp; vp != NULL; vp = vp_prev) {
		vp_prev = vp->lg.prev;
		save = pv_free;
		nf = 0;
		for (i = 0; i < VALUES_PER_POOL; i++) {
			/*
			 * If this space is taken - skip it.
			 */
			if (!(vp->pool[i].admin & ADMIN_FREE))
				continue;

			/*
			 * Otherwise re-thread the free list through here.
			 */
			vp->pool[i].next = pv_free;
			pv_free = &vp->pool[i];
			nf++;
		}
		if (nf == VALUES_PER_POOL) {
			/*printf("Pool #%d is EMPTY, DESTROYING\n", pool_no);*/
			pv_free = save;
			if (vp->lg.prev != NULL)
				vp->lg.prev->lg.next = vp->lg.next;
			if (vp->lg.next != NULL)
				vp->lg.next->lg.prev = vp->lg.prev;
			if (vp == current_vp)
				current_vp = vp_prev;
			bhuna_free(vp);
		} else {
			/*printf("Pool #%d is not empty\n", pool_no);*/
		}
		pool_no++;
	}
}
#endif
