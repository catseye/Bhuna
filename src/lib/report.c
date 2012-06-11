/*
 * report.c
 * Translation error/warning reporter for Bhuna.
 * $Id: scan.c 54 2004-04-23 22:51:09Z catseye $
 */

#include <stdarg.h>
#include <stdio.h>

#include "mem.h"
#include "scan.h"
#include "report.h"

#include "type.h"
#include "symbol.h"

static int errors;
static int warnings;
static FILE *rfile;

void
report_start(void)
{
	rfile = stderr;
	errors = 0;
	warnings = 0;
}

int
report_finish(void)
{
	/* if verbose */
	fprintf(rfile, "Translation finished with %d errors and %d warnings\n",
	    errors, warnings);
	return(errors);
}

void
report(int rtype, struct scan_st *sc, char *fmt, ...)
{
	va_list args;
	int i;

	if (sc != NULL) {
		fprintf(rfile, "%s (line %d, column %d, token '%s'): ",
		    rtype == REPORT_ERROR ? "Error" : "Warning",
		    sc->lino, sc->columno, sc->token);
	} else {
		fprintf(rfile, "%s (line ?, column ?, token ?): ",
		    rtype == REPORT_ERROR ? "Error" : "Warning");
	}

	va_start(args, fmt);
	for (i = 0; fmt[i] != '\0'; i++) {
		if (fmt[i] == '%') {
			i++;
			switch (fmt[i]) {
			case 't':
				type_print(rfile, va_arg(args, struct type *));
				break;
			case 'S':
				symbol_print(rfile, va_arg(args, struct symbol *));
				break;
			case 's':
				fprintf(stderr, "%s", va_arg(args, char *));
				break;
			case 'd':
				fprintf(stderr, "%d", va_arg(args, int));
				break;
			}
		} else {
			fprintf(rfile, "%c", fmt[i]);
		}
	}
	va_end(args);

	fprintf(rfile, ".\n");
	
	if (rtype == REPORT_ERROR) {
		errors++;
	} else {
		warnings++;
	}
}
