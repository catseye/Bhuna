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

/*
#include "closure.h"
#include "dict.h"
*/

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

struct value {
	int			type;
	int			refcount;
	union value_union	v;
};

#ifdef REFCOUNTING_MACROS
#define value_release(v)	\
	if ((v) != NULL && (--((v)->refcount)) == 0) value_free((v));
#define	value_grab(v)		\
	if ((v) != NULL) (v)->refcount++;
#else
void		 value_grab(struct value *);
void		 value_release(struct value *);
#endif

void		 value_free(struct value *);

struct value	*value_dup(struct value *);

struct value	*value_new_integer(int);
struct value	*value_new_boolean(int);
struct value	*value_new_atom(int);
struct value	*value_new_string(char *);
struct value	*value_new_list(void);
struct value	*value_new_error(char *);
struct value	*value_new_builtin(struct builtin *);
struct value	*value_new_closure(struct ast *, struct activation *);
struct value	*value_new_dict(void);

void		 value_set_from_value(struct value **, struct value *);

void		 value_set_integer(struct value **, int);
void		 value_set_boolean(struct value **, int);
void		 value_set_atom(struct value **, int);
void		 value_set_string(struct value **, char *);
void		 value_set_list(struct value **);
void		 value_set_error(struct value **, char *);
void		 value_set_builtin(struct value **, struct builtin *);
void		 value_set_closure(struct value **, struct ast *, struct activation *);
void		 value_set_dict(struct value **);

void		 value_list_append(struct value **, struct value *);

void		 value_dict_store(struct value **, struct value *, struct value *);

void		 value_print(struct value *);
int		 value_equal(struct value *, struct value *);

void		 value_dump_global_table(void);

#endif /* !__VALUE_H_ */
