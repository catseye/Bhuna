/*
 * symbol.h
 * Symbol structures and prototypes for Bhuna.
 * $Id: symbol.h 54 2004-04-23 22:51:09Z catseye $
 */

#ifndef __SYMBOL_H_
#define __SYMBOL_H_

struct symbol_table {
	struct symbol_table	*parent;	/* link to scopes above us */
	struct symbol		*head;		/* first symbol in table */
};

struct symbol {
	struct symbol_table	*in;	/* link to table we're in */
	struct symbol		*next;	/* next symbol in symbol table */
	char			*token;	/* lexeme making up the symbol */
	int			 kind;	/* kind of symbol */
};

#define SYM_KIND_ANONYMOUS	0
#define SYM_KIND_COMMAND	1
#define SYM_KIND_FUNCTION	2
#define SYM_KIND_VARIABLE	3

struct symbol_table	*symbol_table_new(struct symbol_table *);
struct symbol_table	*symbol_table_dup(struct symbol_table *);
void			 symbol_table_free(struct symbol_table *);

struct symbol_table	*symbol_table_root(struct symbol_table *);

int			 symbol_table_is_empty(struct symbol_table *);

struct symbol		*symbol_define(struct symbol_table *, char *, int);
struct symbol		*symbol_lookup(struct symbol_table *, char *, int);

int			 symbol_is_global(struct symbol *);

void			 symbol_table_dump(struct symbol_table *, int);
void			 symbol_dump(struct symbol *, int);

#endif /* !__SYMBOL_H_ */
