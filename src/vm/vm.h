#include <sys/types.h>

struct value;

struct op {
	int	 opcode;
	value	*value;		/* for pushing values */
	int	 index;
	int	 upcount;	/* for pushing/popping locals */
}

#define OP_HALT		0
#define OP_PUSH_VALUE	1
#define OP_PUSH_LOCAL	2
#define OP_POP_LOCAL	3
#define OP_APPLY	4
#define OP_JZ		5
#define OP_JMP		6

struct vm {
	int pc;
	int sp;
	struct value **stack;
	struct op *prog;
}


struct vm	*vm_new(size_t, size_t);
void		 vm_free(struct vm *);
void		 vm_reset(struct vm *);
void		 vm_run(struct vm *);
void		 vm_gen(struct vm *, struct op);
