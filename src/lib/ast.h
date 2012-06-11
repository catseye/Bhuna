#ifndef __AST_H_
#define __AST_H_

#include "vm.h"
#include "value.h"

struct builtin;
struct type;
struct symbol;
struct symbol_table;
struct scan_st;

struct ast_local {
	int			 index;
	int			 upcount;
	struct symbol		*sym;
};

struct ast_value {
	struct value		 value;
};

struct ast_builtin {
	struct builtin		*bi;
	struct ast		*right;		/* ISA arg */
};

struct ast_apply {
	struct ast		*left;		/* ISA var(/...?)  (fn/cmd) */
	struct ast		*right;		/* ISA arg */
	int			 is_pure;
};

struct ast_arg {
	struct ast		*left;		/* ISA arg/apply/var */
	struct ast		*right;		/* ISA arg/apply/var */
};

struct ast_routine {
	struct ast		*body;
};

struct ast_statement {
	struct ast		*left;		/* ISA statement/apply */
	struct ast		*right;		/* ISA statement/apply */
};

struct ast_assignment {
	struct ast		*left;		/* ISA var */
	struct ast		*right;		/* ISA apply/var */
	int			 defining;
};

struct ast_conditional {
	struct ast		*test;		/* ISA apply/var */
	struct ast		*yes;		/* ISA statement/apply */
	struct ast		*no;		/* ISA statement/apply/NULL */
	/*int			 index;*/		/* temp var in act rec */
};

struct ast_while_loop {
	struct ast		*test;		/* ISA apply/var */
	struct ast		*body;		/* ISA statement/apply */
};

struct ast_retr {
	struct ast		*body;		/* ISA apply/var */
};

#define AST_LOCAL	 1
#define AST_VALUE	 2
#define	AST_BUILTIN	 3
#define	AST_APPLY	 4
#define	AST_ARG		 5
#define	AST_ROUTINE	 6
#define	AST_STATEMENT	 7
#define	AST_ASSIGNMENT	 8
#define	AST_CONDITIONAL	 9
#define	AST_WHILE_LOOP	10
#define	AST_RETR	11

union ast_union {
	struct ast_local	local;
	struct ast_value	value;
	struct ast_builtin	builtin;
	struct ast_apply	apply;
	struct ast_arg		arg;
	struct ast_routine	routine;
	struct ast_statement	statement;
	struct ast_assignment	assignment;
	struct ast_conditional	conditional;
	struct ast_while_loop	while_loop;
	struct ast_retr		retr;
};

struct ast {
	int				 type;
	struct scan_st			*sc;
	struct type			*datatype;
	vm_label_t			 label;
	union ast_union			 u;
};

struct ast		*ast_new_local(struct symbol_table *, struct symbol *);
struct ast		*ast_new_value(struct value, struct type *);
struct ast		*ast_new_builtin(struct scan_st *, struct builtin *, struct ast *);
struct ast		*ast_new_apply(struct scan_st *, struct ast *, struct ast *, int);
struct ast		*ast_new_arg(struct ast *, struct ast *);
struct ast		*ast_new_routine(struct ast *);
struct ast		*ast_new_statement(struct ast *, struct ast *);
struct ast		*ast_new_assignment(struct scan_st *, struct ast *, struct ast *, int);
struct ast		*ast_new_conditional(struct scan_st *, struct ast *, struct ast *, struct ast *);
struct ast		*ast_new_while_loop(struct scan_st *, struct ast *, struct ast *);
struct ast		*ast_new_retr(struct ast *);
void			 ast_free(struct ast *);

struct ast		*ast_find_local(struct ast *);
int			 ast_is_constant(struct ast *);
int			 ast_count_args(struct ast *);

void			 ast_dump(struct ast *, int);
char			*ast_name(struct ast *);

#endif /* !__AST_H_ */
