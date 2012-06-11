#ifndef __AST_H_
#define __AST_H_

struct value;
struct builtin;

struct ast_local {
	int			 index;
	int			 upcount;
#ifdef DEBUG
	struct symbol		*sym;
#endif
};

struct ast_value {
	struct value		*value;
};

struct ast_builtin {
	struct ast		*left;		/* ISA var(/...?)  (fn/cmd) */
	struct ast		*right;		/* ISA arg */
	struct builtin		*bi;
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

struct ast_statement {
	struct ast		*left;		/* ISA statement/apply */
	struct ast		*right;		/* ISA statement/apply */
};

struct ast_assignment {
	struct ast		*left;		/* ISA var */
	struct ast		*right;		/* ISA apply/var */
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
#define	AST_STATEMENT	 6
#define	AST_ASSIGNMENT	 7
#define	AST_CONDITIONAL	 8
#define	AST_WHILE_LOOP	 9
#define	AST_RETR	10

union ast_union {
	struct ast_local	local;
	struct ast_value	value;
	struct ast_builtin	builtin;
	struct ast_apply	apply;
	struct ast_arg		arg;
	struct ast_statement	statement;
	struct ast_assignment	assignment;
	struct ast_conditional	conditional;
	struct ast_while_loop	while_loop;
	struct ast_retr		retr;
};

struct ast {
	int				is_constant;
	int				type;
	union ast_union			u;
};

struct ast		*ast_new_local(int, int, void *);
struct ast		*ast_new_value(struct value *);
struct ast		*ast_new_builtin(struct ast *, struct ast *, struct builtin *);
struct ast		*ast_new_apply(struct ast *, struct ast *, int);
struct ast		*ast_new_arg(struct ast *, struct ast *);
struct ast		*ast_new_statement(struct ast *, struct ast *);
struct ast		*ast_new_assignment(struct ast *, struct ast *);
struct ast		*ast_new_conditional(struct ast *, struct ast *, struct ast *);
struct ast		*ast_new_while_loop(struct ast *, struct ast *);
struct ast		*ast_new_retr(struct ast *);
void			 ast_free(struct ast *);

void			 ast_dump(struct ast *, int);
char			*ast_name(struct ast *);

void			 ast_eval_init(void);
void			 ast_eval(struct ast *, struct value **);

int			 ast_is_constant(struct ast *);

#endif /* !__AST_H_ */
