#ifndef __BUILTIN_H_
#define __BUILTIN_H_

struct value;

struct builtin_desc {
	char *name;
	void (*fn)(struct value **);
};

extern struct builtin_desc builtins[];

void	builtin_print(struct value **);

void	builtin_return(struct value **);

void	builtin_not(struct value **);
void	builtin_and(struct value **);
void	builtin_or(struct value **);

void	builtin_equ(struct value **);
void	builtin_neq(struct value **);
void	builtin_gt(struct value **);
void	builtin_lt(struct value **);
void	builtin_gte(struct value **);
void	builtin_lte(struct value **);

void	builtin_add(struct value **);
void	builtin_mul(struct value **);
void	builtin_sub(struct value **);
void	builtin_div(struct value **);
void	builtin_mod(struct value **);

void	builtin_index(struct value **);

#endif
