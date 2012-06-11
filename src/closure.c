#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "closure.h"
#include "symbol.h"
#include "ast.h"
#include "activation.h"

struct closure *
closure_new(struct ast *a, struct activation *ar)
{
	struct closure *c;

	assert(a->type == AST_SCOPE);

	c = bhuna_malloc(sizeof(struct closure));
	c->ast = a;
	c->ar = ar;

	return(c);
}

void
closure_free(struct closure *c)
{
	activation_release(c->ar);
	bhuna_free(c);
}

void
closure_dump(struct closure *c)
{
#ifdef DEBUG
	printf("closure{");
	ast_dump(c->ast, 0);
	activation_dump(c->ar, 0);
	printf("}");
#endif
}
