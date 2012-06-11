#ifndef __VM_H_
#define __VM_H_

struct ast;

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
#define	INSTR_DEEP_COPY		138

vm_label_t	 ast_gen(struct ast *);
void		 vm_run(vm_label_t);
void		 vm_release(vm_label_t);

#endif
