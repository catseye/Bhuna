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
#include "type.h"
#include "report.h"

#define VAR_LOCAL		0
#define VAR_GLOBAL		1

#define VAR_MUST_EXIST		0
#define VAR_MAY_EXIST		1
#define VAR_MUST_NOT_EXIST	2

/*  --- util --- */

/*
 * Convenience function to create AST for a named arity-2 function call.
 */
static struct ast *
ast_new_call2(char *name, struct scan_st *sc, struct symbol_table *stab,
	       struct ast *left, struct ast *right)
{
	struct symbol *sym;
	struct ast *a;

	right = ast_new_arg(right, NULL);
	left = ast_new_arg(left, right);

	sym = symbol_lookup(stab, name, VAR_GLOBAL);
	assert(sym != NULL && sym->builtin != NULL);
	a = ast_new_builtin(sc, sym->builtin, left);

	return(a);
}

static struct ast *
ast_new_call3(char *name, struct scan_st *sc, struct symbol_table *stab,
	       struct ast *left, struct ast *index, struct ast *right)
{
	struct symbol *sym;
	struct ast *a;

	right = ast_new_arg(right, NULL);
	index = ast_new_arg(index, right);
	left = ast_new_arg(left, index);

	sym = symbol_lookup(stab, name, VAR_GLOBAL);
	assert(sym != NULL && sym->builtin != NULL);
	a = ast_new_builtin(sc, sym->builtin, left);

	return(a);
}

/* ---------------- TOP LEVEL ----------------*/

struct ast *
parse_program(struct scan_st *sc, struct symbol_table *stab)
{
	struct ast *a = NULL;
	int cc, retr = 0;

	while (sc->type != TOKEN_EOF && !retr) {
		cc = 0;
		a = ast_new_statement(a, parse_statement(sc, stab, &retr, &cc));
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
	int retr = 0;

	assert(*istab != NULL);

	if (tokeq(sc, "{")) {
		scan_expect(sc, "{");
		while (tokne(sc, "}") && sc->type != TOKEN_EOF && !retr) {
			a = ast_new_statement(a,
			    parse_statement(sc, *istab, &retr, cc));
		}
		scan_expect(sc, "}");
	} else {
		a = parse_statement(sc, *istab, &retr, cc);
	}

	/*
	 * XXX
	 * For housekeeping, it would be nice to place a reference to this
	 * symbol table in the overlying symbol table, so that when it is
	 * dumped, this one is dumped too.
	 */

	return(a);
}

struct ast *
parse_statement(struct scan_st *sc, struct symbol_table *stab, int *retr, int *cc)
{
	struct symbol_table *istab;
	struct ast *a, *l, *r;

	if (tokeq(sc, "{")) {
		istab = symbol_table_new(stab, 0);
		a = parse_block(sc, stab, &istab, cc);
	} else if (tokeq(sc, "if")) {
		scan(sc);
		a = parse_expr(sc, stab, 0, NULL, cc);
		istab = symbol_table_new(stab, 0);
		l = parse_block(sc, stab, &istab, cc);
		if (tokeq(sc, "else")) {
			scan(sc);
			istab = symbol_table_new(stab, 0);
			r = parse_block(sc, stab, &istab, cc);
		} else {
			r = NULL;
		}
		a = ast_new_conditional(sc, a, l, r);
	} else if (tokeq(sc, "while")) {
		scan(sc);
		l = parse_expr(sc, stab, 0, NULL, cc);
		istab = symbol_table_new(stab, 0);
		r = parse_block(sc, stab, &istab, cc);
		a = ast_new_while_loop(sc, l, r);
	} else if (tokeq(sc, "return")) {
		scan(sc);
		a = parse_expr(sc, stab, 0, NULL, cc);
		a = ast_new_retr(a);
		*retr = 1;
	} else {
		int is_const = 0;
		int is_def = 0;

		while (tokeq(sc, "local") || tokeq(sc, "const")) {
			is_def = 1;
			if (tokeq(sc, "local")) {
				scan(sc);
				/* Not much, mere presence works. */
			} else if (tokeq(sc, "const")) {
				scan(sc);
				is_const = 1;
			}
		}
		if (is_def || symbol_lookup(stab, sc->token, VAR_GLOBAL) == NULL) {
			/*
			 * Symbol doesn't exist at all, so it
			 * must be a variable definition.
			 */
			a = parse_definition(sc, stab, is_const, cc);
		} else {
			/*
			 * Symbol already exists, so it could be
			 * either a command or an assignment.
			 */
			a = parse_command_or_assignment(sc, stab, cc);
		}
	}
	if (tokeq(sc, ";"))
		scan(sc);
	return(a);
}

struct ast *
parse_definition(struct scan_st *sc, struct symbol_table *stab, int is_const,
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
	r = parse_expr(sc, stab, 0, sym, cc);
	if (is_const) {
		if (r == NULL || r->type != AST_VALUE) {
			report(REPORT_ERROR, sc, "Expression must be constant");
		} else {
			symbol_set_value(sym, r->u.value.value);
			ast_free(l);
			ast_free(r);
		}
		return(NULL);
	} else {
		return(ast_new_assignment(sc, l, r));
	}
}

struct ast *
parse_command_or_assignment(struct scan_st *sc, struct symbol_table *stab,
			    int *cc)
{
	struct symbol *sym;
	struct ast *a, *l, *r;

	a = parse_var(sc, stab, &sym, VAR_GLOBAL, VAR_MUST_EXIST, NULL);

	/*
	 * A[I] = J	-> Store A, I, J
	 * A[A[I]] = J	-> Store A, A[I], J
	 * A[I][J] = K	-> Store A[I], J, K
	 * A[I][J][K] = L	-> Store A[I][J], K, L
	 */
	while (tokeq(sc, "[") || tokeq(sc, ".")) {
		if (tokeq(sc, "[")) {
			scan(sc);
			l = parse_expr(sc, stab, 0, NULL, cc);
			scan_expect(sc, "]");
			if (tokeq(sc, "=")) {
				/*
				 * It was the last one; this is an assigment.
				 */
				scan(sc);
				r = parse_expr(sc, stab, 0, NULL, cc);
				a = ast_new_call3("Store", sc, stab, a, l, r);
				return(a);
			} else if (tokne(sc, "[") && tokne(sc, ".")) {
				/*
				 * It was the last one; this is a command.
				 */
				/* ... */
			} else {
				/*
				 * Still more to go.
				 */
				a = ast_new_call2("Fetch", sc, stab, a, l);
			}
		} else if (tokeq(sc, ".")) {
			scan(sc);
			r = parse_literal(sc, stab);
			a = ast_new_call2("Fetch", sc, stab, a, r);
		}
	}
	
	/*
	 * If the variable-expression was followed by an equals sign,
	 * it's an assignment to an already-existing variable.
	 */
	if (tokeq(sc, "=")) {
		if (sym->value != NULL) {
			report(REPORT_ERROR, sc, "Value not modifiable");
		} else {
			scan(sc);
			r = parse_expr(sc, stab, 0, NULL, cc);
			a = ast_new_assignment(sc, a, r);
		}
		return(a);
	}

	if (tokne(sc, "}") && tokne(sc, ";") && sc->type != TOKEN_EOF) {
		l = parse_expr_list(sc, stab, NULL, cc);
	} else {
		l = NULL;
	}

	/*
	 * Otherwise, it's a command.
	 */
	if (!type_is_possibly_routine(sym->type)) {
		report(REPORT_ERROR, sc, "Command application of non-routine variable");
		/*return(NULL);*/
	}
	type_ensure_routine(sym->type);
	if (!type_is_void(type_representative(sym->type)->t.closure.range)) {
		report(REPORT_ERROR, sc, "Command application of function variable");
		/*return(NULL);*/
	}

	if (sym->builtin != NULL) {
		a = ast_new_builtin(sc, sym->builtin, l);
	} else {
		a = ast_new_apply(sc, a, l, 0);
	}

	return(a);
}

struct ast *
parse_expr_list(struct scan_st *sc, struct symbol_table *stab,
		struct symbol *excl, int *cc)
{
	struct ast *a, *b;

	a = parse_expr(sc, stab, 0, excl, cc);
	if (tokeq(sc, ",")) {
		scan(sc);
		b = parse_expr_list(sc, stab, excl, cc);
	} else {
		b = NULL;
	}
	return(ast_new_arg(a, b));
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
	   struct symbol *excl, int *cc)
{
	struct ast *l, *r;
	int done = 0, i = 0;
	char the_op[256];

	if (level > maxlevel) {
		l = parse_primitive(sc, stab, excl, cc);
		return(l);
	} else {
		l = parse_expr(sc, stab, level + 1, excl, cc);
		while (!done) {
			done = 1;
			for (i = 0; i < 6 && op[level][i][0] != '\0'; i++) {
				if (tokeq(sc, op[level][i])) {
					strlcpy(the_op, sc->token, 256);
					scan(sc);
					done = 0;
					r = parse_expr(sc, stab, level + 1, excl, cc);
					l = ast_new_call2(the_op, sc, stab, l, r);
					break;
				}
			}
		}
		return(l);
	}
}

struct ast *
parse_primitive(struct scan_st *sc, struct symbol_table *stab,
	        struct symbol *excl, int *cc)
{
	struct ast *a, *l, *r;
	struct value *v;
	struct symbol *sym;
	struct symbol_table *istab;

	if (tokeq(sc, "(")) {
		scan(sc);
		a = parse_expr(sc, stab, 0, excl, cc);
		scan_expect(sc, ")");
	} else if (tokeq(sc, "^")) {
		int my_cc = 0;
		int my_arity = 0;
		struct type *a_type = NULL;

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
			if (a_type == NULL)
				a_type = sym->type;
			else
				a_type = type_new_arg(sym->type, a_type);
			my_arity++;
			/*
			printf("ARG TYPE:");
			type_print(stdout, a_type);
			printf("\n");
			*/
			if (tokeq(sc, ","))
				scan(sc);
		}
		if (a_type == NULL)
			a_type = type_new(TYPE_VOID);
		a = parse_block(sc, stab, &istab, &my_cc);
		a = ast_new_routine(my_arity, symbol_table_size(istab) - my_arity, my_cc, a);
		if (type_is_set(a->datatype) && type_set_contains_void(a->datatype)) {
			report(REPORT_ERROR, sc, "Routine must be either function or command");
		}
		v = value_new_closure(a, NULL);
		a = ast_new_value(v,
		    type_new_closure(a_type, a->datatype));
		value_release(v);
	} else if (tokeq(sc, "!")) {
		scan(sc);
		a = parse_primitive(sc, stab, excl, cc);
		sym = symbol_lookup(stab, "!", 1);
		/* XXX builtin */
		a = ast_new_apply(sc, ast_new_local(stab, sym), a, 1);
	} else if (tokeq(sc, "[")) {
		scan(sc);
		v = value_new_list();
		a = ast_new_value(v, NULL);	/* XXX list */
		value_release(v);
		if (tokne(sc, "]")) {
			l = parse_expr_list(sc, stab, excl, cc);
			sym = symbol_lookup(stab, "List", VAR_GLOBAL);
			assert(sym->builtin != NULL);
			a = ast_new_builtin(sc, sym->builtin, l);
		}
		scan_expect(sc, "]");
	} else if (sc->type == TOKEN_BAREWORD && isupper(sc->token[0])) {
		a = parse_var(sc, stab, &sym, VAR_GLOBAL, VAR_MUST_EXIST, NULL);
		if (sym == excl) {
			report(REPORT_ERROR, sc, "Initializer cannot refer to variable being defined");
			return(NULL);
		}
		while (tokeq(sc, "(") || tokeq(sc, "[") || tokeq(sc, ".")) {
			if (tokeq(sc, "(")) {
				scan(sc);
				if (tokne(sc, ")")) {
					l = parse_expr_list(sc, stab, excl, cc);
				} else {
					l = NULL;
				}
				scan_expect(sc, ")");

				if (!type_is_possibly_routine(sym->type)) {
					report(REPORT_ERROR, sc, "Function application of non-routine variable");
					/*return(NULL);*/
				}
				type_ensure_routine(sym->type);
				if (type_is_void(type_representative(sym->type)->t.closure.range)) {
					report(REPORT_ERROR, sc, "Function application of command variable");
					/*return(NULL);*/
				}
				
				if (sym->builtin != NULL) {
					a = ast_new_builtin(sc, sym->builtin, l);
				} else {
					a = ast_new_apply(sc, a, l, sym->is_pure);
				}
			} else if (tokeq(sc, "[")) {
				scan(sc);
				r = parse_expr(sc, stab, 0, excl, cc);
				scan_expect(sc, "]");
				a = ast_new_call2("Fetch", sc, stab, a, r);
			} else if (tokeq(sc, ".")) {
				scan(sc);
				r = parse_literal(sc, stab);
				a = ast_new_call2("Fetch", sc, stab, a, r);
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
		a = ast_new_value(v, type_new(TYPE_ATOM));
		value_release(v);
		scan(sc);		
	} else if (sc->type == TOKEN_NUMBER) {
		v = value_new_integer(atoi(sc->token));
		a = ast_new_value(v, type_new(TYPE_INTEGER));
		value_release(v);
		scan(sc);
	} else if (sc->type == TOKEN_QSTRING) {
		v = value_new_string(sc->token);
		a = ast_new_value(v, type_new(TYPE_STRING));
		value_release(v);
		scan(sc);
	} else {
		report(REPORT_ERROR, sc, "Illegal literal");
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
			report(REPORT_ERROR, sc, "Undefined symbol");
		}
		*sym = symbol_define(stab, sc->token, SYM_KIND_VARIABLE, v);
		symbol_set_type(*sym, type_brand_new_var());
	} else {
		if (existence == VAR_MUST_NOT_EXIST) {
			report(REPORT_ERROR, sc, "Symbol already defined");
		}
	}
	scan(sc);

	if ((*sym)->value != NULL) {
		a = ast_new_value((*sym)->value, (*sym)->type);
	} else {
		a = ast_new_local(stab, (*sym));
	}
	return(a);
}
