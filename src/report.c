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
 * report.c
 * Translation error/warning reporter for Bhuna.
 * $Id: scan.c 54 2004-04-23 22:51:09Z catseye $
 */

#include <stdarg.h>
#include <stdio.h>
/*
#include <stdlib.h>
#include <string.h>
*/

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
