/*
 * gc.h
 */

struct vm;

#define DEFAULT_GC_TRIGGER	8192

void			 gc(struct vm *);
