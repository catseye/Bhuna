/*
 * mem.h
 * $Id$
 * Bhuna memory management functions and macros.
 */

#ifndef __MEM_H_
#define __MEM_H_

#include <sys/types.h>

#ifdef DEBUG
void		*bhuna_malloc(size_t);
char		*bhuna_strdup(char *);
void		 bhuna_free(void *);
#else
#include <stdlib.h>
#define	bhuna_malloc(x) malloc(x)
#define	bhuna_strdup(x) strdup(x)
#define	bhuna_free(x) free(x)
#endif

#endif
