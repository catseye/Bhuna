/*
 * icode.h
 */

#ifndef __ICODE_H_
#define __ICODE_H_

#include "vm.h"

struct value;
struct ast;
struct builtin;

struct iprogram {
	struct icode	*head;
	struct icode	*tail;
};

union icode_operand {
	struct {
		int index;
		int upcount;
	}		 local;
	struct value	*value;
	struct icode	*branch;
	struct builtin	*builtin;
};

#define ICOMEFROM_ICODE		0
#define	ICOMEFROM_CLOSURE	1

struct icomefrom {
	struct icomefrom	*next;
	int			 type;
	union {
		struct icode	*icode;
		struct closure	*closure;
	}			 referrer;
};

struct icode {
	struct icode		*next;
	struct icode		*prev;
	struct icomefrom	*referrers;	/* links back to all icodes that branch here */
	int			 number;	/* for identification porpoises */
	vm_label_t		 label;		/* corresponding instr in vm */
	unsigned char		 opcode;
	union icode_operand	 operand;
};

struct iprogram	*iprogram_new(void);
void		 iprogram_free(struct iprogram *);
void		 iprogram_dump(struct iprogram *, vm_label_t);

struct icode	*icode_new(struct iprogram *, int);
struct icode	*icode_new_local(struct iprogram *, int, int, int);
struct icode	*icode_new_value(struct iprogram *, int, struct value *);
struct icode	*icode_new_builtin(struct iprogram *, struct builtin *);

void		 icode_free(struct iprogram *, struct icode *);

void		 icode_set_branch(struct icode *, struct icode *);

struct iprogram	*ast_gen_iprogram(struct ast *);

void		 iprogram_eliminate_nops(struct iprogram *);
void		 iprogram_eliminate_dead_code(struct iprogram *);
void		 iprogram_optimize_tail_calls(struct iprogram *);
void		 iprogram_optimize_push_small_ints(struct iprogram *);

void		 referrer_unwire(struct icode *, struct icode *);
void		 referrers_rewire(struct icode *, struct icode *);

#endif
