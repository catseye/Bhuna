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
 * parse.c
 * Recursive-descent parser for Bhuna.
 * $Id: parse.c 54 2004-04-23 22:51:09Z catseye $
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scan.h"
#include "parse.h"
#include "symbol.h"
#include "value.h"
#include "atom.h"
#include "ast.h"

#define VAR_LOCAL		0
#define VAR_GLOBAL		1

#define VAR_MUST_EXIST		0
#define VAR_MAY_EXIST		1
#define VAR_MUST_NOT_EXIST	2

/*  --- util --- */

/*
 * Convenience function to create AST for a named function call on 2 arguments.
 */
static struct ast *
ast_new_nfcall(char *name, struct symbol_table *stab, struct ast *a, struct ast *b)
{
	struct symbol *sym;
	/*struct ast *l, *r = NULL;*/

	sym = symbol_lookup(stab, name, VAR_GLOBAL);
	assert(sym->builtin != NULL);
	a = ast_new_builtin(a, b, sym->builtin);
	/*
	} else if (sym->value != NULL) {
		r = ast_new_arg(a, b);
		l = ast_new_value(sym->value);
		a = ast_new_apply(l, r, sym->is_pure);
	} else {
		r = ast_new_arg(a, b);
		l = ast_new_local(sym->index, stab->level - sym->in->level, sym);
		a = ast_new_apply(l, r, sym->is_pure);
	}
	*/
	return(a);
}

/* ---------------- TOP LEVEL ----------------*/

struct ast *
parse_program(struct scan_st *sc, struct symbol_table *stab)
{
	struct ast *a = NULL;
	int cc;

	while (sc->type != TOKEN_EOF) {
		cc = 0;
		a = ast_new_statement(a, parse_statement(sc, stab, &cc));
	}

	return(a);
}

/*
 * Caller must allocate *istab if they want us to have one.
 * Otherwise they should set it to NULL, and we'll just use the one above.
 */
struct ast *
parse_block(struct scan_st *sc, struct symbol_table *stab,
	    struct symbol_table **istab, int *cc)
{
	struct ast *a = NULL;
/*	struct value *v;
	struct symbol *sym; */

	assert(*istab != NULL);

	if (tokeq(sc, "{")) {
		scan_expect(sc, "{");
		while (tokne(sc, "}") && sc->type != TOKEN_EOF) {
			a = ast_new_statement(a, parse_statement(sc, *istab, cc));
		}
		scan_expect(sc, "}");
	} else {
		a = parse_statement(sc, *istab, cc);
	}

	/*
	 * For housekeeping, we place a reference to this symbol table
	 * in an anonymous symbol in the overlying symbol table.
	 */
	/*
	if (!symbol_table_is_empty(*istab) != NULL) {
		sym = symbol_define(stab, NULL, SYM_KIND_ANONYMOUS);
		v = value_new_symbol_table(*istab);
		symbol_set_value(sym, v);
		value_release(v);
	}
	*/

	return(a);
}

struct ast *
parse_statement(struct scan_st *sc, struct symbol_table *stab, int *cc)
{
	struct symbol_table *istab;
	/*struct symbol *sym;*/
	struct ast *a, *l, *r;

	if (tokeq(sc, "{")) {
		istab = symbol_table_new(stab, 0);
		a = parse_block(sc, stab, &istab, cc);
	} else if (tokeq(sc, "if")) {
		scan(sc);
		/*
		 * Create a temporary value for this test.
		 */
		/*
		sym = symbol_define(stab, NULL, SYM_KIND_ANONYMOUS, NULL);
		*/
		a = parse_expr(sc, stab, 0, cc);
		istab = symbol_table_new(stab, 0);
		l = parse_block(sc, stab, &istab, cc);
		if (tokeq(sc, "else")) {
			scan(sc);
			istab = symbol_table_new(stab, 0);
			r = parse_block(sc, stab, &istab, cc);
		} else {
			r = NULL;
		}
		/*
		a = ast_new_conditional(a, l, r, sym->index);
		*/
		a = ast_new_conditional(a, l, r);
	} else if (tokeq(sc, "while")) {
		scan(sc);
		l = parse_expr(sc, stab, 0, cc);
		istab = symbol_table_new(stab, 0);
		r = parse_block(sc, stab, &istab, cc);
		a = ast_new_while_loop(l, r);
	} else if (tokeq(sc, "return")) {
		scan(sc);
		a = parse_expr(sc, stab, 0, cc);
		a = ast_new_retr(a);
	} else {
		int is_const = 0;
		int is_assign = 0;

		while (tokeq(sc, "local") || tokeq(sc, "const")) {
			is_assign = 1;
			if (tokeq(sc, "local")) {
				scan(sc);
				/* Not much, mere presence works. */
			} else if (tokeq(sc, "const")) {
				scan(sc);
				is_const = 1;
			}
		}
		if (is_assign || symbol_lookup(stab, sc->token, VAR_GLOBAL) == NULL) {
			/*
			 * Symbol doesn't exist at all - it MUST be
			 * an assignment, and it MUST be local.
			 */
			a = parse_assignment(sc, stab, is_const, cc);
		} else {
			a = parse_command(sc, stab, cc);
		}
	}
	if (tokeq(sc, ";"))
		scan(sc);
	return(a);
}

struct ast *
parse_assignment(struct scan_st *sc, struct symbol_table *stab, int is_const,
		 int *cc)
{
	struct symbol *sym;
	struct ast *l, *r;
	struct value *v = NULL;

	if (is_const) {
		/*
		 * This is just a placeholder.
		 */
		v = value_new_integer(42);
	}
	l = parse_var(sc, stab, &sym, VAR_LOCAL, VAR_MUST_NOT_EXIST, v);
	value_release(v);
	scan_expect(sc, "=");
	r = parse_expr(sc, stab, 0, cc);
	if (is_const) {
		if (!ast_is_constant(r)) {
			scan_error(sc, "Expression must be constant");
		} else {
			/* XXX CONSTANT FOLDING PLEASE */
			/*
			ast_eval(r, &v);
			printf("Constant value = ");
			value_print(v);
			printf("\n");
			symbol_set_value(sym, v);
		
			ast_free(l);
			ast_free(r);
			*/
		}
		return(NULL);
	} else {
		return(ast_new_assignment(l, r));
	}
}

struct ast *
parse_command(struct scan_st *sc, struct symbol_table *stab,
	      int *cc)
{
	struct symbol *sym;
	struct ast *a, *l, *r, *z;

	a = parse_var(sc, stab, &sym, VAR_GLOBAL, VAR_MUST_EXIST, NULL);
	if (tokeq(sc, "=")) {
		/*
		 * Actually... it's an assignment to an already-existing variable.
		 */
		if (sym->value != NULL) {
			scan_error(sc, "Value not modifiable");
		} else {
			scan(sc);
			r = parse_expr(sc, stab, 0, cc);
			a = ast_new_assignment(a, r);
		}
		return(a);
	}

	if (sym->builtin != NULL) {
		l = NULL; r = NULL;
		if (tokne(sc, "}") && tokne(sc, ";") && sc->type != TOKEN_EOF) {
			l = parse_expr(sc, stab, 0, cc);
			if (tokeq(sc, ",")) {
				scan_expect(sc, ",");
				r = parse_expr(sc, stab, 0, cc);
			}
		}
		a = ast_new_builtin(l, r, sym->builtin);
	} else {
		if (tokne(sc, "}") && tokne(sc, ";") && sc->type != TOKEN_EOF) {
			l = parse_expr(sc, stab, 0, cc);
			l = ast_new_arg(l, NULL);
			z = l;
			while (tokeq(sc, ",")) {
				scan_expect(sc, ",");
				r = parse_expr(sc, stab, 0, cc);
				r = ast_new_arg(r, NULL);
				z->u.arg.right = r;
				z = r;
			}
		} else {
			l = NULL;
		}
		a = ast_new_apply(a, l, 0);
	}

	return(a);
}

/* ------------------------- EXPRESSIONS ------------------------ */

int maxlevel = 3;

char *op[4][6] = {
	{ "&", "|",  "",  "",  "",   ""   },
	{ "=", "!=", ">", "<", ">=", "<=" },
	{ "+", "-",  "",  "",  "",   ""   },
	{ "*", "/",  "%", "",  "",   ""   }
};

struct ast *
parse_expr(struct scan_st *sc, struct symbol_table *stab, int level,
	   int *cc)
{
	struct ast *l, *r;
	int done = 0, i = 0;
	char the_op[256];

	if (level > maxlevel) {
		l = parse_primitive(sc, stab, cc);
		return(l);
	} else {
		l = parse_expr(sc, stab, level + 1, cc);
		while (!done) {
			done = 1;
			for (i = 0; i < 6 && op[level][i][0] != '\0'; i++) {
				if (tokeq(sc, op[level][i])) {
					strlcpy(the_op, sc->token, 256);
					scan(sc);
					done = 0;
					r = parse_expr(sc, stab, level + 1, cc);
					l = ast_new_nfcall(the_op, stab, l, r);
					break;
				}
			}
		}
		return(l);
	}
}

struct ast *
parse_primitive(struct scan_st *sc, struct symbol_table *stab, int *cc)
{
	struct ast *a, *l, *r, *z;
	struct value *v;
	struct symbol *sym;
	struct symbol_table *istab;

	if (tokeq(sc, "(")) {
		scan(sc);
		a = parse_expr(sc, stab, 0, cc);
		scan_expect(sc, ")");
	} else if (tokeq(sc, "^")) {
		int my_cc = 0;

		/*
		 * Enclosing block contains a closure:
		 */
		(*cc)++;
		scan(sc);
		istab = symbol_table_new(stab, 1);
		while (tokne(sc, "{") && sc->type != TOKEN_EOF) {
			a = parse_var(sc, istab, &sym,
			    VAR_LOCAL, VAR_MUST_NOT_EXIST, NULL);
			ast_free(a);
			if (tokeq(sc, ","))
				scan(sc);
		}
		a = parse_block(sc, stab, &istab, &my_cc);
		v = value_new_closure(a, NULL, symbol_table_size(istab), my_cc);
		a = ast_new_value(v);
		value_release(v);
	} else if (tokeq(sc, "!")) {
		scan(sc);
		a = parse_primitive(sc, stab, cc);
		sym = symbol_lookup(stab, "!", 1);
		a = ast_new_apply(ast_new_local(
		    sym->index,
		    stab->level - sym->in->level,
		    sym), a, 1);
	} else if (tokeq(sc, "[")) {
		scan(sc);
		v = value_new_list();
		a = ast_new_value(v);
		value_release(v);
		if (tokne(sc, "]")) {
			r = parse_expr(sc, stab, 0, cc);
			a = ast_new_nfcall("Cons", stab, a, r);
			while (tokeq(sc, ",")) {
				scan(sc);
				r = parse_expr(sc, stab, 0, cc);
				a = ast_new_nfcall("Cons", stab, a, r);
			}
		}
		scan_expect(sc, "]");
	} else if (sc->type == TOKEN_BAREWORD && isupper(sc->token[0])) {
		a = parse_var(sc, stab, &sym, VAR_GLOBAL, VAR_MUST_EXIST, NULL);
		while (tokeq(sc, "(") || tokeq(sc, "[") || tokeq(sc, ".")) {
			if (tokeq(sc, "(")) {
				scan(sc);
				if (tokne(sc, ")")) {
					l = parse_expr(sc, stab, 0, cc);
					l = ast_new_arg(l, NULL);
					z = l;
					while (tokeq(sc, ",")) {
						scan(sc);
						r = parse_expr(sc, stab, 0, cc);
						r = ast_new_arg(r, NULL);
						z->u.arg.right = r;
						z = r;
					}
				} else {
					l = NULL;
				}
				scan_expect(sc, ")");
				a = ast_new_apply(a, l, sym->is_pure);
			} else if (tokeq(sc, "[")) {
				scan(sc);
				r = parse_expr(sc, stab, 0, cc);
				scan_expect(sc, "]");
				a = ast_new_nfcall("Index", stab, a, r);
			} else if (tokeq(sc, ".")) {
				scan(sc);
				r = parse_literal(sc, stab);
				a = ast_new_nfcall("Index", stab, a, r);
			}
		}
	} else {
		a = parse_literal(sc, stab);
	}

	return(a);
}

struct ast *
parse_literal(struct scan_st *sc, struct symbol_table *stab)
{
	struct ast *a;
	struct value *v;

	if (sc->type == TOKEN_BAREWORD && islower(sc->token[0])) {
		v = value_new_atom(atom_resolve(sc->token));
		a = ast_new_value(v);
		value_release(v);
		scan(sc);		
	} else if (sc->type == TOKEN_NUMBER) {
		v = value_new_integer(atoi(sc->token));
		a = ast_new_value(v);
		value_release(v);
		scan(sc);
	} else if (sc->type == TOKEN_QSTRING) {
		v = value_new_string(sc->token);
		a = ast_new_value(v);
		value_release(v);
		scan(sc);
	} else {
		scan_error(sc, "Illegal literal");
		scan(sc);
		a = NULL;
	}
	
	return(a);
}

struct ast *
parse_var(struct scan_st *sc, struct symbol_table *stab,
	  struct symbol **sym,
	   int globality, int existence,
	   struct value *v)
{
	struct ast *a;

	*sym = symbol_lookup(stab, sc->token, globality);
	if (*sym == NULL) {
		if (existence == VAR_MUST_EXIST) {
			scan_error(sc, "Undefined symbol");
		}
		*sym = symbol_define(stab, sc->token, SYM_KIND_VARIABLE, v);
	} else {
		if (existence == VAR_MUST_NOT_EXIST) {
			scan_error(sc, "Symbol already defined");
		}
	}
	scan(sc);

	if ((*sym)->value != NULL) {
		a = ast_new_value((*sym)->value);
	} else {
		a = ast_new_local((*sym)->index, stab->level - (*sym)->in->level, (*sym));
	}
	return(a);
}
