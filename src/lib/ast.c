#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "ast.h"
#include "list.h"
#include "value.h"
#include "builtin.h"
#include "activation.h"
#include "vm.h"
#include "type.h"
#include "scan.h"
#include "utf8.h"

#include "symbol.h"
#include "report.h"

extern int trace_type_inference;

/***** constructors *****/

struct ast *
ast_new(int type)
{
	struct ast *a;

	a = malloc(sizeof(struct ast));
	a->type = type;
	a->sc = NULL;
	a->label = NULL;
	a->datatype = NULL;

	return(a);
}

struct ast *
ast_new_local(struct symbol_table *stab, struct symbol *sym)
{
	struct ast *a;

	a = ast_new(AST_LOCAL);
	a->u.local.index = sym->index;
	a->u.local.upcount = stab->level - sym->in->level;
	a->u.local.sym = sym;
	a->datatype = sym->type;

#ifdef DEBUG
	if (trace_type_inference) {
		printf("(new-local)*****\n");
		printf("type is: ");
		type_print(stdout, a->datatype);
		printf("\n*******\n");
	}
#endif

	return(a);
}

struct ast *
ast_new_value(struct value v, struct type *t)
{
	struct ast *a;

	a = ast_new(AST_VALUE);
	a->u.value.value = v;
	a->datatype = t;

#ifdef DEBUG
	if (trace_type_inference) {
		printf("(value)*****\n");
		printf("type is: ");
		type_print(stdout, a->datatype);
		printf("\n*******\n");
	}
#endif

	return(a);
}

struct ast *
ast_new_builtin(struct scan_st *sc, struct builtin *bi, struct ast *right)
{
	struct ast *a;
	struct type *t, *tr;
	int unify = 0;

	t = bi->ty();
	type_ensure_routine(t);

#ifdef DEBUG
	if (trace_type_inference) {
		printf("(builtin `");
		fputsu8(stdout, bi->name);
		printf("`)*****\n");
		printf("type of args is: ");
		if (right != NULL)
			type_print(stdout, right->datatype);
		printf("\ntype of builtin is: ");
		type_print(stdout, t);
	}
#endif

	if (right == NULL)
		tr = type_new(TYPE_VOID);
	else
		tr = right->datatype;

	unify = type_unify_crit(sc,
	    type_representative(t)->t.closure.domain, tr);

#ifdef DEBUG
	if (trace_type_inference) {
		printf("\nthese unify? --> %d <--", unify);
		printf("\n****\n");
	}
#endif

	/*
	 * Fold constants.
	 */
	if (bi->is_pure && ast_is_constant(right)) {
		struct value v;
		struct activation *ar;
		struct ast *g;
		int i = 0;
		int varity;

		if (bi->arity == -1) {
			varity = ast_count_args(right);
		} else {
			varity = bi->arity;
		}

		if (unify) {
			ar = activation_new_on_heap(varity, NULL, NULL);
			for (g = right, i = 0;
			     g != NULL && g->type == AST_ARG && i < varity;
			     g = g->u.arg.right, i++) {
				if (g->u.arg.left != NULL)
					activation_initialize_value(ar, i,
					     g->u.arg.left->u.value.value);
			}
			v = bi->fn(ar);
		} else {
			a = NULL;
		}

		a = ast_new_value(v, type_representative(t)->t.closure.range);

		return(a);
	}

	a = ast_new(AST_BUILTIN);

	a->u.builtin.bi = bi;
	a->u.builtin.right = right;
	a->datatype = type_representative(t)->t.closure.range;

	return(a);
}

struct ast *
ast_new_apply(struct scan_st *sc, struct ast *fn, struct ast *args, int is_pure)
{
	struct ast *a;
	int unify;

	a = ast_new(AST_APPLY);
	a->u.apply.left = fn;
	a->u.apply.right = args;
	a->u.apply.is_pure = is_pure;

	type_ensure_routine(fn->datatype);

#ifdef DEBUG
	if (trace_type_inference) {
		printf("(apply)*****\n");
		printf("type of args is: ");
		if (args == NULL) printf("N/A"); else type_print(stdout, args->datatype);
		printf("\ntype of closure is: ");
		type_print(stdout, fn->datatype);
	}
#endif

	if (args == NULL) {
		unify = type_unify_crit(sc,
		    type_representative(fn->datatype)->t.closure.domain,
		    type_new(TYPE_VOID));	/* XXX need not be new */
	} else {
		unify = type_unify_crit(sc,
		    type_representative(fn->datatype)->t.closure.domain,
		    args->datatype);
	}

#ifdef DEBUG
	if (trace_type_inference) {
		printf("\nthese unify? --> %d <--", unify);
		printf("\n****\n");
	}
#endif

	a->datatype = type_representative(fn->datatype)->t.closure.range;

	return(a);
}

struct ast *
ast_new_arg(struct ast *left, struct ast *right)
{
	struct ast *a;

	if (left == NULL)
		return(NULL);

	a = ast_new(AST_ARG);
	a->u.arg.left = left;
	a->u.arg.right = right;
	if (a->u.arg.right == NULL) {
		a->datatype = a->u.arg.left->datatype;
	} else {
		a->datatype = type_new_arg(
		    a->u.arg.left->datatype,
		    a->u.arg.right->datatype);
	}
	return(a);
}

struct ast *
ast_new_routine(struct ast *body)
{
	struct ast *a;

	a = ast_new(AST_ROUTINE);
	a->u.routine.body = body;
	
	if (a->u.routine.body != NULL)
		a->datatype = a->u.routine.body->datatype;
	else
		a->datatype = type_new(TYPE_VOID);

#ifdef DEBUG
	if (trace_type_inference) {
		printf("(routine)*****\n");
		printf("type is: ");
		type_print(stdout, a->datatype);
		printf("\n****\n");
	}
#endif

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
	/* XXX check that a->u.statement.left->datatype is VOID ?? */
	a->datatype = a->u.statement.right->datatype;
	/* haha... */
	/*
	a->datatype = type_new_set(a->u.statement.left->datatype,
	    a->u.statement.right->datatype);
	*/

#ifdef DEBUG
	if (trace_type_inference) {
		printf("(statement)*****\n");
		printf("type is: ");
		type_print(stdout, a->datatype);
		printf("\n****\n");
	}
#endif

	return(a);
}

struct ast *
ast_new_assignment(struct scan_st *sc, struct ast *left, struct ast *right,
		   int defining)
{
	struct ast *a;
	int unify;

	/*
	 * Do some 'self-repairing' in the case of syntax errors that
	 * generate a corrupt AST (e.g. Foo = <eof>)
	 */
	if (right == NULL)
		return(left);

	a = ast_new(AST_ASSIGNMENT);
	a->u.assignment.left = left;
	a->u.assignment.right = right;
	a->u.assignment.defining = defining;

	unify = type_unify_crit(sc, left->datatype, right->datatype);

#ifdef DEBUG
	if (trace_type_inference) {
		printf("(assign)*****\n");
		printf("type of LHS is: ");
		type_print(stdout, left->datatype);
		printf("\ntype of RHS is: ");
		type_print(stdout, right->datatype);
		printf("\nthese unify? --> %d <--", unify);
		printf("\ntype of LHS is now: ");
		type_print(stdout, left->datatype);
		printf("\n****\n");
	}
#endif
	a->datatype = type_new(TYPE_VOID);

	return(a);
}

struct ast *
ast_new_conditional(struct scan_st *sc, struct ast *test, struct ast *yes, struct ast *no)
{
	struct ast *a;
	int unify;
	struct type *t;

	a = ast_new(AST_CONDITIONAL);
	a->u.conditional.test = test;
	a->u.conditional.yes = yes;
	a->u.conditional.no = no;
	/* check that a->u.conditional.test is BOOLEAN */

	/* XXX need not be new boolean - reuse an old one */
	unify = type_unify_crit(sc, test->datatype, type_new(TYPE_BOOLEAN));

#ifdef DEBUG
	if (trace_type_inference) {
		printf("(if)*****\n");
		printf("type of YES is: ");
		type_print(stdout, yes->datatype);
		if (no != NULL) {
			printf("\ntype of NO is: ");
			type_print(stdout, no->datatype);
		}
	}
#endif

	/* XXX check that a->u.conditional.yes is VOID */
	/* XXX check that a->u.conditional.no is VOID */

	/* actually, either of these can be VOID, in which case, pick the other */
	/* unify = type_unify_crit(sc, yes->datatype, no->datatype); */
	/* haha */
	if (no == NULL) {
		t = type_new(TYPE_VOID);
	} else {
		t = a->u.conditional.no->datatype;
	}
	a->datatype = type_new_set(a->u.conditional.yes->datatype, t);

#ifdef DEBUG
	if (trace_type_inference) {
		printf("\nresult type is: ");
		type_print(stdout, a->datatype);
		printf("\n****\n");
	}
#endif

	return(a);
}

struct ast *
ast_new_while_loop(struct scan_st *sc, struct ast *test, struct ast *body)
{
	struct ast *a;
	int unify;

	a = malloc(sizeof(struct ast));
	a->type = AST_WHILE_LOOP;

	a->u.while_loop.test = test;
	a->u.while_loop.body = body;
	/* XXX need not be new boolean - reuse an old one */
	unify = type_unify_crit(sc, test->datatype, type_new(TYPE_BOOLEAN));

	/* XXX check that a->u.while_loop.body is VOID */
	/* a->datatype = type_new(TYPE_VOID); */
	a->datatype = body->datatype;

	return(a);
}

struct ast *
ast_new_retr(struct ast *body)
{
	struct ast *a;

	a = ast_new(AST_RETR);
	a->u.retr.body = body;
	/* XXX check against other return statements in same function... somehow... */
	a->datatype = a->u.retr.body->datatype;

#ifdef DEBUG
	if (trace_type_inference) {
		printf("(retr)*****\n");
		printf("type is: ");
		type_print(stdout, a->datatype);
		printf("\n****\n");
	}
#endif

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
	if (a->sc != NULL)
		scan_close(a->sc);
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

/*
 * This is a rather specialized function to find the l-value of a store builtin.
 */
struct ast *
ast_find_local(struct ast *a)
{
	while (a->type != AST_LOCAL && a != NULL) {
		if (a->type == AST_BUILTIN)
			a = a->u.builtin.right;
		else if (a->type == AST_ARG)
			a = a->u.arg.left;
		else
			report(REPORT_ERROR, NULL, "wtf malformed AST");
	}

	return(a);
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
	return("AST_UNKNOWN?");
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
		/* XXX
		printf("@#%d -> ", a->label - (vm_label_t)program);
		*/
		printf("@#%08lx -> ", (unsigned long)a->label);
	}
	printf(ast_name(a));
	printf("=");
	type_print(stdout, a->datatype);
	switch (a->type) {
	case AST_LOCAL:
		printf("(%d,%d)=", a->u.local.index, a->u.local.upcount);
		if (a->u.local.sym != NULL)
			symbol_dump(a->u.local.sym, 0);
		printf("\n");
		break;
	case AST_VALUE:
		printf("(");
		value_print(a->u.value.value);
		printf(")\n");
		break;
	case AST_BUILTIN:
		printf("`");
		fputsu8(stdout, a->u.builtin.bi->name);
		printf("`{\n");
		ast_dump(a->u.builtin.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_APPLY:
		printf("{\n");
		ast_dump(a->u.apply.left, indent + 1);
		ast_dump(a->u.apply.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_ARG:
		printf("{\n");
		ast_dump(a->u.arg.left, indent + 1);
		ast_dump(a->u.arg.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_ROUTINE:
		printf("{\n");
		ast_dump(a->u.routine.body, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_STATEMENT:
		printf("{\n");
		ast_dump(a->u.statement.left, indent + 1);
		ast_dump(a->u.statement.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_ASSIGNMENT:
		printf("(%s){\n", a->u.assignment.defining ?
		    "definition" : "application");
		ast_dump(a->u.assignment.left, indent + 1);
		ast_dump(a->u.assignment.right, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_CONDITIONAL:
		printf("{\n");
		ast_dump(a->u.conditional.test, indent + 1);
		ast_dump(a->u.conditional.yes, indent + 1);
		if (a->u.conditional.no != NULL)
			ast_dump(a->u.conditional.no, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_WHILE_LOOP:
		printf("{\n");
		ast_dump(a->u.while_loop.test, indent + 1);
		ast_dump(a->u.while_loop.body, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	case AST_RETR:
		printf("{\n");
		ast_dump(a->u.retr.body, indent + 1);
		for (i = 0; i < indent; i++) printf("  "); printf("}\n");
		break;
	}
#endif
}
