/*
 * mem.c
 * $Id$
 * Bhuna memory management functions.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

void
bhuna_free(void *ptr)
{
	assert(ptr != NULL);
	free(ptr);
}
