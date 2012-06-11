#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "closure.h"
#include "symbol.h"
#include "ast.h"
#include "activation.h"
#include "vm.h"

struct closure *
closure_new(struct ast *a, struct activation *ar, int arity, int cc)
{
	struct closure *c;

	c = bhuna_malloc(sizeof(struct closure));
	c->ast = a;
	c->label = NULL;
	c->ar = ar;
	c->arity = arity;
	c->cc = cc;

	return(c);
}

void
closure_free(struct closure *c)
{
	/*activation_release(c->ar);*/
	bhuna_free(c);
}

void
closure_dump(struct closure *c)
{
#ifdef DEBUG
	printf("closure(arity=%d,contains=%d){", c->arity, c->cc);
	activation_dump(c->ar, 0);
	ast_dump(c->ast, 0);
	printf("}");
#endif
}
