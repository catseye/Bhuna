#include "value.h"

struct lifeguard {
	struct value_pool	*next;
	struct value_pool	*prev;
};

#define PAGE_SIZE 16384
#define	VALUES_PER_POOL	((PAGE_SIZE - sizeof(struct lifeguard)) / \
			sizeof(struct value) - 1)

struct value_pool {
	struct lifeguard	lg;
	struct value	 	pool[VALUES_PER_POOL];
};

void			 value_pool_new(void);

struct value		*value_allocate(void);
void			 value_deallocate(struct value *);

void			 pool_report(void);
void			 clean_pools(void);
