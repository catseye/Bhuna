/*
 * Copyright (c)2004 Cat's Eye Technologies.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * 
 *   Neither the name of Cat's Eye Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
/*
 * symbol.c
 * Symbol and symbol table housekeeping and manipulation for Bhuna.
 * $Id: symbol.c 54 2004-04-23 22:51:09Z catseye $
 */

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "symbol.h"

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

	return(sym);
}

static void
symbol_free(struct symbol *sym)
{
	bhuna_free(sym->token);
	bhuna_free(sym);
}

/*** CONSTRUCTOR/DESTRUCTOR ***/

struct symbol_table *
symbol_table_new(struct symbol_table *parent)
{
	struct symbol_table *stab;

	stab = bhuna_malloc(sizeof(struct symbol_table));
	stab->parent = parent;
	stab->head = NULL;

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
symbol_table_is_empty(struct symbol_table *stab)
{
	return(stab->head == NULL);
}

/*** OPERATIONS ***/

/*
 * Define a new symbol.
 * Assumption: a preceding call to symbol_lookup failed.
 * If token == NULL, a new anonymous symbol is created.
 */
struct symbol *
symbol_define(struct symbol_table *stab, char *token, int kind)
{
	struct symbol *new_sym;

	new_sym = symbol_new(token, kind);

	new_sym->in = stab;
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
#endif
}
