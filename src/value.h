/*
 * value.h
 * Values for Bhuna.
 * $Id$
 */

#ifndef __VALUE_H_
#define __VALUE_H_

struct symbol;
struct symbol_table;
struct list;
struct value;
struct closure;
struct ast;
struct activation;

#define	VALUE_VOID	 0
#define	VALUE_INTEGER	 1
#define	VALUE_BOOLEAN	 2
#define VALUE_ATOM	 3
#define	VALUE_STRING	 4
#define	VALUE_LIST	 5
#define	VALUE_STAB	 6
#define	VALUE_ERROR	 7
#define	VALUE_BUILTIN	 8
#define VALUE_CLOSURE	 9
#define VALUE_DICT	10
#define VALUE_SYMBOL	11	/* ??? */
#define VALUE_OPAQUE	15

union value_union {
	int			 i;
	int			 b;
	int			 atom;
	char			*s;
	struct list		*l;
	struct symbol_table	*stab;
	char			*e;
	void			(*f)(struct value **);
	struct closure		*k;
	/* struct dict		*d; */
	struct symbol		*sym;
};

struct value {
	int			type;
	int			refcount;
	union value_union	v;
};

struct value	*value_new(int);

struct value	*value_new_integer(int);
struct value	*value_new_boolean(int);
struct value	*value_new_atom(int);
struct value	*value_new_string(char *);
struct value	*value_new_list(void);
struct value	*value_new_symbol_table(struct symbol_table *);
struct value	*value_new_error(char *);
struct value	*value_new_builtin(void (*)(struct value **));
struct value	*value_new_closure(struct ast *, struct activation *);
struct value	*value_new_symbol(struct symbol *);

void		 value_set_integer(struct value **, int);
void		 value_set_boolean(struct value **, int);
void		 value_set_atom(struct value **, int);
void		 value_set_string(struct value **, char *);
void		 value_set_list(struct value **);
void		 value_set_symbol_table(struct value **, struct symbol_table *);
void		 value_set_error(struct value **, char *);
void		 value_set_builtin(struct value **, void (*)(struct value **));
void		 value_set_closure(struct value **, struct ast *, struct activation *);
void		 value_set_symbol(struct value **, struct symbol *);

void		 value_set_from_value(struct value **, struct value *);

void		 value_grab(struct value *);
void		 value_release(struct value *);

void		 value_list_append(struct value **, struct value *);

void		 value_print(struct value *);
int		 value_equal(struct value *, struct value *);

void		 value_dump_global_table(void);

#endif /* !__VALUE_H_ */
