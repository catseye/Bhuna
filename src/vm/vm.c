#include "mem.h"
#include "vm.h"

struct vm *
vm_new(size_t prog_size, size_t stack_size)
{
	struct vm *vm;

	vm = bhuna_malloc(sizeof(struct vm));
	vm->pc = 0;
	vm->sp = 0;
	vm->prog =  bhuna_malloc(sizeof(struct op) * stack_size);
	vm->stack = bhuna_malloc(sizeof(struct value *) * stack_size);

	return(vm);
}

void
vm_free(struct vm *vm)
{
	bhuna_free(vm->prog);
	bhuna_free(vm->stack);
	bhuna_free(vm);
}

void
vm_reset(struct vm *vm)
{
	vm->pc = 0;
}

void
vm_gen(struct vm *vm, struct op op)
{
	vm->prog[vm->pc++] = op;
}

static void
vm_push(struct vm *vm, struct value *v)
{
	vm->stack[vm->sp++] = v;
}

static void
vm_pop(struct vm *vm, struct value *v)
{
	v = vm->stack[--vm->sp];
}

void
vm_run(struct vm *vm)
{
	int done = 0;
	while (!done) {
		switch (vm->prog[pc].opcode) {
		case OP_HALT:
			done = 1;
			break;
		case OP_PUSH_VALUE:
			break;
		case OP_PUSH_LOCAL:
			break;
		case OP_POP_LOCAL:
			break;
		case OP_APPLY:
			break;
		case OP_JZ:
			break;
		case OP_JMP:
			break;
		}
		vm->pc++;
	}
}




void		 op_local(struct vm *);
void		 op_value(struct vm *);

void
op_apply(struct vm *vm)
{
	struct closure *k;
	struct activation *ar;
	struct value *v;

	vm_pop(k);
	vm_pop(ar);
	v = f(ar);
	vm_push(v);
}

void
op_assign(struct vm *vm)
{
	vm_pop(v);
	vm_pop(local);
	local_poke(local, v);
}

void
op_jz(struct vm *vm)
{
}

void
op_jmp(struct vm *vm)
{
	vm_pop(k);
	vm->pc = vm->prog[k];
}

void
op_halt(struct vm *vm)
{
	/* n/a */
}
