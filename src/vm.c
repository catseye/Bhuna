#include <stdio.h>
#include <stdlib.h>

#include "vm.h"
#include "builtin.h"
#include "activation.h"
#include "value.h"
#include "closure.h"
#include "ast.h"

static struct value *value_stack[65536];
static struct value **vsp = value_stack;

static vm_label_t call_stack[65536];
static vm_label_t *csp = call_stack;

/*
 * Macros to push values onto and pop values off of the stack.
 *
 * Note  that pushing a value increases its refcount, while
 * popping a value does not.  Why the assymetry?  The assumption
 * is that when you pop a value, it will be placed into a local
 * variable; thus you still hold the same number of references to it.
 *
 * Also, if pushing a value increases its refcount, why is
 * 'value_grab(v)' not part of the PUSH_VALUE macro?  Because
 * a push is almost always followed by a value_release(), so we
 * optimize by avoiding the combination.
 */
#define PUSH_VALUE(v)	{ *(vsp++) = v; }
#define POP_VALUE(v)	{ v = *(--vsp); }

#define PUSH_PC(pc)	(*(csp++) = pc)
#define POP_PC()	(*(--csp))

extern struct activation *current_ar;

extern int trace_vm;
static int i;

static void
dump_stack()
{
	struct value **si;
	int i = 0;

	for (si = value_stack; si < vsp; si++) {
		printf("sk@%02d: ", i++);
		value_print(*si);
		printf("\n");
	}
}

void
vm_run(vm_label_t program)
{
	vm_label_t pc = program;
	vm_label_t label;
	struct value *l = NULL, *r = NULL, *v = NULL;
	struct activation *ar;
	int varity;

#ifdef DEBUG
	if (trace_vm) {
		printf("___ virtual machine started ___\n");
	}
#endif

	while (*pc != INSTR_HALT) {
#ifdef DEBUG
		if (trace_vm) {
			printf("#%d:\n", pc - program);
			dump_stack();
		}
#endif
		switch (*pc) {

#ifdef INLINE_BUILTINS
		case INDEX_BUILTIN_NOT:
			POP_VALUE(l);
			if (l->type == VALUE_BOOLEAN) {
				v = value_new_boolean(!l->v.b);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			break;
		case INDEX_BUILTIN_AND:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
				v = value_new_boolean(l->v.b && r->v.b);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_OR:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
				v = value_new_boolean(l->v.b || r->v.b);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;

		case INDEX_BUILTIN_EQU:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_boolean(l->v.i == r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_NEQ:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_boolean(l->v.i != r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_GT:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_boolean(l->v.i > r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_LT:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_boolean(l->v.i < r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_GTE:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_boolean(l->v.i >= r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_LTE:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_boolean(l->v.i <= r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;

		case INDEX_BUILTIN_ADD:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_integer(l->v.i + r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_MUL:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_integer(l->v.i * r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_SUB:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_integer(l->v.i - r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_DIV:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				if (r->v.i == 0)
					v = value_new_error("division by zero");
				else
					v = value_new_integer(l->v.i / r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;
		case INDEX_BUILTIN_MOD:
			POP_VALUE(r);
			POP_VALUE(l);
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				if (r->v.i == 0)
					v = value_new_error("modulo by zero");
				else
					v = value_new_integer(l->v.i % r->v.i);
			} else {
				v = value_new_error("type mismatch");			}
			PUSH_VALUE(v);
			value_release(l);
			value_release(r);
			break;

#endif /* INLINE_BUILTINS */

		case INSTR_PUSH_VALUE:
			l = *(struct value **)(pc + 1);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_PUSH_VALUE:\n");
				value_print(l);
				printf("\n");
			}
#endif
			value_grab(l);
			PUSH_VALUE(l);
			pc += sizeof(struct value *);
			break;

		case INSTR_PUSH_LOCAL:
			l = activation_get_value(current_ar, *(pc + 1), *(pc + 2));
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_PUSH_LOCAL:\n");
				value_print(l);
				printf("\n");
			}
#endif
			value_grab(l);
			PUSH_VALUE(l);
			pc += sizeof(unsigned char) * 2;
			break;

		case INSTR_POP_LOCAL:
			POP_VALUE(l);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_POP_LOCAL:\n");
				value_print(l);
				printf("\n");
			}
#endif
			activation_set_value(current_ar, *(pc + 1), *(pc + 2), l);
			value_release(l);
			pc += sizeof(unsigned char) * 2;
			break;

		case INSTR_JMP:
			label = *(vm_label_t *)(pc + 1);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_JMP -> #%d:\n", label - program);
			}
#endif
			pc = label - 1;
			break;

		case INSTR_JZ:
			POP_VALUE(l);
			label = *(vm_label_t *)(pc + 1);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_JZ -> #%d:\n", label - program);
			}
#endif
			if (!l->v.b) {
				pc = label - 1;
			} else {
				pc += sizeof(vm_label_t);
			}
			value_release(l);
			break;

		case INSTR_CALL:
			POP_VALUE(l);
			label = l->v.k->ast->label;
			if (l->v.k->ast->u.routine.cc > 0) {
				/*
				 * Create a new activation record
				 * on the heap for this call.
				 */
				ar = activation_new_on_heap(
				    l->v.k->ast->u.routine.arity +
				    l->v.k->ast->u.routine.locals,
				    current_ar, l->v.k->ar);
			} else {
				/*
				 * Optimize by placing it on a stack.
				 */
				ar = activation_new_on_stack(
				    l->v.k->ast->u.routine.arity +
				    l->v.k->ast->u.routine.locals,
				    current_ar, l->v.k->ar);				
			}
			/*
			 * Fill out the activation record.
			 */
			for (i = l->v.k->ast->u.routine.arity - 1; i >= 0; i--) {
				POP_VALUE(r)
				activation_initialize_value(ar, i, r);
				value_release(r);
			}

			current_ar = ar;
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_CALL -> #%d:\n", label - program);
			}
#endif
			PUSH_PC(pc + 1); /* + sizeof(vm_label_t)); */
			pc = label - 1;
			value_release(l);
			break;

		case INSTR_GOTO:
			POP_VALUE(l);
			label = l->v.k->ast->label;
			/*
			 * DON'T create a new activation record for this leap
			 * UNLESS it's to a different closure than the currently
			 * executing one.
			 */
			/*
			ar = activation_new(l->v.k->arity, current_ar, l->v.k->ar);
			*/
			/*
			 * blah bla blah
			 */
			for (i = l->v.k->ast->u.routine.arity - 1; i >= 0; i--) {
				POP_VALUE(r);
				activation_set_value(current_ar, i, 0, r);
				value_release(r);
			}
			/*
			current_ar = ar;
			*/
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_GOTO -> #%d:\n", label - program);
			}
#endif
			/*PUSH_PC(pc + 1);*/ /* + sizeof(vm_label_t)); */
			pc = label - 1;
			value_release(l);
			break;

		case INSTR_RET:
			pc = POP_PC() - 1;
			if (activation_is_on_stack(current_ar)) {
				ar = current_ar->caller;
				activation_free_from_stack(current_ar);
				current_ar = ar;
			} else {
				current_ar = current_ar->caller;
			}
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_RET -> #%d:\n", pc - program);
			}
#endif
			break;

		case INSTR_SET_ACTIVATION:
			POP_VALUE(l);
			l->v.k->ar = current_ar;
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_SET_ACTIVATION #%d\n", l->v.k->ast->label - program);
			}
#endif
			PUSH_VALUE(l);
			break;

		case INSTR_DEEP_COPY:
			POP_VALUE(l);
			r = value_dup(l);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_DEEP_COPY\n");
			}
#endif
			value_release(l);
			PUSH_VALUE(r);
			break;

		default:
			/*
			 * We assume it was a non-inline builtin.
			 */
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_BUILTIN(%s):\n", builtins[*pc].name);
			}
#endif
			varity = builtins[*pc].arity;
			if (varity == -1) {
				POP_VALUE(l);
				varity = l->v.i;
				value_release(l);
			}
			ar = activation_new_on_stack(varity, current_ar, NULL);
			for (i = varity - 1; i >= 0; i--) {
				POP_VALUE(l);
				activation_initialize_value(ar, i, l);
				value_release(l);
			}

			v = NULL;
			builtins[*pc].fn(ar, &v);
			activation_free_from_stack(ar);
#ifdef DEBUG
			if (trace_vm) {
				printf("result was:\n");
				value_print(v);
				printf("\n");
			}
#endif
			if (!builtins[*pc].is_pure) {
				value_release(v);
			} else {
				PUSH_VALUE(v);
			}
		}
		pc++;
	}

#ifdef DEBUG
	if (trace_vm) {
		printf("___ virtual machine finished ___\n");
	}
#endif
}


/*
 * Release all the values embedded in the VM program.
 */
void
vm_release(vm_label_t program)
{
	vm_label_t pc = program;

#ifdef DEBUG
	if (trace_vm) {
		printf("___ release virtual machine program ___\n");
	}
#endif
	while (*pc != INSTR_HALT) {
		switch (*pc) {
		case INSTR_PUSH_VALUE:
			value_release(*(struct value **)(pc + 1));
			pc += sizeof(struct value *);
			break;

		case INSTR_PUSH_LOCAL:
			pc += sizeof(unsigned char) * 2;
			break;

		case INSTR_POP_LOCAL:
			pc += sizeof(unsigned char) * 2;
			break;

		case INSTR_JMP:
			pc += sizeof(vm_label_t);
			break;

		case INSTR_JZ:
			pc += sizeof(vm_label_t);
			break;

		case INSTR_CALL:
			break;

		case INSTR_GOTO:
			break;

		case INSTR_RET:
			break;

		case INSTR_SET_ACTIVATION:
			/* pc += sizeof(vm_label_t); */
			break;

		case INSTR_DEEP_COPY:
			break;

		default:
			/*
			 * We assume it was a builtin.
			 */
			break;
		}
		pc++;
	}
#ifdef DEBUG
	if (trace_vm) {
		printf("___ virtual machine program released ___\n");
	}
#endif
}
