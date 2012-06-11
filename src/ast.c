#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "list.h"
#include "value.h"
#include "builtin.h"
#include "activation.h"
#include "vm.h"

#ifdef DEBUG
#include "symbol.h"
#endif

extern unsigned char program[];

/***** constructors *****/

struct ast *
ast_new(int type)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = type;
	a->label = NULL;

	return(a);
}

struct ast *
ast_new_local(int index, int upcount, void *sym)
{
	struct ast *a;

	a = ast_new(AST_LOCAL);
	a->u.local.index = index;
	a->u.local.upcount = upcount;
#ifdef DEBUG
	a->u.local.sym = sym;
#endif

	return(a);
}

struct ast *
ast_new_value(struct value *v)
{
	struct ast *a;

	a = ast_new(AST_VALUE);
	value_grab(v);
	a->u.value.value = v;

	return(a);
}

struct ast *
ast_new_builtin(struct builtin *bi, struct ast *right)
{
	struct ast *a;

	/*
	 * Fold constants.
	 */
	if (bi->is_pure && ast_is_constant(right)) {
		struct value *v = NULL;
		struct activation *ar;
		struct ast *g;
		int i = 0;
		int varity;

		if (bi->arity == -1) {
			varity = ast_count_args(right);
		} else {
			varity = bi->arity;
		}
		ar = activation_new_on_stack(varity, NULL, NULL);
		for (g = right, i = 0;
		     g != NULL && g->type == AST_ARG && i < varity;
		     g = g->u.arg.right, i++) {
			if (g->u.arg.left != NULL)
				activation_initialize_value(ar, i,
				     g->u.arg.left->u.value.value);
		}
		bi->fn(ar, &v);
		activation_free_from_stack(ar);
		a = ast_new_value(v);
		value_release(v);

		return(a);
	}

	a = ast_new(AST_BUILTIN);

	a->u.builtin.bi = bi;
	a->u.builtin.right = right;

	return(a);
}

struct ast *
ast_new_apply(struct ast *fn, struct ast *args, int is_pure)
{
	struct ast *a;

	a = ast_new(AST_APPLY);
	a->u.apply.left = fn;
	a->u.apply.right = args;
	a->u.apply.is_pure = is_pure;

	return(a);
}

struct ast *
ast_new_arg(struct ast *left, struct ast *right)
{
	struct ast *a;

	a = ast_new(AST_ARG);
	a->u.arg.left = left;
	a->u.arg.right = right;

	return(a);
}

struct ast *
ast_new_routine(int arity, int locals, int cc, struct ast *body)
{
	struct ast *a;

	a = ast_new(AST_ROUTINE);
	a->u.routine.arity = arity;
	a->u.routine.locals = locals;
	a->u.routine.cc = cc;
	a->u.routine.body = body;

	return(a);
}

struct ast *
ast_new_statement(struct ast *left, struct ast *right)
{
	struct ast *a;

	if (left == NULL && right == NULL)
		return(NULL);
	if (left == NULL)
		return(right);
	if (right == NULL)
		return(left);

	a = ast_new(AST_STATEMENT);
	a->u.statement.left = left;
	a->u.statement.right = right;

	return(a);
}

struct ast *
ast_new_assignment(struct ast *left, struct ast *right)
{
	struct ast *a;

	a = ast_new(AST_ASSIGNMENT);
	a->u.assignment.left = left;
	a->u.assignment.right = right;

	return(a);
}

struct ast *
ast_new_conditional(struct ast *test, struct ast *yes, struct ast *no)
{
	struct ast *a;

	a = ast_new(AST_CONDITIONAL);
	a->u.conditional.test = test;
	a->u.conditional.yes = yes;
	a->u.conditional.no = no;

	return(a);
}

struct ast *
ast_new_while_loop(struct ast *test, struct ast *body)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = AST_WHILE_LOOP;

	a->u.while_loop.test = test;
	a->u.while_loop.body = body;

	return(a);
}

struct ast *
ast_new_retr(struct ast *body)
{
	struct ast *a;

	a = ast_new(AST_RETR);
	a->u.retr.body = body;

	return(a);
}

/*** DESTRUCTOR ***/

void
ast_free(struct ast *a)
{
	if (a == NULL) {
		return;
	}
	switch (a->type) {
	case AST_LOCAL:
		break;
	case AST_VALUE:
		value_release(a->u.value.value);
		break;
	case AST_BUILTIN:
		ast_free(a->u.builtin.right);
		break;
	case AST_APPLY:
		ast_free(a->u.apply.left);
		ast_free(a->u.apply.right);
		break;
	case AST_ARG:
		ast_free(a->u.arg.left);
		ast_free(a->u.arg.right);
		break;
	case AST_ROUTINE:
		ast_free(a->u.routine.body);
		break;
	case AST_STATEMENT:
		ast_free(a->u.statement.left);
		ast_free(a->u.statement.right);
		break;
	case AST_ASSIGNMENT:
		ast_free(a->u.assignment.left);
		ast_free(a->u.assignment.right);
		break;
	case AST_CONDITIONAL:
		ast_free(a->u.conditional.test);
		ast_free(a->u.conditional.yes);
		ast_free(a->u.conditional.no);
		break;
	case AST_WHILE_LOOP:
		ast_free(a->u.while_loop.test);
		ast_free(a->u.while_loop.body);
		break;
	case AST_RETR:
		ast_free(a->u.retr.body);
		break;
	}
	free(a);
}

/*** PREDICATES &c. ***/

int
ast_is_constant(struct ast *a)
{
	if (a == NULL)
		return(1);
	switch (a->type) {
	case AST_VALUE:
		return(1);
	case AST_ARG:
		return(ast_is_constant(a->u.arg.left) &&
		       ast_is_constant(a->u.arg.right));
	}
	return(0);
}

int
ast_count_args(struct ast *a)
{
	int ac;

	for (ac = 0; a != NULL && a->type == AST_ARG; a = a->u.arg.right, ac++)
		;

	return(ac);
}

/*** DEBUGGING ***/

char *
ast_name(struct ast *a)
{
#ifdef DEBUG
	if (a == NULL)
		return("(null)");
	switch (a->type) {
	case AST_LOCAL:
		return("AST_LOCAL");
	case AST_VALUE:
		return("AST_VALUE");
	case AST_BUILTIN:
		return("AST_BUILTIN");
	case AST_APPLY:
		return("AST_APPLY");
	case AST_ARG:
		return("AST_ARG");
	case AST_ROUTINE:
		return("AST_ROUTINE");
	case AST_STATEMENT:
		return("AST_STATEMENT");
	case AST_ASSIGNMENT:
		return("AST_ASSIGNMENT");
	case AST_CONDITIONAL:
		return("AST_CONDITIONAL");
	case AST_WHILE_LOOP:
		return("AST_WHILE_LOOP");
	case AST_RETR:
		return("AST_RETR");
	}
#endif
	return("AST_UNKNOWN??!?");
}

void
ast_dump(struct ast *a, int indent)
{
#ifdef DEBUG
	int i;

	if (a == NULL) {
		return;
	}
	for (i = 0; i < indent; i++) printf("  ");
	if (a->label != NULL) {
		printf("@#%d -> ", a->label - (vm_label_t)program);
	}
	switch (a->type) {
	case AST_LOCAL:
		printf("local(%d,%d)=", a->u.local.index, a->u.local.upcount);
		if (a->u.local.sym != NULL)
			symbol_dump(a->u.local.sym, 0);
		printf("\n");
		break;
	case AST_VALUE:
		printf("value(");
		value_print(a->u.value.value);
		printf(")\n");
		break;
	case AST_BUILTIN:
		printf("builtin `%s`{\n", a->u.builtin.bi->name);
		ast_dump(a->u.builtin.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_APPLY:
		printf("apply {\n");
		ast_dump(a->u.apply.left, indent + 1);
		ast_dump(a->u.apply.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_ARG:
		printf("arg {\n");
		ast_dump(a->u.arg.left, indent + 1);
		ast_dump(a->u.arg.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_ROUTINE:
		printf("routine/%d (contains %d) {\n",
		    a->u.routine.arity, a->u.routine.cc);
		ast_dump(a->u.routine.body, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_STATEMENT:
		printf("statement {\n");
		ast_dump(a->u.statement.left, indent + 1);
		ast_dump(a->u.statement.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_ASSIGNMENT:
		printf("assign {\n");
		ast_dump(a->u.assignment.left, indent + 1);
		ast_dump(a->u.assignment.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_CONDITIONAL:
		printf("conditional {\n"); /* a->u.conditional.index); */
		ast_dump(a->u.conditional.test, indent + 1);
		ast_dump(a->u.conditional.yes, indent + 1);
		if (a->u.conditional.no != NULL)
			ast_dump(a->u.conditional.no, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_WHILE_LOOP:
		printf("while {\n");
		ast_dump(a->u.while_loop.test, indent + 1);
		ast_dump(a->u.while_loop.body, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_RETR:
		printf("retr {\n");
		ast_dump(a->u.retr.body, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	}
#endif
}
