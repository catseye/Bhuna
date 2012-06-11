#include "mem.h"
#include "vm.h"

struct vm {
	size_t pc;
	value **stack;
	void (**f)(struct vm *);
}


struct vm *
vm_new(size_t prog_size, size_t stack_size)
{
	struct vm *vm;

	vm = bhuna_malloc(sizeof(struct vm));
	vm->prog =  bhuna_malloc(void (*)(struct vm *) * stack_size);
	vm->stack = bhuna_malloc(sizeof(struct value *) * stack_size);
	vm->pc = vm->prog[0];
	vm->ctr = 0;

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
vm_run(struct vm *vm)
{
	while (vm->pc != op_halt) {
		vm->pc++;
	}
}

void
vm_gen(struct vm *vm, void (*ins)(struct vm *))
{
	vm->prog[vm->cnt++] = ins;
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
