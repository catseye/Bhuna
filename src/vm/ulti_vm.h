#include <sys/types.h>

struct value;

struct vm {
	void (*pc)(struct vm *);
	struct value **stack;
	void (**prog)(struct vm *);
	int ctr;
}


struct vm	*vm_new(size_t, size_t);
void		 vm_free(struct vm *);
void		 vm_run(struct vm *);

void		 vm_gen(struct vm *, void (*)(struct vm *));

void		 op_local(struct vm *);
void		 op_value(struct vm *);
void		 op_apply(struct vm *);
void		 op_assign(struct vm *);
void		 op_jz(struct vm *);
void		 op_jmp(struct vm *);
void		 op_halt(struct vm *);
