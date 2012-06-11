/*
 * buffer.c
 * $Id$
 * Routines to manipulate extensible buffers.
 */

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "buffer.h"

/*
 * Create a new extensible buffer with the given initial size.
 */
struct buffer *
buffer_new(size_t size)
{
	struct buffer *e;

	e = malloc(sizeof(struct buffer));

	e->len = 0;
	e->size = size;
	e->pos = 0;

	e->buf = malloc(size);
	e->buf[0] = '\0';

	return(e);
}

/*
 * Deallocate the memory used for an extensible buffer.
 */
void
buffer_free(struct buffer *e)
{
	if (e != NULL) {
		if (e->buf != NULL)
			free(e->buf);
		free(e);
	}
}

/*
 * Return the underlying (static) buffer of an extensible buffer.
 *
 * NOTE that you should NEVER cache the returned pointer anywhere,
 * as any further manipulation of the extensible buffer may cause
 * it to be invalidated.
 *
 * ALSO NOTE that the buffer may contain embedded NULs, but will
 * also be guaranteed to be NUL-terminated.
 */
char *
buffer_buf(struct buffer *e)
{
	return(e->buf);
}

/*
 * Return the current length of the extensible buffer.
 */
size_t
buffer_len(struct buffer *e)
{
	return(e->len);
}

/*
 * Return the current size of the extensible buffer.  This is how
 * big it's length may grow to before expanded.
 */
size_t
buffer_size(struct buffer *e)
{
	return(e->size);
}

/*
 * Ensure that an extensible buffer's size is at least the given
 * size.  If it is not, it will be internally grown to that size.
 * This does not affect the contents of the buffer in any way.
 */
void
buffer_ensure_size(struct buffer *e, size_t size)
{
	if (e->size >= size) return;
	e->size = size;
	if ((e->buf = realloc(e->buf, e->size)) == NULL) {
		err(EX_UNAVAILABLE, "realloc()");
	}
}

/*
 * Set the contents of an extensible buffer from a regular (char *)
 * buffer.  The extensible buffer will grow if needed.  Any existing
 * contents of the extensible buffer are destroyed in this operation.
 * Note that, because this requires that the length of the
 * regular buffer be specified, it may safely contain NUL bytes.
 */
void
buffer_set(struct buffer *e, char *buf, size_t length)
{
	while ((length + 1) > e->size) {
		e->size *= 2;
	}
	if ((e->buf = realloc(e->buf, e->size)) == NULL) {
		err(EX_UNAVAILABLE, "realloc()");
	}
	memcpy(e->buf, buf, length);
	e->len = length;
	e->buf[e->len] = '\0';
}

/*
 * Append the contents of a regular buffer to the end of the existing
 * contents of an extensible buffer.  The extensible buffer will grow
 * if needed.  Note that, because this requires that the length of the
 * regular buffer be specified, it may safely contain NUL bytes.
 */
void
buffer_append(struct buffer *e, char *buf, size_t length)
{
	while (e->len + (length + 1) > e->size) {
		e->size *= 2;
	}
	if ((e->buf = realloc(e->buf, e->size)) == NULL) {
		err(EX_UNAVAILABLE, "realloc()");
	}
	memcpy(e->buf + e->len, buf, length);
	e->len += length;
	e->buf[e->len] = '\0';
}

/*
 * Set the contents of an extensible buffer from an ASCIIZ string.
 * This is identical to buffer_set except that the length need not
 * be specified, and the ASCIIZ string may not contain embedded NUL's.
 */
void
buffer_cpy(struct buffer *e, char *s)
{
	buffer_set(e, s, strlen(s));
}

/*
 * Append the contents of an ASCIIZ string to an extensible buffer.
 * This is identical to buffer_append except that the length need not
 * be specified, and the ASCIIZ string may not contain embedded NUL's.
 */
void
buffer_cat(struct buffer *e, char *s)
{
	buffer_append(e, s, strlen(s));
}

/*
 * Append the entire contents of a text file to an extensible buffer.
 */
int
buffer_cat_file(struct buffer *e, char *fmt, ...)
{
	va_list args;
	char *filename, line[1024];
	FILE *f;

	va_start(args, fmt);
	vasprintf(&filename, fmt, args);
	va_end(args);

	if ((f = fopen(filename, "r")) == NULL)
		return(0);

	free(filename);

	while (fgets(line, 1023, f) != NULL) {
		buffer_cat(e, line);
	}

	fclose(f);

	return(1);
}

/*
 * Append the entire output of a shell command to an extensible buffer.
 */
int
buffer_cat_pipe(struct buffer *e, char *fmt, ...)
{
	va_list args;
	char *command, line[1024];
	FILE *p;

	va_start(args, fmt);
	vasprintf(&command, fmt, args);
	va_end(args);

	if ((p = popen(command, "r")) == NULL)
		return(0);

	free(command);

	while (fgets(line, 1023, p) != NULL) {
		buffer_cat(e, line);
	}

	pclose(p);

	return(1);
}

/*** CURSORED FUNCTIONS ***/

/*
 * Note that the cursor can be anywhere from the first character to
 * one position _beyond_ the last character in the buffer.
 */

int
buffer_seek(struct buffer *e, size_t pos)
{
	if (pos <= e->size) {
		e->pos = pos;
		return(1);
	} else {
		return(0);
	}
}

size_t
buffer_tell(struct buffer *e)
{
	return(e->pos);
}

int
buffer_eof(struct buffer *e)
{
	return(e->pos >= e->size);
}

char
buffer_peek_char(struct buffer *e)
{
	return(e->buf[e->pos]);
}

char
buffer_scan_char(struct buffer *e)
{
	return(e->buf[e->pos++]);
}

int
buffer_compare(struct buffer *e, char *s)
{
	int i, pos;

	for (i = 0, pos = e->pos; s[i] != '\0' && pos < e->size; i++, pos++) {
		if (e->buf[pos] != s[i])
			return(0);
	}

	if (pos <= e->size) {
		return(pos);
	} else {
		return(0);
	}
}

int
buffer_expect(struct buffer *e, char *s)
{
	int pos;

	if ((pos = buffer_compare(e, s)) > 0) {
		e->pos = pos;
		return(1);
	} else {
		return(0);
	}
}

void
buffer_push(struct buffer *e, void *src, size_t len)
{
	buffer_ensure_size(e, e->pos + len);
	memcpy(e->buf + e->pos, src, len);
	e->pos += len;
}

int
buffer_pop(struct buffer *e, void *dest, size_t len)
{
	if (e->pos - len > 0) {
		e->pos -= len;
		memcpy(dest, e->buf + e->pos, len);
		return(1);
	} else {
		return(0);
	}
}
