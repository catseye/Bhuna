/*
 * atom.h
 * $Id$
 */

#ifndef __ATOM_H_
#define __ATOM_H_

#include <wchar.h>

struct atom_entry {
	struct atom_entry *next;
	wchar_t *lexeme;
	int atom;
};

int atom_resolve(wchar_t *);

#endif /* !__ATOM_H_ */
