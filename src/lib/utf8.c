/*
 * Copyright (c) 1999 G. Adam Stanislav
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	fget.utf-8.c v.1
 */

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <ctype.h>

#include "utf8.h"

struct u8putback {
	struct u8putback	*next;
	wchar_t			 c;
};

static struct u8putback *pb_head = NULL;

wchar_t
fgetu8(FILE *input) {
	wchar_t c;
	int ch, i, iterations;

	if (pb_head != NULL) {
		struct u8putback *pb;

		pb = pb_head;
		pb_head = pb->next;
		c = pb->c;
		free(pb);
		return(c);
	}

	if (input == NULL)
		return((wchar_t)EOF);

	if ((c = fgetc(input)) == EOF)
		return((wchar_t)EOF);

	if ((c & 0xFE) == 0xFC) {
		c &= 0x01;
		iterations = 5;
	} else if ((c & 0xFC) == 0xF8) {
		c &= 0x03;
		iterations = 4;
	} else if ((c & 0xF8) == 0xF0) {
		c &= 0x07;
		iterations = 3;
	} else if ((c & 0xF0) == 0xE0) {
		c &= 0x0F;
		iterations = 2;
	} else if ((c & 0xE0) == 0xC0) {
		c &= 0x1F;
		iterations = 1;
	} else if ((c & 0x80) == 0x80) {
		return((wchar_t)UTF8INVALID);
	} else {
		return(c);
	}

	for (i = 0; i < iterations; i++) {
		if ((ch = fgetc(input)) == EOF)
			return((wchar_t)EOF);

		if ((ch & 0xC0) != 0x80)
			return((wchar_t)UTF8INVALID);

		c <<= 6;
		c |= ch & 0x3F;
	}

	return(c);
}

wchar_t
ungetu8(wchar_t c, FILE *f)
{
	struct u8putback *pb;

	pb = malloc(sizeof(struct u8putback));
	pb->c = c;
	pb->next = pb_head;
	pb_head = pb;

	return(c);
}

void
fputsu8(FILE *f, wchar_t *s)
{
	wchar_t *wc;

	for (wc = s; *wc != L'\0'; wc++) {
		fprintf(f, "%c", *wc > 127 ? '?' : *(char *)wc);
	}
}

/*
 * For operating systems like FreeBSD 4.9 / DragonFly that don't yet
 * support wide character predicates.
 */

#ifndef HAS_WCHAR_PREDS

int
iswalpha(wchar_t w)
{
	if (w > 0xff) return(0);
	return(isalpha((char)w & 0xff));
}

int
iswspace(wchar_t w)
{
	if (w > 0xff) return(0);
	return(isspace((char)w & 0xff));
}

int
iswdigit(wchar_t w)
{
	if (w > 0xff) return(0);
	return(isdigit((char)w & 0xff));
}

#endif

int
wcstoi(wchar_t *w)
{
	int i, c = 0;

	for (i = 0; i < wcslen(w); i++) {
		if (!iswdigit(w[i]))
			return(0);
		c = c * 10 + (w[i] - L'0');
	}

	return(c);
}
