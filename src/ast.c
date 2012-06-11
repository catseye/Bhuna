#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "list.h"
#include "symbol.h"
#include "value.h"

/***** constructors *****/

struct ast *
ast_new_sym(struct symbol *s)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = AST_SYM;

	a->u.sym.sym = s;

	return(a);
}

struct ast *
ast_new_value(struct value *v)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = AST_VALUE;

	value_grab(v);
	a->u.value.value = v;

	return(a);
}

struct ast *
ast_new_scope(struct ast *body, struct symbol_table *local)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = AST_SCOPE;

	a->u.scope.body = body;

	a->u.scope.local = local;

	return(a);
}

struct ast *
ast_new_apply(struct ast *fn, struct ast *args)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = AST_APPLY;

	a->u.apply.left = fn;
	a->u.apply.right = args;

	return(a);
}

struct ast *
ast_new_arg(struct ast *left, struct ast *right)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = AST_ARG;

	a->u.arg.left = left;
	a->u.arg.right = right;

	return(a);
}

struct ast *
ast_new_statement(struct ast *left, struct ast *right)
{
	struct ast *a;

	/*
	if (left == NULL && right == NULL)
		return(NULL);
	if (left == NULL)
		return(right);
	if (right == NULL)
		return(left);
	*/

	a = malloc(sizeof(struct ast));
	a->type = AST_STATEMENT;

	a->u.statement.left = left;
	a->u.statement.right = right;

	return(a);
}

struct ast *
ast_new_assignment(struct ast *left, struct ast *right)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = AST_ASSIGNMENT;

	a->u.assignment.left = left;
	a->u.assignment.right = right;

	return(a);
}

struct ast *
ast_new_conditional(struct ast *test, struct ast *yes, struct ast *no)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = AST_CONDITIONAL;

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

void
ast_free(struct ast *a)
{
	if (a == NULL) {
		return;
	}
	switch (a->type) {
	case AST_SYM:
		break;
	case AST_VALUE:
		value_release(a->u.value.value);
		break;
	case AST_SCOPE:
		ast_free(a->u.scope.body);
		break;
	case AST_APPLY:
		ast_free(a->u.apply.left);
		ast_free(a->u.apply.right);
		break;
	case AST_ARG:
		ast_free(a->u.arg.left);
		ast_free(a->u.arg.right);
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
	}
	free(a);
}

char *
ast_name(struct ast *a)
{
#ifdef DEBUG
	if (a == NULL)
		return("(null)");
	switch (a->type) {
	case AST_SYM:
		return("AST_SYM");
	case AST_VALUE:
		return("AST_VALUE");
	case AST_SCOPE:
		return("AST_SCOPE");
	case AST_APPLY:
		return("AST_APPLY");
	case AST_ARG:
		return("AST_ARG");
	case AST_STATEMENT:
		return("AST_STATEMENT");
	case AST_ASSIGNMENT:
		return("AST_ASSIGNMENT");
	case AST_CONDITIONAL:
		return("AST_CONDITIONAL");
	case AST_WHILE_LOOP:
		return("AST_WHILE_LOOP");
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
	switch (a->type) {
	case AST_SYM:
		printf("symbol(");
		symbol_dump(a->u.sym.sym, 0);
		printf(")\n");
		break;
	case AST_VALUE:
		printf("value(");
		value_print(a->u.value.value);
		printf(")\n");
		break;
	case AST_SCOPE:
		printf("scope {\n");
		symbol_table_dump(a->u.scope.local, 0);
		/*symbol_table_dump(a->u.scope.trap, 0);*/
		ast_dump(a->u.scope.body, indent + 1);
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
		printf("conditional {\n");
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
		break;
	}
#endif
}
