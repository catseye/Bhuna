/*
 * symbol.h
 * Symbol structures and prototypes for Bhuna.
 * $Id: symbol.h 54 2004-04-23 22:51:09Z catseye $
 */

#ifndef __SYMBOL_H_
#define __SYMBOL_H_

#include <stdio.h>
#include <wchar.h>

#include "value.h"

struct type;

struct symbol_table {
	struct symbol_table	*parent;	/* link to scopes above us */
	struct symbol		*head;		/* first symbol in table */
	int			 next_index;	/* next index to be taken */
	int			 level;		/* lexical level of the table */
};

struct symbol {
	struct symbol_table	*in;	/* link to table we're in */
	struct symbol		*next;	/* next symbol in symbol table */
	wchar_t			*token;	/* lexeme making up the symbol */
	int			 kind;	/* kind of symbol */
	struct type		*type;	/* data type */

	struct builtin		*builtin;
	int			 is_pure; /* if true, symbol represents a function which is ref.transp. */
	int			 is_const; /* if true, symbol represents a constant */
	struct value		 value;	/* if symbol is a constant, this is the value */

	int			 index;	/* index into activation record */
};

#define SYM_KIND_ANONYMOUS	0
#define SYM_KIND_COMMAND	1
#define SYM_KIND_FUNCTION	2
#define SYM_KIND_VARIABLE	3

struct symbol_table	*symbol_table_new(struct symbol_table *, int);
struct symbol_table	*symbol_table_dup(struct symbol_table *);
void			 symbol_table_free(struct symbol_table *);

struct symbol_table	*symbol_table_root(struct symbol_table *);

int			 symbol_table_size(struct symbol_table *);

struct symbol		*symbol_define(struct symbol_table *, wchar_t *, int, struct value *);
struct symbol		*symbol_lookup(struct symbol_table *, wchar_t *, int);

int			 symbol_is_global(struct symbol *);

void			 symbol_set_type(struct symbol *, struct type *);
void			 symbol_set_value(struct symbol *, struct value);

void			 symbol_table_dump(struct symbol_table *, int);
void			 symbol_dump(struct symbol *, int);

void			 symbol_print(FILE *f, struct symbol *);

#endif /* !__SYMBOL_H_ */
