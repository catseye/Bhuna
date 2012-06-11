/*
 * mem.c
 * $Id$
 * Bhuna memory management functions.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void *
bhuna_malloc(size_t size, char *what)
{
	void *ptr;

	ptr = malloc(size);
	assert(ptr != NULL);
#ifdef BZERO
	bzero(ptr, size);
#endif

	return(ptr);
}

char *
bhuna_strdup(char *string)
{
	char *ptr;

	ptr = strdup(string);
	assert(ptr != NULL);

	return(ptr);
}

wchar_t *
bhuna_wcsdup(wchar_t *w)
{
	wchar_t *n;

	n = bhuna_malloc((wcslen(w) + 1) * sizeof(wchar_t), "wcsdup");
	wcscpy(n, w);

	return(n);
}

void
bhuna_free(void *ptr)
{
	assert(ptr != NULL);
	free(ptr);
}
