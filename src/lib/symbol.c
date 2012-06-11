/*
 * symbol.c
 * Symbol and symbol table housekeeping and manipulation for Bhuna.
 * $Id: symbol.c 54 2004-04-23 22:51:09Z catseye $
 */

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "symbol.h"
#include "type.h"
#include "value.h"

/*** GLOBALS ***/

static int anon_counter = 0;

/*** STATICS ***/

static struct symbol *
symbol_new(char *token, int kind)
{
	struct symbol *sym;

	sym = bhuna_malloc(sizeof(struct symbol));

	if (token == NULL) {
		asprintf(&sym->token, "%%%d", ++anon_counter);
	} else {
		sym->token = bhuna_strdup(token);
	}

	sym->kind = kind;
	sym->in = NULL;
	sym->index = -1;
	sym->is_pure = 0;
	sym->type = NULL;
	sym->value = NULL;
	sym->builtin = NULL;

	return(sym);
}

static void
symbol_free(struct symbol *sym)
{
	/* attached types and values take care of themselves now... */
	bhuna_free(sym->token);
	bhuna_free(sym);
}

/*** CONSTRUCTOR/DESTRUCTOR ***/

/*
 * Offset is how many levels deeper this symbol table should be from
 * its parent.  Generally, closures use 1, anonymous blocks (like if
 * and while) use 0 (same level.)
 */
struct symbol_table *
symbol_table_new(struct symbol_table *parent, int offset)
{
	struct symbol_table *stab;

	stab = bhuna_malloc(sizeof(struct symbol_table));

	stab->head = NULL;
	stab->next_index = 0;

	stab->parent = parent;
	if (parent != NULL)
		stab->level = stab->parent->level + offset;
	else
		stab->level = 0;

	return(stab);
}

void
symbol_table_free(struct symbol_table *stab)
{
	struct symbol *s, *t;

	s = stab->head;
	while (s != NULL) {
		t = s->next;
		symbol_free(s);
		s = t;
	}

	bhuna_free(stab);
}

struct symbol_table *
symbol_table_root(struct symbol_table *stab)
{
	if (stab->parent == NULL)
		return(stab);
	else
		return(symbol_table_root(stab->parent));
}

int
symbol_table_size(struct symbol_table *stab)
{
	return(stab->next_index);
}

/*** OPERATIONS ***/

/*
 * Define a new symbol.
 * Assumption: a preceding call to symbol_lookup failed.
 * If token == NULL, a new anonymous symbol is created.
 */
struct symbol *
symbol_define(struct symbol_table *stab, char *token, int kind, struct value *v)
{
	struct symbol *new_sym;

	new_sym = symbol_new(token, kind);

	new_sym->in = stab;
	if (v == NULL) {
		struct symbol_table *defining_table;

		/*
		 * Find allocation offset for the symbol's value.
		 */
		defining_table = stab;
		while (defining_table->parent != NULL &&
		    defining_table->parent->level == defining_table->level)
			defining_table = defining_table->parent;
		new_sym->index = defining_table->next_index++;
		/*printf("%s->index = %d\n", new_sym->token, new_sym->index);*/
	} else {
		new_sym->value = v;
	}
	new_sym->next = stab->head;
	stab->head = new_sym;

	return(new_sym);
}

struct symbol *
symbol_lookup(struct symbol_table *stab, char *s, int global)
{
	struct symbol *sym;

	for (sym = stab->head; sym != NULL; sym = sym->next)
		if (strcmp(s, sym->token) == 0)
			return(sym);
	if (global && stab->parent != NULL)
		return(symbol_lookup(stab->parent, s, global));
	return(NULL);
}

int
symbol_is_global(struct symbol *sym)
{
	return(sym->in->parent == NULL);
}

void
symbol_set_type(struct symbol *sym, struct type *t)
{
	assert(sym->type == NULL);
	sym->type = t;
}

void
symbol_set_value(struct symbol *sym, struct value *v)
{
	assert(sym->value != NULL);
	sym->value = v;
}

#ifdef DEBUG
static int stab_indent = 0;
#endif

void
symbol_table_dump(struct symbol_table *stab, int show_ast)
{
#ifdef DEBUG
	struct symbol *s;
	int i;

	printf("STAB<\n");
	stab_indent += 4;
	for (s = stab->head; s != NULL; s = s->next) {
		symbol_dump(s, show_ast);
		/* if (s->next != NULL) */
		printf("\n");
	}
	stab_indent -= 4;
	for (i = 0; i < stab_indent; i++)
		printf(" ");
	printf(">");
#endif
}

void
symbol_dump(struct symbol *sym, int show_ast)
{
#ifdef DEBUG
	int i;

	for (i = 0; i < stab_indent; i++)
		printf(" ");
	printf("`%s'(%08lx)", sym->token, (unsigned long)sym);
	type_print(stdout, sym->type);
	if (sym->value != NULL) {
		printf("=");
		value_print(sym->value);
	}
#endif
}

void
symbol_print(FILE *f, struct symbol *sym)
{
#ifdef DEBUG
	fprintf(f, "symbol `%s' (type = ", sym->token);
	type_print(f, sym->type);
	fprintf(f, ")");
#endif
}
