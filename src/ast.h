#ifndef __AST_H_
#define __AST_H_

struct value;
struct symbol;
struct list;

struct ast_sym {
	struct symbol		*sym;
};

struct ast_value {
	struct value		*value;
};

struct ast_scope {
	struct symbol_table	*local;
	struct ast		*body;
};

struct ast_apply {
	struct ast		*left;		/* ISA var(/...?)  (fn/cmd) */
	struct ast		*right;		/* ISA arg */
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
};

struct ast_while_loop {
	struct ast		*test;		/* ISA apply/var */
	struct ast		*body;		/* ISA statement/apply */
};

#define AST_SYM		1
#define AST_VALUE	2
#define AST_SCOPE	3
#define	AST_APPLY	4
#define	AST_ARG		5
#define	AST_STATEMENT	6
#define	AST_ASSIGNMENT	7
#define	AST_CONDITIONAL	8
#define	AST_WHILE_LOOP	9

union ast_union {
	struct ast_sym		sym;
	struct ast_value	value;
	struct ast_scope	scope;
	struct ast_apply	apply;
	struct ast_arg		arg;
	struct ast_statement	statement;
	struct ast_assignment	assignment;
	struct ast_conditional	conditional;
	struct ast_while_loop	while_loop;
};

struct ast {
	int				type;
	union ast_union			u;
};

struct ast		*ast_new_sym(struct symbol *);
struct ast		*ast_new_value(struct value *);
struct ast		*ast_new_scope(struct ast *, struct symbol_table *);
struct ast		*ast_new_apply(struct ast *, struct ast *);
struct ast		*ast_new_arg(struct ast *, struct ast *);
struct ast		*ast_new_statement(struct ast *, struct ast *);
struct ast		*ast_new_assignment(struct ast *, struct ast *);
struct ast		*ast_new_conditional(struct ast *, struct ast *, struct ast *);
struct ast		*ast_new_while_loop(struct ast *, struct ast *);
void			 ast_free(struct ast *);

void			 ast_dump(struct ast *, int);
char			*ast_name(struct ast *);

void			 ast_eval(struct ast *, struct value **);

#endif /* !__AST_H_ */
