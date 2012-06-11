#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <dlfcn.h>

#include "builtin.h"
#include "value.h"
#include "list.h"
#include "dict.h"
#include "closure.h"
#include "activation.h"
#include "type.h"
#include "symbol.h"
#include "utf8.h"

#include "ast.h"
#include "vm.h"
#include "process.h"

/*
 * Built-in operations.
 */

struct builtin builtins[] = {
	{L"Print",	builtin_print,	btype_print,		-1, 0, 0, 1, 0},
	{L"!",		builtin_not,	btype_unary_logic,	 1, 1, 1, 1, 1},
	{L"&",		builtin_and,	btype_binary_logic,	 2, 1, 1, 1, 2},
	{L"|",		builtin_or,	btype_binary_logic,	 2, 1, 1, 1, 3},
	{L"=",		builtin_equ,	btype_equality,		 2, 1, 1, 1, 4},
	{L"!=",		builtin_neq,	btype_equality,		 2, 1, 1, 1, 5},
	{L">",		builtin_gt,	btype_compare,		 2, 1, 1, 1, 6},
	{L"<",		builtin_lt,	btype_compare,		 2, 1, 1, 1, 7},
	{L">=",		builtin_gte,	btype_compare,		 2, 1, 1, 1, 8},
	{L"<=",		builtin_lte,	btype_compare,		 2, 1, 1, 1, 9},
	{L"+",		builtin_add,	btype_arith,		 2, 1, 1, 1, 10},
	{L"-",		builtin_sub,	btype_arith,		 2, 1, 1, 1, 11},
	{L"*",		builtin_mul,	btype_arith,		 2, 1, 1, 1, 12},
	{L"/",		builtin_div,	btype_arith,		 2, 1, 1, 1, 13},
	{L"%",		builtin_mod,	btype_arith,		 2, 1, 1, 1, 14},
	{L"List",	builtin_list,	btype_list,		-1, 1, 1, 1, 15},
	{L"Fetch",	builtin_fetch,	btype_fetch,		 2, 1, 1, 1, 16},
	{L"Store",	builtin_store,	btype_store,		 3, 0, 0, 1, 17},
	{L"Dict",	builtin_dict,	btype_dict,		-1, 1, 1, 1, 18},
	{L"Spawn",	builtin_spawn,	btype_spawn,		 1, 1, 0, 1, 19},
	{L"Send",	builtin_send,	btype_send,		 2, 0, 0, 1, 20},
	{L"Recv",	builtin_recv,	btype_recv,		 1, 1, 0, 1, 21},
	{L"Self",	builtin_self,	btype_self,		 0, 1, 0, 1, 22},
	{NULL,		NULL,		NULL,			 0, 0, 0, 0, 0}
};

struct value
builtin_print(struct activation *ar)
{
	int i;
	struct value v;

	for (i = 0; i < ar->size; i++) {
		v = activation_get_value(ar, i, 0);

		switch (v.type) {
		case VALUE_INTEGER:
			printf("%d", v.v.i);
			break;
		case VALUE_BOOLEAN:
			printf("%s", v.v.b ? "true" : "false");
			break;
		case VALUE_STRING:
			fputsu8(stdout, v.v.s->v.s);
			break;
		case VALUE_LIST:
			/*
			printf("[");
			for (l = v->v.l; l != NULL; l = l->next) {
			*/
				
			list_dump(v.v.s->v.l);
			break;
		case VALUE_ERROR:
			printf("#ERR<%s>", v.v.s->v.e);
			break;
		case VALUE_BUILTIN:
			printf("#BIF<%08lx>", (unsigned long)v.v.bi);
			break;
		case VALUE_CLOSURE:
			closure_dump(v.v.s->v.k);
			break;
		case VALUE_DICT:
			dict_dump(v.v.s->v.d);
			break;
		case VALUE_OPAQUE:
			printf("#OPAQUE<%08lx>", (unsigned long)v.v.ptr);
			break;
		default:
			printf("???unknown(%d)???", v.type);
			break;
		}
	}
	return(value_null());
}

/*** logical ***/

struct value
builtin_not(struct activation *ar)
{
	struct value q = activation_get_value(ar, 0, 0);

	if (q.type == VALUE_BOOLEAN) {
		return(value_new_boolean(!q.v.b));
	} else {
		return(value_new_error("type mismatch"));
	}
}

struct value
builtin_and(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_BOOLEAN && r.type == VALUE_BOOLEAN) {
		return(value_new_boolean(l.v.b && r.v.b));
	} else {
		return(value_new_error("type mismatch"));
	}
}

struct value
builtin_or(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_BOOLEAN && r.type == VALUE_BOOLEAN) {
		return(value_new_boolean(l.v.b || r.v.b));
	} else {
		return(value_new_error("type mismatch"));
	}
}

/*** comparison ***/

struct value
builtin_equ(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		return value_new_boolean(l.v.i == r.v.i);
	} else if (l.type == VALUE_OPAQUE && r.type == VALUE_OPAQUE) {
		return value_new_boolean(l.v.ptr == r.v.ptr);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_neq(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		return value_new_boolean(l.v.i != r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_gt(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		return value_new_boolean(l.v.i > r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_lt(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		return value_new_boolean(l.v.i < r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_gte(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		return value_new_boolean(l.v.i >= r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_lte(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		return value_new_boolean(l.v.i <= r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

/*** arithmetic ***/

struct value
builtin_add(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		return value_new_integer(l.v.i + r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_mul(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

#if 0
	printf("IN MUL, L = ");
	value_print(l);
	printf("  R = ");
	value_print(r);
	printf("\n");
#endif

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		return value_new_integer(l.v.i * r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_sub(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		return value_new_integer(l.v.i - r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_div(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		if (r.v.i == 0)
			return value_new_error("division by zero");
		else
			return value_new_integer(l.v.i / r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_mod(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);

	if (l.type == VALUE_INTEGER && r.type == VALUE_INTEGER) {
		if (r.v.i == 0)
			return value_new_error("modulo by zero");
		else
			return value_new_integer(l.v.i % r.v.i);
	} else {
		return value_new_error("type mismatch");
	}
}

/*** list ***/

struct value
builtin_list(struct activation *ar)
{
	int i;
	struct value v, x;

	v = value_new_list();

	for (i = ar->size - 1; i >= 0; i--) {
		x = activation_get_value(ar, i, 0);
		value_list_append(v, x);
	}

	return(v);
}

struct value
builtin_fetch(struct activation *ar)
{
	struct value l = activation_get_value(ar, 0, 0);
	struct value r = activation_get_value(ar, 1, 0);
	int count;
	struct list *li;

	if (l.type == VALUE_CLOSURE && r.type == VALUE_INTEGER) {
		int i = r.v.i - 1;
		/*
		 * This is _EVIL_!
		 */
		if (i >= 0 && i < l.v.s->v.k->ar->size) {
			return(activation_get_value(l.v.s->v.k->ar, i, 0));
		} else {
			return(value_new_error("out of bounds"));
		}
	} else if (l.type == VALUE_DICT) {
		return(dict_fetch(l.v.s->v.d, r));
	} else if (l.type == VALUE_LIST && r.type == VALUE_INTEGER) {
		li = l.v.s->v.l;
		for (count = 1; li != NULL && count < r.v.i; count++)
			li = li->next;
		if (li == NULL)
			return value_new_error("out of bounds");
		else {
			return li->value;
		}
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_store(struct activation *ar)
{
	struct value d = activation_get_value(ar, 0, 0);
	struct value i = activation_get_value(ar, 1, 0);
	struct value p = activation_get_value(ar, 2, 0);
	int count;
	struct list *li;

	if (d.type == VALUE_DICT) {
		dict_store(d.v.s->v.d, i, p);
		return(d);
	} else if (d.type == VALUE_LIST && i.type == VALUE_INTEGER) {
		li = d.v.s->v.l;
		for (count = 1; li != NULL && count < i.v.i; count++)
			li = li->next;
		if (li == NULL)
			return(value_new_error("no such element"));
		else {
			li->value = p;
			return(d);
		}
	} else {
		return(value_new_error("type mismatch"));
	}
}

struct value
builtin_dict(struct activation *ar)
{
	int i;
	struct value key, val, v;

	v = value_new_dict();

	if (ar->size % 2 != 0) {
		return(value_new_error("number of arguments must be even"));
	} else {
		for (i = 0; i < ar->size; i += 2) {
			key = activation_get_value(ar, i, 0);
			val = activation_get_value(ar, i + 1, 0);
			value_dict_store(v, key, val);
		}
	}
	
	return(v);
}

/*** multiprocessing ***/

struct value
builtin_spawn(struct activation *ar)
{
	struct value q = activation_get_value(ar, 0, 0);
	struct process *p;

	if (q.type == VALUE_CLOSURE) {
		p = process_spawn(q.v.s->v.k);
		return value_new_opaque(p);
	} else {
		return value_new_error("type mismatch");
	}
}

struct value
builtin_send(struct activation *ar)
{
	struct value pv = activation_get_value(ar, 0, 0);
	struct value mv = activation_get_value(ar, 1, 0);
	struct process *p;

	if (pv.type == VALUE_OPAQUE) {
		p = (struct process *)pv.v.ptr;
		process_send(p, mv);
		return value_null();
	} else {
		return value_new_error("type mismatch");
	}
}

/*
 * This can't really be done here - it should be done in the vm.
 */
struct value
builtin_recv(struct activation *ar)
{
	struct value tv = activation_get_value(ar, 0, 0);
	struct value rv = value_null();

	if (tv.type == VALUE_INTEGER) {
		process_recv(&rv);
		return(rv);
	} else {
		return value_new_error("type mismatch");
	}
}

/*
 * This can't really be done here - it should be done in the vm.
 */
struct value
builtin_self(struct activation *ar)
{
	return(value_new_opaque(current_process));
}

/*** TYPES ***/

struct type *
btype_print(void)
{
	return(
	  type_new_closure(
	    type_new_var(1),
	    type_new(TYPE_VOID)
	  )
	);
}

struct type *
btype_unary_logic(void)
{
	return(
	  type_new_closure(
	    type_new(TYPE_BOOLEAN),
	    type_new(TYPE_BOOLEAN)
	  )
	);
}

struct type *
btype_binary_logic(void)
{
	return(
	  type_new_closure(
	    type_new_arg(type_new(TYPE_BOOLEAN), type_new(TYPE_BOOLEAN)),
	    type_new(TYPE_BOOLEAN)
	  )
	);
}

struct type *
btype_equality(void)
{
	return(
	  type_new_closure(
	    type_new_arg(type_new_var(9), type_new_var(9)),
	    type_new(TYPE_BOOLEAN)
	  )
	);
}

struct type *
btype_compare(void)
{
	return(
	  type_new_closure(
	    type_new_arg(type_new(TYPE_INTEGER), type_new(TYPE_INTEGER)),
	    type_new(TYPE_BOOLEAN)
	  )
	);
}

struct type *
btype_arith(void)
{
	return(
	  type_new_closure(
	    type_new_arg(type_new(TYPE_INTEGER), type_new(TYPE_INTEGER)),
	    type_new(TYPE_INTEGER)
	  )
	);
}

struct type *
btype_list(void)
{
	return(
	  type_new_closure(
	    type_new_var(2),
	    type_new_list(type_new(TYPE_INTEGER))
	  )
	);
}

struct type *
btype_fetch(void)
{
	return(
	  type_new_closure(
	    type_new_arg(type_new_var(5), type_new(TYPE_INTEGER)),
	    type_new_var(5)
	  )
	);
}

struct type *
btype_store(void)
{
	return(
	  type_new_closure(
	    type_new_arg(
		type_new_var(6),
		type_new_arg(type_new_var(7), type_new_var(8))
	    ),
	    type_new(TYPE_VOID)
	  )
	);
}

struct type *
btype_dict(void)
{
	return(NULL);
}

struct type *
btype_spawn(void)
{
	return(
	  type_new_closure(
	    type_new_closure(
		type_new(TYPE_VOID),
		type_new(TYPE_VOID)
	    ),
	    type_new_opaque("pid")
	  )
	);
}

struct type *
btype_send(void)
{
	return(
	  type_new_closure(
	    type_new_arg(
		type_new_opaque("pid"),
		type_new_var(10)
	    ),
	    type_new(TYPE_VOID)
	  )
	);
}

struct type *
btype_recv(void)
{
	return(
	  type_new_closure(
	    type_new(TYPE_INTEGER),
	    type_new_var(11)
	  )
	);
}

struct type *
btype_self(void)
{
	return(
	  type_new_closure(
	    type_new(TYPE_VOID),
	    type_new_opaque("pid")
	  )
	);
}

/*** REGISTRATION ***/

struct symbol *
register_builtin(struct symbol_table *stab, struct builtin *b)
{
	struct value v;
	struct symbol *sym;

	v = value_new_builtin(b);
	value_deregister(v);	/* don't garbage-collect it */
	sym = symbol_define(stab, b->name, SYM_KIND_COMMAND, &v);
	sym->is_pure = b->is_pure;
	sym->builtin = b;
	sym->type = b->ty();

	return(sym);
}

void
register_std_builtins(struct symbol_table *stab)
{
	int i;
	struct value v;
	struct symbol *sym;

	for (i = 0; builtins[i].name != NULL; i++)
		register_builtin(stab, &builtins[i]);

	/* XXX */
	/* Register global constants - this should probably be elsewhere. */
	/* And/or we should have "constant builtins" that have a va()
	    function that returns the constant value, hmm.... */

	v = value_new_string(L"\n");
	value_deregister(v);
	sym = symbol_define(stab, L"EoL", SYM_KIND_VARIABLE, &v);
	sym->type = type_new(TYPE_STRING);

	v = value_new_boolean(1);
	value_deregister(v);
	sym = symbol_define(stab, L"True", SYM_KIND_VARIABLE, &v);
	sym->type = type_new(TYPE_BOOLEAN);

	v = value_new_boolean(0);
	value_deregister(v);
	sym = symbol_define(stab, L"False", SYM_KIND_VARIABLE, &v);
	sym->type = type_new(TYPE_BOOLEAN);
}

void
load_builtins(struct symbol_table *stab, char *modname)
{
	void *lib_handle;
	const char *error_msg;
	struct builtin *ext_builtins;
	int i;
	char *filename;

	asprintf(&filename, "%s.so", modname);
	if ((lib_handle = dlopen(filename, RTLD_LAZY)) == NULL) {
		fprintf(stderr, "Error during dlopen(): %s\n", dlerror());
		exit(1);
	}
	free(filename);

	ext_builtins = dlsym(lib_handle, "builtins");
	if ((error_msg = dlerror()) != NULL) {
		fprintf(stderr, "Error locating 'builtins' - %s\n", error_msg);
		exit(1);
	}

	for (i = 0; ext_builtins[i].name != NULL; i++) {
		/* printf("Registering ext builtin `%s'...\n", ext_builtins[i].name); */
		register_builtin(stab, &ext_builtins[i]);
	}

	/* dlclose(lib_handle); */
}

/*
		struct activation *ar;
		struct value *v = NULL, *result = NULL;

		ar = activation_new_on_stack(builtins[i].arity, current_ar, NULL);
		for (j = 0; j < builtins[i].arity; j++) {
			v = value_new_integer(76);
			activation_set_value(ar, j, 0, v);
		}
		(*builtins[i].fn)(ar, &result);
		activation_free_from_stack(ar);
		printf("Done!  Result: ");
		value_print(result);
		printf("\n");
*/
