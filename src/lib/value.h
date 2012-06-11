/*
 * value.h
 * Values for Bhuna.
 * $Id$
 */

#ifndef __VALUE_H_
#define __VALUE_H_

struct list;
struct value;
struct closure;
struct ast;
struct activation;
struct dict;
struct builtin;

#ifdef HASH_CONSING
#define HASH_CONS_SIZE	23211

struct hc_chain {
	struct hc_chain *next;
	struct value *v;
};
#endif

#define	VALUE_VOID	 0
#define	VALUE_INTEGER	 1
#define	VALUE_BOOLEAN	 2
#define VALUE_ATOM	 3
#define	VALUE_STRING	 4
#define	VALUE_LIST	 5
#define	VALUE_ERROR	 6
#define	VALUE_BUILTIN	 7
#define VALUE_CLOSURE	 8
#define VALUE_DICT	 9
#define VALUE_OPAQUE	15

union value_union {
	int			 i;
	int			 b;
	int			 atom;
	char			*s;
	struct list		*l;
	char			*e;
	struct builtin		*bi;
	struct closure		*k;
	struct dict		*d;
};

#define	ADMIN_FREE		1	/* on the free list */
#define	ADMIN_MARKED		2	/* marked, during gc */
#define	ADMIN_PERMANENT		4	/* don't EVER gc this 'k? */

struct value {
	struct value		*next;
	unsigned short int	 admin;
	unsigned short int	 type;
	int			 refcount;
	union value_union	 v;
};

void		 value_free(struct value *);

struct value	*value_dup(struct value *);

struct value	*value_new_integer(int);
struct value	*value_new_boolean(int);
struct value	*value_new_atom(int);
struct value	*value_new_string(char *);
struct value	*value_new_list(void);
struct value	*value_new_error(char *);
struct value	*value_new_builtin(struct builtin *);
struct value	*value_new_closure(struct ast *, struct activation *, int, int, int);
struct value	*value_new_dict(void);

void		 value_deregister(struct value *);

void		 value_set_from_value(struct value **, struct value *);

void		 value_list_append(struct value **, struct value *);

void		 value_dict_store(struct value **, struct value *, struct value *);

void		 value_print(struct value *);
int		 value_equal(struct value *, struct value *);

void		 value_dump_global_table(void);

#endif /* !__VALUE_H_ */
