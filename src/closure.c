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
closure_new(struct ast *a, struct activation *ar)
{
	struct closure *c;

	c = bhuna_malloc(sizeof(struct closure));
	c->ast = a;
	c->ar = ar;

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
	printf("closure{");
	activation_dump(c->ar, 1);
	ast_dump(c->ast, 1);
	printf("}");
#endif
}
