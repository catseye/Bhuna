#include <stdlib.h>
#include <stdio.h>

struct tree {
	struct tree *l;
	struct tree *r;
	char c;
};

struct tree *
new_tree(struct tree *l, struct tree *r, char c)
{
	struct tree *t;

	t = malloc(sizeof(struct tree));
	t->l = l;
	t->r = r;
	t->c = c;
	return(t);
}

void
dump_tree_r(struct tree *t)
{
	if (t == NULL) return;
	dump_tree_r(t->l);
	dump_tree_r(t->r);
	printf(" %c", t->c);
}

/*-------------------------------------------*/

#define MOVE 0
#define QUERY 1

struct stack {
	struct tree *t;
	int state;
} s[256];
int sp = 0;

void
push(struct tree *t, int state)
{
	s[sp].t = t;
	s[sp].state = state;
	sp++;
}	

void
pop(struct tree **t, int *state)
{
	--sp;
	*t = s[sp].t;
	*state = s[sp].state;
}	

void
dump_tree_i(struct tree *arg)
{
	struct tree *t;
	int state;

	push(arg, MOVE);
	
	while (sp != 0) {
		pop(&t, &state);
		if (state == MOVE) {
			push(t, QUERY);
			if (t->r != NULL) {
				push(t->r, MOVE);
			}
			if (t->l != NULL) {
				push(t->l, MOVE);
			}
		} else if (state == QUERY) {
			printf(" %c", t->c);
		}
	}
}

int main(int argc, char **argv)
{
	struct tree *t;

	t = new_tree(
	    new_tree(NULL, NULL, 'a'),
	    new_tree(
		new_tree(NULL, NULL, '2'),
		new_tree(NULL, NULL, '3'),
	    '+'),
	'=');

	dump_tree_r(t);
	printf("\n");
	dump_tree_i(t);
	printf("\n");
	return(0);
}
