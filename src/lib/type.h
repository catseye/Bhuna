#include <stdio.h>

struct scan_st;

#define	TYPE_VOID	 0
#define	TYPE_INTEGER	 1
#define	TYPE_BOOLEAN	 2
#define TYPE_ATOM	 3
#define	TYPE_STRING	 4
#define	TYPE_LIST	 5
#define	TYPE_ERROR	 6
#define	TYPE_BUILTIN	 7
#define TYPE_CLOSURE	 8
#define TYPE_DICT	 9
#define TYPE_OPAQUE	15
#define TYPE_VAR	16
#define TYPE_ARG	17
#define TYPE_SET	18

struct type_list {
	struct type *contents;
};

struct type_dict {
	struct type *index;
	struct type *contents;
};

struct type_closure {
	struct type *domain;
	struct type *range;
};

/* type of a list of arguments given to a function, c.f. ast_arg */
struct type_arg {
	struct type *left;
	struct type *right;
};

/* union of several heterogenous types... :) */
struct type_set {
	struct type *left;
	struct type *right;
};

struct type_var {
	int num;
};

union type_union {
	struct type_list	list;
	struct type_dict	dict;
	struct type_closure	closure;
	struct type_arg		arg;
	struct type_set		set;
	struct type_var		var;
};

struct type {
	struct type *next;	/* for freein' */
	int tclass;
	struct type *unifier;	/* equiv. class under type unif. */
	union type_union t;
};

struct type	*type_new(int);
struct type	*type_new_list(struct type *);
struct type	*type_new_dict(struct type *, struct type *);
struct type	*type_new_closure(struct type *, struct type *);
struct type	*type_new_arg(struct type *, struct type *);
struct type	*type_new_set(struct type *, struct type *);
struct type	*type_new_var(int);
struct type	*type_brand_new_var(void);

void		 types_free(void);

int		 type_equal(struct type *, struct type *);
int		 type_unify(struct type *, struct type *);
struct type	*type_representative(struct type *);

void		 type_ensure_routine(struct type *);
int		 type_is_possibly_routine(struct type *);
int		 type_unify_crit(struct scan_st *, struct type *, struct type *);
int		 type_is_void(struct type *);
int		 type_is_set(struct type *);
int		 type_set_contains_void(struct type *);

void		 type_print(FILE *, struct type *);
