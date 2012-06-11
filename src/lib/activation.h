#include "value.h"

struct vm;

#define	AR_ADMIN_MARKED		1
#define	AR_ADMIN_ON_STACK	4

/*
 * Structure of an activation record.
 * This is actually only the header;
 * the frame itself (containing local variables)
 * follows immediately in memory.
 */
struct activation {
	struct activation	*next;		/* global list of all act recs */
	unsigned short int	 admin;
	unsigned short int	 size;
	struct activation	*caller;	/* recursively shallower activation record */
	struct activation	*enclosing;	/* lexically enclosing activation record */
	/*
	struct value		  value[];
	*/
};

#define VALARY(a,i)	\
	((struct value *)((unsigned char *)a + sizeof(struct activation)))[i]

struct activation	*activation_new_on_heap(int, struct activation *, struct activation *);
struct activation	*activation_new_on_stack(int, struct activation *, struct activation *, struct vm *);
void			 activation_free_from_heap(struct activation *);
void			 activation_free_from_stack(struct activation *, struct vm *);

struct value		 activation_get_value(struct activation *, int, int);
void			 activation_set_value(struct activation *, int, int, struct value);
void			 activation_initialize_value(struct activation *, int, struct value);

void			 activation_dump(struct activation *, int);
