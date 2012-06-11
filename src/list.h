#ifndef __LIST_H_
#define	__LIST_H_

#include <sys/types.h>

struct value;

struct list {
	struct list		*next;
	struct value		*value;
};

void		 list_cons(struct list **, struct value *);
struct list	*list_dup(struct list *);
void		 list_free(struct list **);
size_t		 list_length(struct list *);
int		 list_contains(struct list *, struct value *);

void		 list_dump(struct list *);

#endif /* !__LIST_H_ */
