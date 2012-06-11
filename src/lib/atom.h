/*
 * atom.h
 * $Id$
 */

#ifndef __ATOM_H_
#define __ATOM_H_

struct atom_entry {
	struct atom_entry *next;
	char *lexeme;
	int atom;
};

int atom_resolve(char *);

#endif /* !__ATOM_H_ */
