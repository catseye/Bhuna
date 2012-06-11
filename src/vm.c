#include <stdio.h>
#include <stdlib.h>

#include "vm.h"
#include "builtin.h"
#include "activation.h"
#include "value.h"
#include "closure.h"

static struct value *value_stack[65536];
static struct value **vsp = value_stack;

static vm_label_t call_stack[65536];
static vm_label_t *csp = call_stack;

#define PUSH_VALUE(v)	(*(vsp++) = v)
#define POP_VALUE()	(*(--vsp))
#define GET_VALUE(i)	(*(vsp - (i + 1)))

#define PUSH_PC(pc)	(*(csp++) = pc)
#define POP_PC()	(*(--csp))

extern struct activation *current_ar;

extern int trace_vm;
static int i;

void
vm_run(vm_label_t program)
{
	vm_label_t pc = program;
	vm_label_t label;
	struct value *l, *r;
	struct activation *ar;

#ifdef DEBUG
	if (trace_vm) {
		printf("___ virtual machine started ___\n");
	}
#endif

	while (*pc != INSTR_HALT) {
#ifdef DEBUG
		if (trace_vm) {
			printf("#%d:\n", pc - program);
		}
#endif
		switch (*pc) {

#ifdef INLINE_BUILTINS
/*
		case INDEX_BUILTIN_PRINT:
			l = POP_VALUE();
			ar = activation_new(1, current_ar, NULL);
			activation_set_value(ar, 0, 0, l);
			builtin_print(ar, &l);
			activation_free(ar);
			break;
*/

		case INDEX_BUILTIN_NOT:
			l = POP_VALUE();
			if (l->type == VALUE_BOOLEAN) {
				value_set_boolean(&l, !l->v.b);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_AND:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
				value_set_boolean(&l, l->v.b && r->v.b);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_OR:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_BOOLEAN && r->type == VALUE_BOOLEAN) {
				value_set_boolean(&l, l->v.b || r->v.b);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;

		case INDEX_BUILTIN_EQU:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(&l, l->v.i == r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_NEQ:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(&l, l->v.i != r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_GT:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(&l, l->v.i > r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_LT:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(&l, l->v.i < r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_GTE:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(&l, l->v.i >= r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_LTE:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_boolean(&l, l->v.i <= r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;

		case INDEX_BUILTIN_ADD:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_integer(&l, l->v.i + r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_MUL:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_integer(&l, l->v.i * r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_SUB:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				value_set_integer(&l, l->v.i - r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_DIV:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				if (r->v.i == 0)
					value_set_error(&l, "division by zero");
				else
					value_set_integer(&l, l->v.i / r->v.i);
			} else {
				value_set_error(&l, "type mismatch");
			}
			PUSH_VALUE(l);
			break;
		case INDEX_BUILTIN_MOD:
			r = POP_VALUE();
			l = POP_VALUE();
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				if (r->v.i == 0)
					value_set_error(&l, "modulo by zero");
				else
					value_set_integer(&l, l->v.i % r->v.i);
			} else {
				value_set_error(&l, "type mismatch");			}
			PUSH_VALUE(l);
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
			l = POP_VALUE();
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_POP_LOCAL:\n");
				value_print(l);
				printf("\n");
			}
#endif
			activation_set_value(current_ar, *(pc + 1), *(pc + 2), l);
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
			l = POP_VALUE();
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
			break;

		case INSTR_CALL:
			l = POP_VALUE();
			label = l->v.k->label;
			/*
			 * Create a new activation record for this call.
			 */
			ar = activation_new(l->v.k->arity, current_ar, l->v.k->ar);
			/*
			 * blah bla blah
			 */
			for (i = l->v.k->arity - 1; i >= 0; i--) {
				activation_set_value(ar, i, 0, POP_VALUE());
			}
			current_ar = ar;
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_CALL -> #%d:\n", label - program);
			}
#endif
			PUSH_PC(pc + 1); /* + sizeof(vm_label_t)); */
			pc = label - 1;
			break;

		case INSTR_GOTO:
			l = POP_VALUE();
			label = l->v.k->label;
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
			for (i = l->v.k->arity - 1; i >= 0; i--) {
				activation_set_value(current_ar, i, 0, POP_VALUE());
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
			break;

		case INSTR_RET:
			pc = POP_PC() - 1;
			current_ar = current_ar->caller;
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_RET -> #%d:\n", pc - program);
			}
#endif
			break;

		case INSTR_SET_ACTIVATION:
			GET_VALUE(0)->v.k->ar = current_ar;
			label = *(vm_label_t *)(pc + 1);
			GET_VALUE(0)->v.k->label = label;
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_SET_ACTIVATION #%d\n", label - program);
			}
#endif
			pc += sizeof(vm_label_t);
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
			ar = activation_new(builtins[*pc].arity, current_ar, NULL);
			if (builtins[*pc].arity == 1) {
				l = POP_VALUE();
				activation_set_value(ar, 0, 0, l);
			}
			if (builtins[*pc].arity == 2) {
				r = POP_VALUE();
				l = POP_VALUE();
				activation_set_value(ar, 0, 0, l);
				activation_set_value(ar, 1, 0, r);
			}
			builtins[*pc].fn(ar, &l);
#ifdef DEBUG
			if (trace_vm) {
				printf("result was:\n");
				value_print(l);
				printf("\n");
			}
#endif
			PUSH_VALUE(l);
			activation_free(ar);
		}
		pc++;
	}

#ifdef DEBUG
	if (trace_vm) {
		printf("___ virtual machine finished ___\n");
	}
#endif
}
