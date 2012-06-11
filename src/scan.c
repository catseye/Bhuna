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
 * scan.c
 * Lexical scanner for Bhuna.
 * $Id: scan.c 54 2004-04-23 22:51:09Z catseye $
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "scan.h"

struct scan_st *
scan_open(char *filename)
{
	struct scan_st *sc;

	if ((sc = bhuna_malloc(sizeof(struct scan_st))) == 0) {
		return(NULL);
	}
	if ((sc->token = (char *)bhuna_malloc(256 * sizeof(char))) == NULL) {
		bhuna_free(sc);
		return(NULL);
	}
	if ((sc->in = fopen(filename, "r")) == NULL) {
		bhuna_free(sc->token);
		bhuna_free(sc);
		return(NULL);
	}

	sc->lino = 1;
	sc->columno = 1;
	sc->errors = 0;
	scan(sc);		/* prime the pump */

	return(sc);
}

void
scan_close(struct scan_st *sc)
{	
	fclose(sc->in);
	bhuna_free(sc->token);
	bhuna_free(sc);
}

void
scan_error(struct scan_st *sc, char *fmt, ...)
{
	va_list args;
	char err[256];

	va_start(args, fmt);
	vsnprintf(err, 255, fmt, args);

	printf("Error (line %d, column %d, token '%s'): %s.\n",
	    sc->lino, sc->columno, sc->token, err);

	sc->errors++;
}

void
scan_char(struct scan_st *sc, char *x)
{	
	*x = (char)getc(sc->in);
	if (*x == '\n') {
		sc->columno = 1;
		sc->lino++;
	} else {
		sc->columno++;
	}
}

void
scan_putback(struct scan_st *sc, char x)
{
	if (feof(sc->in)) return;
	ungetc(x, sc->in);
	if (x == '\n') {
		sc->columno = 80;	/* XXX heh */
		sc->lino--;
	} else {
		sc->columno--;
	}
}

void
scan(struct scan_st *sc)
{
	char x;
	int i = 0;

	sc->token[0] = '\0';
	if (feof(sc->in)) {
		sc->type = TOKEN_EOF;
		return;
	}

	scan_char(sc, &x);

	/* Skip whitespace. */

top:
	while (isspace(x) && !feof(sc->in)) {
		scan_char(sc, &x);
	}

	/* Skip comments. */

	if (x == '/') {
		scan_char(sc, &x);
		if (x == '/') {
			while (x != '\n' && !feof(sc->in)) {
				scan_char(sc, &x);
			}
			goto top;
		} else {
			scan_putback(sc, x);
			x = '/';
			/* falls through to the bottom of scan() */
		}
	}

	if (feof(sc->in)) {
		sc->token[0] = '\0';
		sc->type = TOKEN_EOF;
		return;
	}

	/*
	 * Scan decimal numbers.  Must start with a
	 * digit (not a sign or decimal point.)
	 */
	if (isdigit(x)) {
		while ((isdigit(x) || x == '.') && !feof(sc->in)) {
			sc->token[i++] = x;
			scan_char(sc, &x);
		}
		scan_putback(sc, x);
		sc->token[i] = 0;
		sc->type = TOKEN_NUMBER;
		return;
	}

	/*
	 * Scan quoted strings.
	 */
	if (x == '"') {
		scan_char(sc, &x);
		while (x != '"' && !feof(sc->in) && i < 255) {
			sc->token[i++] = x;
			scan_char(sc, &x);
		}
		sc->token[i] = 0;
		sc->type = TOKEN_QSTRING;
		return;
	}

	/*
	 * Scan alphanumeric ("bareword") tokens.
	 */
	if (isalpha(x) || x == '_') {
		while ((isalpha(x) || isdigit(x) || x == '_') && !feof(sc->in)) {
			sc->token[i++] = x;
			scan_char(sc, &x);
		}
		scan_putback(sc, x);
		sc->token[i] = 0;
		sc->type = TOKEN_BAREWORD;
		return;
	}

	/*
	 * Scan multi-character symbols.
	 */
	if (x == '>' || x == '<' || x == '=' || x == '!') {
		while ((x == '>' || x == '<' || x == '=' || x == '!') &&
		    !feof(sc->in) && i < 255) {
			sc->token[i++] = x;
			scan_char(sc, &x);
		}
		sc->token[i] = '\0';
		sc->type = TOKEN_SYMBOL;
		return;
	}

	/*
	 * Degenerate case: scan single symbols.
	 */
	sc->token[0] = x;
	sc->token[1] = 0;
	sc->type = TOKEN_SYMBOL;
}

void
scan_expect(struct scan_st *sc, char *x)
{
	if (!strcmp(sc->token, x)) {
		scan(sc);
	} else {
		scan_error(sc, "Expected '%s'", x);
	}
}
