/*
 * value.h
 * Values for Bhuna.
 * $Id$
 */

#ifndef __VALUE_H_
#define __VALUE_H_

#include <wchar.h>

struct list;
struct value;
struct closure;
struct ast;
struct activation;
struct dict;
struct builtin;

/*
 * Structured values - strings, lists, dicts, and the like.
 * These are garbage collected and refcounted and so forth.
 */
struct s_value {
	struct s_value		*next;
	unsigned char		 admin;		/* ADMIN_ flags */
	unsigned char		 type;		/* VALUE_ */
	int			 refcount;
	union {
		wchar_t			*s;
		struct list		*l;
		char			*e;
		struct closure		*k;
		struct dict		*d;
	} v;
};

#define	ADMIN_FREE		1	/* on the free list */
#define	ADMIN_MARKED		2	/* marked, during gc */
#define	ADMIN_PERMANENT		4	/* don't EVER gc this 'k? */

/*
 * Simple values.
 * These are not garbage-collected, refcounted and so forth.
 */
struct value {
	unsigned char		type;		/* VALUE_ */
	union {
		int			 i;
		int			 b;
		int			 a;
		/* double d; */
		struct builtin		*bi;
		void			*ptr;
		struct s_value		*s;
	} v;
};

#define	VALUE_NULL	 0
#define	VALUE_INTEGER	 1
#define	VALUE_BOOLEAN	 2
#define VALUE_ATOM	 3
#define	VALUE_BUILTIN	 4
#define VALUE_OPAQUE	 5

#define	VALUE_STRUCTURED 8

#define	VALUE_STRING	 (VALUE_STRUCTURED | 0)
#define	VALUE_LIST	 (VALUE_STRUCTURED | 1)
#define	VALUE_ERROR	 (VALUE_STRUCTURED | 2)
#define VALUE_CLOSURE	 (VALUE_STRUCTURED | 3)
#define VALUE_DICT	 (VALUE_STRUCTURED | 4)

/* Prototypes */

struct value	value_null(void);

struct value	value_new_integer(int);
struct value	value_new_boolean(int);
struct value	value_new_atom(int);
struct value	value_new_opaque(void *);

struct value	value_new_string(wchar_t *);
struct value	value_new_list(void);
struct value	value_new_error(const char *);
struct value	value_new_builtin(struct builtin *);
struct value	value_new_closure(struct ast *, struct activation *, int, int, int);
struct value	value_new_dict(void);

void		s_value_free(struct s_value *);

struct value	value_dup(struct value);

void		value_deregister(struct value);

void		value_list_append(struct value, struct value);

void		value_dict_store(struct value, struct value, struct value);

void		value_print(struct value);
int		value_equal(struct value, struct value);

void		value_dump_global_table(void);

#endif /* !__VALUE_H_ */
