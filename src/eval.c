#ifdef RECURSIVE_AST_EVALUATOR

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "value.h"
#include "list.h"
#include "closure.h"
#include "activation.h"
#include "builtin.h"

#ifdef DEBUG
#include "symbol.h"

extern int trace_assignments;
extern int trace_calls;
extern int trace_ast;
extern int trace_closures;
#endif

extern struct activation *current_ar;

static struct value *initializer;

void
ast_eval_init(void)
{
	initializer = value_new_integer(76);
}

/*** OPERATIONS ***/

/*
 * Fill out an activation record with arguments.
 * Unlike ast_eval(), this is iterative.
 */
static void
ast_fillout(struct ast *a, struct activation *ar)
{
	struct value *v;
	int idxp;

	for (idxp = 0; a != NULL; a = a->u.arg.right) {
		assert(a->type == AST_ARG);
		v = NULL;
		ast_eval(a->u.arg.left, &v);
		activation_set_value(ar, idxp++, 0, v);
		value_release(v);
	}

	/*
	while (idxp < ar->size) {
		// printf("unfilled local or temporary @ %d\n", idxp);
		activation_set_value(ar, idxp++, 0, initializer);
	}
	*/
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
	struct value *l = NULL, *r = NULL, *lv = NULL;
	/* struct value **q = NULL; */
	struct activation *new_ar = NULL;
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
			(*v)->v.k->ar = current_ar;
		}
		/*value_release(a->u.value.value);*/
		break;
	case AST_BUILTIN:
		ast_eval(a->u.builtin.left, &l);
		ast_eval(a->u.builtin.right, &r);

/*
		switch (a->u.builtin.bi->fn) {
		case builtin_not:
		case builtin_and:
		case builtin_or:

		case builtin_equ:
		case builtin_neq:
		case builtin_gt:
		case builtin_lt:
		case builtin_gte:
		case builtin_lte:

		case builtin_add:
		case builtin_mul:
		case builtin_sub:
		case builtin_div:
		case builtin_mod:
		default:
			break;
		}
*/

#ifdef INLINE_BUILTINS
		switch (a->u.builtin.bi->index) {
		case INDEX_BUILTIN_NOT:
			if (l->type == VALUE_BOOLEAN) {
				value_set_boolean(v, !l->v.b);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_AND:
			if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
				value_set_boolean(v, l->v.b && r->v.b);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_OR:
			if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
				value_set_boolean(v, l->v.b || r->v.b);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;

		case INDEX_BUILTIN_EQU:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(v, l->v.i == r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_NEQ:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(v, l->v.i != r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_GT:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(v, l->v.i > r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_LT:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(v, l->v.i < r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_GTE:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(v, l->v.i >= r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_LTE:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(v, l->v.i <= r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;

		case INDEX_BUILTIN_ADD:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_integer(v, l->v.i + r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_MUL:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_integer(v, l->v.i * r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_SUB:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_integer(v, l->v.i - r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_DIV:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				if (r->v.i == 0)
					value_set_error(v, "division by zero");
				else
					value_set_integer(v, l->v.i / r->v.i);
			} else {
				value_set_error(v, "type mismatch");
			}
			break;
		case INDEX_BUILTIN_MOD:
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				if (r->v.i == 0)
					value_set_error(v, "modulo by zero");
				else
					value_set_integer(v, l->v.i % r->v.i);
			} else {
				value_set_error(v, "type mismatch");			}
			break;

		default:
#endif
			/*printf("doing builtin %s\n", a->u.builtin.bi->name);*/
			new_ar = activation_new(a->u.builtin.bi->arity, current_ar, NULL);
			if (a->u.builtin.bi->arity > 0) {
				activation_set_value(new_ar, 0, 0, l);
				if (a->u.builtin.bi->arity > 1) {
					activation_set_value(new_ar, 1, 0, r);
				}
			}
			a->u.builtin.bi->fn(new_ar, v);
			activation_free(new_ar);
#ifdef INLINE_BUILTINS
			break;
		}
#endif
		value_release(l);
		value_release(r);
		break;
	case AST_APPLY:
		/*
		 * Get the function we're being asked to apply.
		 */
		ast_eval(a->u.apply.left, &l);
		assert(l->type == VALUE_CLOSURE);

		/*
		 * Create a new activation record apropos to l.
		 * Include the closure's environment as the
		 * lexical link of the activation record.
		 *
		 * An optimization: if the closure we are executing here
		 * contains no closures of its own, this activation record
		 * will never be used by any other closure, so we don't
		 * have to keep it on the heap nor garbage collect it.
		 * We allocate it on the (C) stack instead.
		 */
		if (l->v.k->cc == 0) {
			/*printf("stack allocation!\n");*/
			new_ar = alloca(sizeof(struct activation) +
				    sizeof(struct value *) * l->v.k->arity);
			bzero(new_ar, sizeof(struct activation) +
				    sizeof(struct value *) * l->v.k->arity);
			new_ar->size = l->v.k->arity;
			new_ar->caller = current_ar;
			new_ar->enclosing = l->v.k->ar;
			new_ar->marked = 0;
		} else {
			new_ar = activation_new(l->v.k->arity,
			    current_ar, l->v.k->ar);
		}

		/*
		 * Now, fill out that activation record with the
		 * supplied arguments.
		 *
		 * Note that, because ast_fillout() is mutually recursive with
		 * ast_eval(), and thus another AST_APPLY could be
		 * encountered, which could cause a new activation record
		 * to be created, which in turn could trigger the garbage
		 * collector, we have deferred registering the new activation
		 * record with the garbage collector until after this has
		 * finished.
		 */
		ast_fillout(a->u.apply.right, new_ar);
		/*
		printf(":: filled out:\n");
		activation_dump(new_ar, 1);
		printf("\n");
		*/

#ifdef DEBUG
		if (trace_calls) {
			printf("---> call:");
			value_print(l);
			printf("(");
			activation_dump(current_ar, 1);
			printf(")\n");
		}
#endif
		/*
		 * On the other hand, if the closure we are executing here
		 * DOES contains closures of its own, we DO have to
		 * register it with the garbage collector.
		 */
		if (l->v.k->cc > 0)
			activation_register(new_ar);
		
		/*
		 * Evaluate it, under the new activation record.
		 */
		current_ar = new_ar;
		ast_eval(l->v.k->ast, v);
		assert(current_ar == new_ar);
		current_ar = current_ar->caller;

		if (l->v.k->cc == 0) {
			for (i = 0; i < l->v.k->arity; i++) {
				value_release(
				    ((struct value **)((char *)new_ar +
				    sizeof(struct activation)))[i]
				);
			}
		}

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
		assert(a->u.assignment.left->type == AST_LOCAL);
		
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
		break;
	case AST_CONDITIONAL:
		/*
		l = activation_get_value(current_ar,
		    a->u.conditional.index, 0);
		*/

		/*
		q = &((struct value **)((char *)current_ar +
		    sizeof(struct activation)))[a->u.conditional.index];
		*/
		ast_eval(a->u.conditional.test, &l);

		if (l == NULL || l->type != VALUE_BOOLEAN) {
			value_set_error(v, "type mismatch");
		} else {
			if (l->v.b) {
				ast_eval(a->u.conditional.yes, v);
			} else if (a->u.conditional.no != NULL) {
				ast_eval(a->u.conditional.no, v);
			} else {
				/*
				value_set_error(v, "missing else");
				*/
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

#endif /* RECURSIVE_AST_EVALUATOR */
