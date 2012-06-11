#ifndef __CLOSURE_H_
#define __CLOSURE_H_

#include "vm.h"

struct activation;
struct ast;

struct closure {
	struct ast		*ast;
	vm_label_t		 label;
	struct activation	*ar;	/* env in which we were created */
	int			 arity;	/* takes this many arguments */
	int			 cc;	/* contains this many sub-closures */
};

struct closure	*closure_new(struct ast *, struct activation *, int, int);
void		 closure_free(struct closure *);
void		 closure_dump(struct closure *);

#endif
