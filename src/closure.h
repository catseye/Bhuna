#ifndef __CLOSURE_H_
#define __CLOSURE_H_

#include "vm.h"

struct activation;
struct ast;

struct closure {
	struct ast		*ast;
	struct activation	*ar;	/* env in which we were created */
};

struct closure	*closure_new(struct ast *, struct activation *);
void		 closure_free(struct closure *);
void		 closure_dump(struct closure *);

#endif
