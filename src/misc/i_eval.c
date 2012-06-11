#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "value.h"
#include "list.h"
#include "closure.h"
#include "activation.h"

#ifdef DEBUG
#include "symbol.h"

extern int trace_assignments;
extern int trace_calls;
extern int trace_ast;
extern int trace_closures;
#endif

extern struct activation *current_ar;

/*** UTILITY ***/

#define MOVE 0
#define QUERY 1

struct stack {
	struct tree *t;
	int state;
} s[256];
int sp = 0;

static void
push(struct ast *t, int state)
{
	if (t == NULL) return;
	s[sp].t = t;
	s[sp].state = state;
	sp++;
}	

static void
pop(struct ast **t, int *state)
{
	--sp;
	*t = s[sp].t;
	*state = s[sp].state;
}	

/*** OPERATIONS ***/

/*
 * Fill out an activation record with arguments.
 * !!!!!!!!ar in here is part of the G.D. root set!!!!!!!!!!!
 */
static void
ast_fillout(struct ast *a, struct activation *ar, int *idxp)
{
	struct value *v = NULL;

	if (a == NULL)
		return;

	switch (a->type) {
	case AST_ARG:
		ast_fillout(a->u.arg.left, ar, idxp);
		ast_fillout(a->u.arg.right, ar, idxp);
		break;
	default:
		ast_eval(a, &v);

		activation_set_value(ar, (*idxp)++, 0, v);
		value_release(v);
	}
}

/*** EVALUATOR ***/

/*
 * a is roughly analogous to the program counter (PC.)
 *
 * v is roughly analogous to the accumulator (A) or top of stack (ToS).
 */
void
ast_eval(struct ast *a, struct value **v)
{
	int state;

	push(a, MOVE);
	
	while (sp != 0) {
		pop(&a, &state);
		if (state == MOVE) {
			switch (a->type) {
				case AST_LOCAL:
				case AST_VALUE:
				case AST_SCOPE:
					ast_eval(a->u.scope.body, v);
					break;
				case AST_APPLY:
					ast_eval(a->u.apply.left, &l);
					ast_eval(l->v.k->ast, v);
					break;
				case AST_ARG:
					assert("this should never happen" == NULL);
					/*value_set_list(v);
					ast_flatten(a, v);*/
					break;
				case AST_STATEMENT:
					push(a, QUERY);
					push(a->u.statement.right, MOVE);
					push(a->u.statement.left, MOVE);
					break;
				case AST_ASSIGNMENT:
					ast_eval(a->u.assignment.right, v);
					break;
				case AST_CONDITIONAL:
					ast_eval(a->u.conditional.test, &l);
					ast_eval(a->u.conditional.yes, v);
					ast_eval(a->u.conditional.no, v);
				case AST_WHILE_LOOP:
					ast_eval(a->u.while_loop.test, &l);
					ast_eval(a->u.while_loop.body, v);
			}

			push(t, QUERY);
			if (t->r != NULL) push(t->r, MOVE);
			if (t->l != NULL) push(t->l, MOVE);
			}
		} else if (state == QUERY) {
			printf(" %c", t->c);
		}
	}
}




void
ast_eval(struct ast *a, struct value **v)
{
	struct value *l = NULL, *lv = NULL;
	struct activation *new_ar = NULL, *old_ar = NULL;
	int i;

	if (a == NULL)
		return;

#ifdef DEBUG
	if (trace_ast) {
		printf(">>> ENTERING %s[0x%08lx]\n", ast_name(a), (unsigned long)a);
	}
#endif

	switch (a->type) {
	case AST_LOCAL:
		/*
		printf("index = %d, upcount = %d\n", 
		    a->u.local.index, a->u.local.upcount);
		activation_dump(current_ar, 0);
		printf("\n");
		*/
		lv = activation_get_value(current_ar,
		    a->u.local.index, a->u.local.upcount);
		assert(lv != NULL);
		value_set_from_value(v, lv);
		/*value_release(lv);*/
		break;
	case AST_VALUE:
		value_set_from_value(v, a->u.value.value);
		if (*v != NULL && (*v)->type == VALUE_CLOSURE) {
#ifdef DEBUG
			if (trace_closures) {
				printf("Freshening ");
				closure_dump((*v)->v.k);
				printf(" with ");
				activation_dump(current_ar, 1);
				printf("\n");
			}
#endif
			/*activation_release((*v)->v.k->ar);
			activation_grab(current_ar);*/
			(*v)->v.k->ar = current_ar;
		}
		/*value_release(a->u.value.value);*/
		break;
	case AST_SCOPE:
		/*
		 * pretty sure we don't need to do anything with the stab, here.
		 * although we may want to note somewhere that this is the
		 * most recently encountered stab.
		 */
		ast_eval(a->u.scope.body, v);
		break;
	case AST_APPLY:
		/*
		 * Get the function we're being asked to apply.
		 */
		ast_eval(a->u.apply.left, &l);

		/*
		 * Create a new activation record apropos to l.
		 * When l is a closure, include the closure's
		 * environment as the lexical link of the record.
		 */
		assert(l->type == VALUE_BUILTIN || l->type == VALUE_CLOSURE);
		old_ar = current_ar;
		if (l->type == VALUE_BUILTIN) {
			new_ar = activation_new(2, current_ar, NULL);	/* haha */
		} else if (l->type == VALUE_CLOSURE) {
			assert(l->v.k->ast->type == AST_SCOPE);
			/* value_print(l); */
			new_ar = activation_new(
			    l->v.k->ast->u.scope.frame_size,
			    current_ar,
			    l->v.k->ar);
		}
		assert(current_ar == old_ar);

		/*
		 * Now, fill out that activation record with the
		 * supplied arguments.
		 */
		i = 0;
		ast_fillout(a->u.apply.right, new_ar, &i);
		/*printf(":: filled out %d\n", i);*/

#ifdef DEBUG
		if (trace_calls) {
			printf("---> call:");
			value_print(l);
			printf("(");
			activation_dump(current_ar, 1);
			printf(")\n");
		}
#endif

		if (l->type == VALUE_BUILTIN) {
			struct value *g;
			g = l->v.f(new_ar);
			assert(current_ar == old_ar);
			value_set_from_value(v, g);
			value_release(g);
		} else if (l->type == VALUE_CLOSURE) {
			current_ar = new_ar;
			ast_eval(l->v.k->ast, v);
			assert(current_ar == new_ar);
			assert(current_ar->caller == old_ar);
			current_ar = current_ar->caller;
		}
		
		/*
		 * Indicate that we're not longer using ar and that
		 * the refcounter can deallocate it if it wants.
		 */
		/*activation_release(current_ar);*/

		/*
		 * Restore the environment of the symbols to what it was
		 * before the closure was evaluated.
		 */
		/*
		printf("Returning to caller:\n");
		activation_dump(current_ar->prev, 1);
		printf("\n");
		*/
		
		
		/*
		current_ar = old_ar;
		*/

#ifdef DEBUG
		if (trace_calls) {
			printf("<--- call done, retval=");
			value_print(*v);
			printf("\n");
		}
#endif

		value_release(l);
		break;
	case AST_ARG:
		assert("this should never happen" == NULL);
		/*value_set_list(v);
		ast_flatten(a, v);*/
		break;
	case AST_STATEMENT:
		ast_eval(a->u.statement.left, &l);
		value_release(l);
		ast_eval(a->u.statement.right, v);
		break;
	case AST_ASSIGNMENT:
		ast_eval(a->u.assignment.right, v);
		assert(a->u.assignment.left != NULL);
		if (a->u.assignment.left->type == AST_LOCAL) {
			activation_set_value(current_ar,
			    a->u.assignment.left->u.local.index,
			    a->u.assignment.left->u.local.upcount,
			    *v);
#ifdef DEBUG
			if (trace_assignments) {
				symbol_dump(a->u.assignment.left->u.local.sym, 1);
				printf(":=");
				value_print(*v);
				printf("\n");
			}
#endif
		} else {
			value_set_error(v, "bad lvalue");
		}
		break;
	case AST_CONDITIONAL:
		ast_eval(a->u.conditional.test, &l);
		if (l == NULL || l->type != VALUE_BOOLEAN) {
			value_set_error(v, "type mismatch");
		} else {
			if (l->v.b) {
				ast_eval(a->u.conditional.yes, v);
			} else if (a->u.conditional.no != NULL) {
				ast_eval(a->u.conditional.no, v);
			} else {
				value_set_error(v, "missing else");
			}
		}
		value_release(l);
		break;
	case AST_WHILE_LOOP:
		for (;;) {
			ast_eval(a->u.while_loop.test, &l);
			if (l == NULL || l->type != VALUE_BOOLEAN) {
				value_release(l);
				value_set_error(v, "type mismatch");
				break;
			} else {
				/*
				printf("WHILE: test=");
				value_print(l);
				printf("\n");
				*/
				if (!l->v.b) {
					/*
					 * `while' condition evaluated to false.
					 */
					value_release(l);
					break;
				}
				ast_eval(a->u.while_loop.body, v);
				/*
				printf("WHILE: body=");
				value_print(*v);
				printf("\n");
				*/
			}
		}
		break;
	}

#ifdef DEBUG
	if (trace_ast) {
		printf("<<< LEAVING %s[0x%08lx] w/value=", ast_name(a), (unsigned long)a);
		value_print(*v);
		printf("\n");
	}
#endif
}
