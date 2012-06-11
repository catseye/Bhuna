#ifndef __CLOSURE_H_
#define __CLOSURE_H_

#include "vm.h"

struct activation;
struct ast;

struct closure {
	struct ast		*ast;
	struct activation	*ar;	/* env in which we were created */
	int			 arity;	/* takes this many arguments */
	int			 locals;/* has this many local variables */
	int			 cc;	/* contains this many closures */
};

struct closure	*closure_new(struct ast *, struct activation *, int, int, int);
void		 closure_free(struct closure *);
void		 closure_dump(struct closure *);

#endif
