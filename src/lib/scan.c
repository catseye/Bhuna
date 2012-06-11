/*
 * scan.c
 * Lexical scanner for Bhuna.
 * $Id: scan.c 54 2004-04-23 22:51:09Z catseye $
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "mem.h"
#include "scan.h"
#include "report.h"
#include "utf8.h"

struct scan_st *
scan_open(char *filename)
{
	struct scan_st *sc;

	sc = bhuna_malloc(sizeof(struct scan_st));
	sc->token = (wchar_t *)bhuna_malloc(256 * sizeof(wchar_t));

	if ((sc->in = fopen(filename, "r")) == NULL) {
		bhuna_free(sc->token);
		bhuna_free(sc);
		return(NULL);
	}

	sc->lino = 1;
	sc->columno = 1;
	sc->lastcol = 0;
	scan(sc);		/* prime the pump */

	return(sc);
}

/*
 * This is just to ease error reporting, so we don't copy the file or nothin'.
 */
struct scan_st *
scan_dup(struct scan_st *orig)
{
	struct scan_st *sc;

	sc = bhuna_malloc(sizeof(struct scan_st));
	sc->token = bhuna_wcsdup(orig->token);

	sc->in = NULL;
	sc->lino = orig->lino;
	sc->columno = orig->columno;
	sc->lastcol = orig->lastcol;

	return(sc);
}

void
scan_close(struct scan_st *sc)
{
	if (sc->in != NULL)
		fclose(sc->in);
	bhuna_free(sc->token);
	bhuna_free(sc);
}

/*
 * x is not a string, it is a pointer to a single character.
 */
static void
scan_char(struct scan_st *sc, wchar_t *x)
{
	sc->lastcol = sc->columno;
	*x = fgetu8(sc->in);
	if (*x == L'\n') {
		sc->columno = 1;
		sc->lino++;
	} else if (*x == L'\t') {
		sc->columno++;
		while (sc->columno % 8 != 0)
			sc->columno++;
	} else {
		sc->columno++;
	}
}

static void
scan_putback(struct scan_st *sc, wchar_t x)
{
	if (feof(sc->in))
		return;
	ungetu8(x, sc->in);
	sc->columno = sc->lastcol;
	if (x == L'\n')
		sc->lino--;
}

static void
real_scan(struct scan_st *sc)
{
	wchar_t x;
	int i = 0;

	sc->token[0] = L'\0';
	if (feof(sc->in)) {
		sc->type = TOKEN_EOF;
		return;
	}

	scan_char(sc, &x);

	/* Skip whitespace. */

top:
	while (iswspace(x) && !feof(sc->in)) {
		scan_char(sc, &x);
	}

	/* Skip comments. */

	if (x == L'/') {
		scan_char(sc, &x);
		if (x == L'/') {
			while (x != L'\n' && !feof(sc->in)) {
				scan_char(sc, &x);
			}
			goto top;
		} else {
			scan_putback(sc, x);
			x = L'/';
			/* falls through to the bottom of scan() */
		}
	}

	if (feof(sc->in)) {
		sc->token[0] = L'\0';
		sc->type = TOKEN_EOF;
		return;
	}

	/*
	 * Scan decimal numbers.  Must start with a
	 * digit (not a sign or decimal point.)
	 */
	if (iswdigit(x)) {
		while ((iswdigit(x) || x == L'.') && !feof(sc->in)) {
			sc->token[i++] = x;
			scan_char(sc, &x);
		}
		scan_putback(sc, x);
		sc->token[i] = L'\0';
		sc->type = TOKEN_NUMBER;
		return;
	}

	/*
	 * Scan quoted strings.
	 */
	if (x == L'"') {
		scan_char(sc, &x);
		while (x != L'"' && !feof(sc->in) && i < 255) {
			sc->token[i++] = x;
			scan_char(sc, &x);
		}
		sc->token[i] = L'\0';
		sc->type = TOKEN_QSTRING;
		return;
	}

	/*
	 * Scan alphanumeric ("bareword") tokens.
	 */
	if (iswalpha(x) || x == L'_') {
		while ((iswalpha(x) || iswdigit(x) || x == L'_') && !feof(sc->in)) {
			sc->token[i++] = x;
			scan_char(sc, &x);
		}
		scan_putback(sc, x);
		sc->token[i] = L'\0';
		sc->type = TOKEN_BAREWORD;
		return;
	}

	/*
	 * Scan multi-character symbols.
	 */
	if (x == L'>' || x == L'<' || x == L'=' || x == L'!') {
		while ((x == L'>' || x == L'<' || x == L'=' || x == L'!') &&
		    !feof(sc->in) && i < 255) {
			sc->token[i++] = x;
			scan_char(sc, &x);
		}
		scan_putback(sc, x);
		sc->token[i] = L'\0';
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
scan(struct scan_st *sc)
{
	real_scan(sc);
	/*
	printf("scanned -> ");
	fputsu8(stdout, sc->token);
	printf("\n");
	*/
}

void
scan_expect(struct scan_st *sc, wchar_t *x)
{
	if (wcscmp(sc->token, x) == 0) {
		scan(sc);
	} else {
		report(REPORT_ERROR, sc, "Expected '%w'", x);
	}
}
