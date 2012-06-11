#include <string.h>
#include <stdlib.h>
#include <wchar.h>

#include "mem.h"
#include "atom.h"

static struct atom_entry *atom_entry_head = NULL;
static int next_atom = 0;

int
atom_resolve(wchar_t *lexeme)
{
	struct atom_entry *ae;

	/* find lexeme in atom table */
	for (ae = atom_entry_head; ae != NULL; ae = ae->next) {
		if (wcscmp(ae->lexeme, lexeme) == 0)
			return(ae->atom);
	}
	/* create new atom */
	ae = bhuna_malloc(sizeof(struct atom_entry));
	ae->next = atom_entry_head;
	ae->lexeme = bhuna_wcsdup(lexeme);
	ae->atom = next_atom++;
	atom_entry_head = ae;

	return(ae->atom);
}
