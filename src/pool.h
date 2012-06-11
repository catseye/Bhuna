#include "value.h"

struct pooled_value {
	union {
		struct value		 v;
		struct pooled_value	*free;
	}				 pv;
};

#define PAGE_SIZE 16384
#define HEADER_SIZE sizeof(struct value_pool *)
#define	VALUES_PER_POOL	(PAGE_SIZE - HEADER_SIZE) / sizeof(struct pooled_value) - 1

struct value_pool {
	struct value_pool *	prev;
	struct pooled_value	pool[VALUES_PER_POOL];
};

void			 value_pool_new(void);

struct value		*value_allocate(void);
void			 value_deallocate(struct value *);
