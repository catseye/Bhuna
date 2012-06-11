#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "value.h"
#include "symbol.h"
#include "list.h"
#include "closure.h"
#include "activation.h"

#ifdef DEBUG
extern int trace_assignments;
extern int trace_calls;
extern int trace_ast;
extern int trace_closures;
#endif

extern struct activation *current_ar;

/*** OPERATIONS ***/

/*
 * Flatten arguments into a given list value.
 */
static void
ast_flatten(struct ast *a, struct value **lvp)
{
	struct value *v = NULL;

	if (a == NULL)
		return;

	switch (a->type) {
	case AST_ARG:
		/*
		 * We go right-to-left here so that our list will
		 * be created the right way 'round.
		 */
		ast_flatten(a->u.arg.right, lvp);
		ast_flatten(a->u.arg.left, lvp);
		break;
	default:
		ast_eval(a, &v);
		value_list_append(lvp, v);
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
	struct symbol *sym;
	struct value *l = NULL, *r = NULL, *lv = NULL;

	if (a == NULL)
		return;

#ifdef DEBUG
	if (trace_ast) {
		printf(">>> ENTERING %s[0x%08lx]\n", ast_name(a), (unsigned long)a);
	}
#endif

	switch (a->type) {
	case AST_SYM:
		lv = activation_get_value(current_ar, a->u.sym.sym);
		if (lv == NULL) {
			printf("*** undefined symbol: ");
			symbol_dump(a->u.sym.sym, 0);
			value_set_error(v, "undefined value");
		} else {
			value_set_from_value(v, lv);
		}
		break;
	case AST_VALUE:
		value_set_from_value(v, a->u.value.value);
		if (*v != NULL && (*v)->type == VALUE_CLOSURE) {
			/* XXX Freshen the closure. */
			activation_release((*v)->v.k->ar);
			activation_grab(current_ar);
			(*v)->v.k->ar = current_ar;
		}
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
		ast_eval(a->u.apply.left, &l);
		ast_eval(a->u.apply.right, &r);
		/*
		 * Make the RHS a list (in case it's not) for consistency.
		 */
		if (r->type == VALUE_LIST) {
			value_set_from_value(v, r);
		} else {
			value_set_list(v);
			value_list_append(v, r);
		}
		value_release(r);

#ifdef DEBUG
		if (trace_calls) {
			printf("---> call:");
			value_print(l);
			printf("(");
			value_print(*v);
			printf(")\n");
		}
#endif

		/*** BEGIN value_call(l, v); ***/

		if (l->type == VALUE_BUILTIN) {
			l->v.f(v);
		} else if (l->type == VALUE_CLOSURE) {
			struct symbol_table *lstab;
			struct activation *old_ar;

			assert(l->v.k->ast->type == AST_SCOPE);
			lstab = l->v.k->ast->u.scope.local;

			/*
			 * Get a new activation record.
			 */
			old_ar = current_ar;

			/*
			 * Create a new activation record whose lexical
			 * link is the environment of the closure.
			 */
			current_ar = activation_new(lstab, l->v.k->ar);

			/*
			 * Populate the closure's trapped symbol table with
			 * values taken from the current activation.
			 */
			/*
			symbol_table_push_frame(l->v.k->ast->u.scope.trap);
			*/

			/*
			 * Put the arguments into the symbol 'Arg' in the new frame.
			 */
			sym = symbol_lookup(lstab, "Arg", 0);
			assert(sym != NULL);
			activation_set_value(current_ar, sym, *v);

			/*
			 * Evaluate the closure.
			 */
			ast_eval(l->v.k->ast, v);

			/*
			 * Indicate that we're not longer using ar and that
			 * the refcounter can deallocate it if it wants.
			 */
			activation_release(current_ar);

			/*
			 * Restore the environment of the symbols to what it was
			 * before the closure was evaluated.
			 */
			current_ar = old_ar;
		} else {
			value_set_error(v, "not executable");
		}

		/*** END value_call(l, v); ***/

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
		value_set_list(v);
		ast_flatten(a, v);
		break;
	case AST_STATEMENT:
		ast_eval(a->u.statement.left, &l);
		value_release(l);
		ast_eval(a->u.statement.right, v);
		break;
	case AST_ASSIGNMENT:
		ast_eval(a->u.assignment.right, v);
		if (a->u.assignment.left != NULL && a->u.assignment.left->type == AST_SYM) {
			sym = a->u.assignment.left->u.sym.sym;
			activation_set_value(current_ar, sym, *v);
#ifdef DEBUG
			if (trace_assignments) {
				symbol_dump(sym, 1);
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
				if (!l->v.b) {
					/*
					 * `while' condition evaluated to false.
					 */
					value_release(l);
					break;
				}
				ast_eval(a->u.while_loop.body, v);
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
