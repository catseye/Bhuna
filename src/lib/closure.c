#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "closure.h"
#include "symbol.h"
#include "ast.h"
#include "activation.h"
#include "vm.h"

#include "type.h"

struct closure *
closure_new(struct ast *a, struct activation *ar, int arity, int locals, int cc)
{
	struct closure *c;

	c = bhuna_malloc(sizeof(struct closure));

	c->ast = a;
	c->ar = ar;
	c->arity = arity;
	c->locals = locals;
	c->cc = cc;

	return(c);
}

void
closure_free(struct closure *c)
{
	bhuna_free(c);
}

void
closure_dump(struct closure *c)
{
#ifdef DEBUG
	printf("closure/%d (l=%d,cc=%d){", c->arity, c->locals, c->cc);
	activation_dump(c->ar, 1);
	/*ast_dump(c->ast, 1);*/	/* XXX for now... */
	printf("}");
#endif
}
