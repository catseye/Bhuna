#ifndef __BUILTIN_H_
#define __BUILTIN_H_

#include <wchar.h>

struct value;
struct activation;
struct type;
struct symbol;
struct symbol_table;

struct builtin {
	const wchar_t *name;
	struct value (*fn)(struct activation *);
	struct type *(*ty)(void);
	int arity;
	int retval;
	int is_pure;
	int is_const;
	int index;
};

#define INDEX_BUILTIN_PRINT	 0
#define INDEX_BUILTIN_NOT	 1
#define INDEX_BUILTIN_AND	 2
#define INDEX_BUILTIN_OR	 3
#define INDEX_BUILTIN_EQU	 4
#define INDEX_BUILTIN_NEQ	 5
#define INDEX_BUILTIN_GT	 6
#define INDEX_BUILTIN_LT	 7
#define INDEX_BUILTIN_GTE	 8
#define INDEX_BUILTIN_LTE	 9
#define INDEX_BUILTIN_ADD	10
#define INDEX_BUILTIN_SUB	11
#define INDEX_BUILTIN_MUL	12
#define INDEX_BUILTIN_DIV	13
#define INDEX_BUILTIN_MOD	14
#define INDEX_BUILTIN_LIST	15
#define INDEX_BUILTIN_FETCH	16
#define INDEX_BUILTIN_STORE	17
#define INDEX_BUILTIN_DICT	18
#define INDEX_BUILTIN_SPAWN	19
#define INDEX_BUILTIN_SEND	20
#define INDEX_BUILTIN_RECV	21
#define INDEX_BUILTIN_SELF	22

#define	INDEX_BUILTIN_LAST	127

extern struct builtin builtins[];

struct value builtin_print(struct activation *);

struct value builtin_not(struct activation *);
struct value builtin_and(struct activation *);
struct value builtin_or(struct activation *);

struct value builtin_equ(struct activation *);
struct value builtin_neq(struct activation *);
struct value builtin_gt(struct activation *);
struct value builtin_lt(struct activation *);
struct value builtin_gte(struct activation *);
struct value builtin_lte(struct activation *);

struct value builtin_add(struct activation *);
struct value builtin_mul(struct activation *);
struct value builtin_sub(struct activation *);
struct value builtin_div(struct activation *);
struct value builtin_mod(struct activation *);

struct value builtin_list(struct activation *);
struct value builtin_fetch(struct activation *);
struct value builtin_store(struct activation *);

struct value builtin_dict(struct activation *);

struct value builtin_spawn(struct activation *);
struct value builtin_send(struct activation *);
struct value builtin_recv(struct activation *);
struct value builtin_self(struct activation *);

struct type		*btype_print(void);
struct type		*btype_unary_logic(void);
struct type		*btype_binary_logic(void);
struct type		*btype_equality(void);
struct type		*btype_compare(void);
struct type		*btype_arith(void);
struct type		*btype_list(void);
struct type		*btype_fetch(void);
struct type		*btype_store(void);
struct type		*btype_dict(void);

struct type		*btype_spawn(void);
struct type		*btype_send(void);
struct type		*btype_recv(void);
struct type		*btype_self(void);

struct symbol		*register_builtin(struct symbol_table *, struct builtin *);
void			 register_std_builtins(struct symbol_table *);
void			 load_builtins(struct symbol_table *, char *);

#endif
