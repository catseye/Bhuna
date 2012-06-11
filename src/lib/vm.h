/*
 * vm.h
 */

#ifndef __VM_H_
#define __VM_H_

struct ast;
struct value;
struct activation;

typedef	unsigned char *	vm_label_t;

#define	INSTR_HALT		128
#define	INSTR_PUSH_VALUE	129
#define	INSTR_PUSH_LOCAL	130
#define	INSTR_POP_LOCAL		131
#define	INSTR_JZ		132
#define	INSTR_JMP		133
#define	INSTR_CALL		134
#define	INSTR_RET		135
#define INSTR_GOTO		136
#define	INSTR_SET_ACTIVATION	137
#define	INSTR_COW_LOCAL		138
#define	INSTR_EXTERNAL		139

struct vm {
	vm_label_t	  program;	/* vm bytecode array */
	size_t		  prog_size;	/* size of bytecode array */
	vm_label_t	  pc;

	struct value	**vstack;	/* vm's working stack */
	size_t		  vstack_size;	/* size of array for stack */
	struct value	**vstack_ptr;	/* ptr to top of stack */

	vm_label_t	 *cstack;	/* vm's call stack */
	size_t		  cstack_size;	/* size of call stack array */
	vm_label_t	 *cstack_ptr;	/* ptr to top of call stack */

	unsigned char	 *astack;
	size_t		  astack_size;	/* size of activation stack */
	unsigned char	 *astack_ptr;	/* top of activation stack */
	
	struct activation *current_ar;	/* current activation record */
};

/*
 * Return codes for vm_run() function.
 */
#define	VM_TERMINATED	0	/* INSTR_HALT was executed */
#define	VM_RETURNED	1	/* INSTR_RET, but ar->caller was NULL */
#define	VM_TIME_EXPIRED	2	/* executed for entire time slice */
#define	VM_WAITING	3	/* entered a wait state (TBI) */

struct vm	*vm_new(vm_label_t, size_t);
void		 vm_free(struct vm *);

void		 ast_gen(vm_label_t *, struct ast *);
void		 vm_set_pc(struct vm *, vm_label_t);
int		 vm_run(struct vm *, int);

#endif
