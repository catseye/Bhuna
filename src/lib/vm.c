#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "vm.h"
#include "builtin.h"
#include "activation.h"
#include "value.h"
#include "closure.h"
#include "ast.h"
#include "gc.h"
#include "process.h"

/*
 * Macros to push values onto and pop values off of the stack.
 */
#define PUSH_VALUE(v)	(*(vm->vstack_ptr++) = v)
#define POP_VALUE(v)	(v = *(--vm->vstack_ptr))

#define PUSH_PC(pc)	(*(vm->cstack_ptr++) = pc)
#define POP_PC()	(*(--vm->cstack_ptr))

#ifdef DEBUG
extern int trace_vm;
extern int trace_gc;
#endif

extern int gc_target, gc_trigger, a_count, v_count;
static int i;

struct vm *
vm_new(vm_label_t program, size_t prog_size)
{
	struct vm *vm;

	vm = bhuna_malloc(sizeof(struct vm));

	vm->prog_size = prog_size;
	vm->program = program;
	vm->pc = vm->program;

	vm->vstack_size = 65536;
	vm->vstack = bhuna_malloc(vm->vstack_size * sizeof(struct value *));
	vm->vstack_ptr = vm->vstack;

	vm->cstack_size = 8192;
	vm->cstack = bhuna_malloc(vm->cstack_size * sizeof(vm_label_t));
	vm->cstack_ptr = vm->cstack;

	vm->astack_size = 65536;
	vm->astack = bhuna_malloc(vm->astack_size * sizeof(unsigned char));
	vm->astack_ptr = vm->astack;

	return(vm);
}

void
vm_free(struct vm *vm)
{
	if (vm == NULL) return;
	bhuna_free(vm->vstack);
	bhuna_free(vm->cstack);
	bhuna_free(vm->astack);
	bhuna_free(vm);
}

#ifdef DEBUG
static void
dump_stack(struct vm *vm)
{
	struct value **si;
	int i = 0;

	for (si = vm->vstack; si < vm->vstack_ptr; si++) {
		printf("sk@%02d: ", i++);
		value_print(*si);
		printf("\n");
	}
}
#endif

void
vm_set_pc(struct vm *vm, vm_label_t pc)
{
	vm->pc = pc;
}

int
vm_run(struct vm *vm, int xmax)
{
	vm_label_t label;
	struct value *l = NULL, *r = NULL, *v = NULL;
	struct activation *ar;
	struct builtin *ext_bi;
	int varity;
	int xcount = 0;
	/*int upcount, index; */

#ifdef DEBUG
	if (trace_vm) {
		printf("___ virtual machine started ___\n");
	}
#endif

	while (*vm->pc != INSTR_HALT) {
#ifdef DEBUG
		if (trace_vm) {
			printf("#%d:\n", vm->pc - vm->program);
			dump_stack(vm);
		}
#endif
		if (((++xcount) & 0xff) == 0) {
			if (xcount >= xmax)
				return(VM_TIME_EXPIRED);

			if (a_count + v_count > gc_target) {
#ifdef DEBUG
				if (trace_gc > 0) {
					printf("[ARC] GARBAGE COLLECTION STARTED on %d activation records + %d values\n",
						a_count, v_count);
					/*activation_dump(current_ar, 0);
					printf("\n");*/
				}
#endif
				/*gc(vm);*/
#ifdef DEBUG
				if (trace_gc > 0) {
					printf("[ARC] GARBAGE COLLECTION FINISHED, now %d activation records + %d values\n",
						a_count, v_count);
					/*activation_dump(current_ar, 0);
					printf("\n");*/
				}
#endif
				/*
				 * Slide the target to account for the fact that there
				 * are now 'a_count' activation records in existence.
				 * Only GC when there are gc_trigger *more* ar's.
				 */
				gc_target = a_count + v_count + gc_trigger;
			}
		}

		switch (*vm->pc) {

#ifdef INLINE_BUILTINS
		case INDEX_BUILTIN_NOT:
			POP_VALUE(l);
			if (l->type == VALUE_BOOLEAN) {
				v = value_new_boolean(!l->v.b);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
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
			break;
		case INDEX_BUILTIN_SUB:
			POP_VALUE(r);
			POP_VALUE(l);
			//subs++;
			if (l->type == VALUE_INTEGER && r->type == VALUE_INTEGER) {
				v = value_new_integer(l->v.i - r->v.i);
			} else {
				v = value_new_error("type mismatch");
			}
			PUSH_VALUE(v);
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
			break;

#endif /* INLINE_BUILTINS */

		case INSTR_PUSH_VALUE:
			l = *(struct value **)(vm->pc + 1);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_PUSH_VALUE:\n");
				value_print(l);
				printf("\n");
			}
#endif
			PUSH_VALUE(l);
			vm->pc += sizeof(struct value *);
			break;

		case INSTR_PUSH_LOCAL:
			l = activation_get_value(vm->current_ar,
			    *(vm->pc + 1), *(vm->pc + 2));

#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_PUSH_LOCAL:\n");
				value_print(l);
				printf("\n");
			}
#endif
			PUSH_VALUE(l);
			vm->pc += sizeof(unsigned char) * 2;
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
			activation_set_value(vm->current_ar,
			    *(vm->pc + 1), *(vm->pc + 2), l);
			vm->pc += sizeof(unsigned char) * 2;
			break;

		case INSTR_JMP:
			label = *(vm_label_t *)(vm->pc + 1);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_JMP -> #%d:\n", label - vm->program);
			}
#endif
			vm->pc = label - 1;
			break;

		case INSTR_JZ:
			POP_VALUE(l);
			label = *(vm_label_t *)(vm->pc + 1);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_JZ -> #%d:\n", label - vm->program);
			}
#endif
			if (!l->v.b) {
				vm->pc = label - 1;
			} else {
				vm->pc += sizeof(vm_label_t);
			}
			break;

		case INSTR_CALL:
			POP_VALUE(l);
			label = l->v.k->ast->label;
			if (l->v.k->cc > 0) {
				/*
				 * Create a new activation record
				 * on the heap for this call.
				 */
				ar = activation_new_on_heap(
				    l->v.k->arity +
				    l->v.k->locals,
				    vm->current_ar, l->v.k->ar);
			} else {
				/*
				 * Optimize by placing it on a stack.
				 */
				ar = activation_new_on_stack(
				    l->v.k->arity +
				    l->v.k->locals,
				    vm->current_ar, l->v.k->ar, vm);
			}
			/*
			 * Fill out the activation record.
			 */
			for (i = l->v.k->arity - 1; i >= 0; i--) {
				POP_VALUE(r);
				activation_initialize_value(ar, i, r);
			}

			vm->current_ar = ar;
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_CALL -> #%d:\n", label - vm->program);
			}
#endif
			/*
			printf("%% process %d pushing pc = %d\n",
			    current_process->number, vm->pc - vm->program);
			*/
			PUSH_PC(vm->pc + 1); /* + sizeof(vm_label_t)); */
			vm->pc = label - 1;
			break;

		case INSTR_GOTO:
			POP_VALUE(l);
			label = l->v.k->ast->label;

			/*
			 * DON'T create a new activation record for this leap
			 * UNLESS the current activation record isn't large enough.
			 */
			/*
			printf("GOTOing a closure w/arity %d locals %d\n",
				l->v.k->arity, l->v.k->locals);
			printf("current ar size %d\n", current_ar->size);
			*/

			if (vm->current_ar->size < l->v.k->arity + l->v.k->locals) {
				/*
				 * REMOVE the current activation record, if on the stack.
				 */
				if (vm->current_ar->admin & AR_ADMIN_ON_STACK) {
					ar = vm->current_ar->caller;
					activation_free_from_stack(vm->current_ar, vm);
					vm->current_ar = ar;
				} else {
					vm->current_ar = vm->current_ar->caller;
				}

				/*
				 * Create a NEW activation record... wherever.
				 */
				if (l->v.k->cc > 0) {
					/*
					 * Create a new activation record
					 * on the heap for this call.
					 */
					vm->current_ar = activation_new_on_heap(
					    l->v.k->arity +
					    l->v.k->locals,
					    vm->current_ar, l->v.k->ar);
				} else {
					/*
					 * Optimize by placing it on a stack.
					 */
					vm->current_ar = activation_new_on_stack(
					    l->v.k->arity +
					    l->v.k->locals,
					    vm->current_ar, l->v.k->ar, vm);
				}
			}

			/*
			printf("NOW GOTOing a closure w/arity %d locals %d\n",
				l->v.k->arity, l->v.k->locals);
			printf("NOW current ar size %d\n", current_ar->size);
			*/

			/*
			 * Fill out the current activation record.
			 */
			for (i = l->v.k->arity - 1; i >= 0; i--) {
				POP_VALUE(r);
				activation_set_value(vm->current_ar, i, 0, r);
			}

#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_GOTO -> #%d:\n", label - vm->program);
			}
#endif
			/*PUSH_PC(pc + 1);*/ /* + sizeof(vm_label_t)); */
			vm->pc = label - 1;
			break;

		case INSTR_RET:
			vm->pc = POP_PC() - 1;
			/*
			printf("%% process %d popped pc = %d\n",
			    current_process->number, vm->pc - vm->program);
			*/
			if (vm->current_ar->admin & AR_ADMIN_ON_STACK) {
				ar = vm->current_ar->caller;
				activation_free_from_stack(vm->current_ar, vm);
				vm->current_ar = ar;
			} else {
				vm->current_ar = vm->current_ar->caller;
			}
			if (vm->current_ar == NULL)
				return(VM_RETURNED);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_RET -> #%d:\n", vm->pc - vm->program);
			}
#endif
			break;

		case INSTR_SET_ACTIVATION:
			POP_VALUE(l);
			l->v.k->ar = vm->current_ar;
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_SET_ACTIVATION #%d\n",
				    l->v.k->ast->label - vm->program);
			}
#endif
			PUSH_VALUE(l);
			break;

		case INSTR_COW_LOCAL:
			l = activation_get_value(vm->current_ar, *(vm->pc + 1), *(vm->pc + 2));

			if (l->refcount > 1) {
				/*
				printf("deep-copying ");
				value_print(l);
				printf("...\n");
				*/
				r = value_dup(l);
				activation_set_value(vm->current_ar, *(vm->pc + 1), *(vm->pc + 2), r);
			}

#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_COW_LOCAL:\n");
				value_print(l);
				printf("\n");
			}
#endif

			vm->pc += sizeof(unsigned char) * 2;
			break;

		case INSTR_EXTERNAL:
			ext_bi = *(struct builtin **)(vm->pc + 1);
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_EXTERNAL(%s):\n", ext_bi->name);
			}
#endif
			varity = ext_bi->arity;
			if (varity == -1) {
				POP_VALUE(l);
				varity = l->v.i;
			}
			ar = activation_new_on_stack(varity, vm->current_ar, NULL, vm);
			for (i = varity - 1; i >= 0; i--) {
				POP_VALUE(l);
				activation_initialize_value(ar, i, l);
			}

			v = NULL;
			ext_bi->fn(ar, &v);
			activation_free_from_stack(ar, vm);
#ifdef DEBUG
			if (trace_vm) {
				printf("result was:\n");
				value_print(v);
				printf("\n");
			}
#endif
			/* XXXXXXX */
			if (ext_bi->is_pure) {
				PUSH_VALUE(v);
			}

			vm->pc += sizeof(struct builtin *);
			break;
		default:
			/*
			 * We assume it was a non-inline builtin.
			 */
#ifdef DEBUG
			if (trace_vm) {
				printf("INSTR_BUILTIN(%s):\n", builtins[*vm->pc].name);
			}
#endif
			varity = builtins[*vm->pc].arity;
			if (varity == -1) {
				POP_VALUE(l);
				varity = l->v.i;
			}
			ar = activation_new_on_stack(varity, vm->current_ar, NULL, vm);
			for (i = varity - 1; i >= 0; i--) {
				POP_VALUE(l);
				activation_initialize_value(ar, i, l);
			}

			v = NULL;
			builtins[*vm->pc].fn(ar, &v);
			activation_free_from_stack(ar, vm);
#ifdef DEBUG
			if (trace_vm) {
				printf("result was:\n");
				value_print(v);
				printf("\n");
			}
#endif
			/* XXXXXXX */
			if (builtins[*vm->pc].is_pure) {
				PUSH_VALUE(v);
			}
		}
		vm->pc++;
	}

#ifdef DEBUG
	if (trace_vm) {
		printf("___ virtual machine finished ___\n");
	}
	/*printf("subs = %d\n", subs);*/
#endif
	return(VM_TERMINATED);
}
