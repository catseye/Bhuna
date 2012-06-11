#include <stdio.h>

#include "value.h"
#include "activation.h"
#include "builtin.h"
#include "type.h"

#include "io.h"

struct builtin builtins[] = {
	{"Hello",	hello,		btype_hello,		 1, 0, 1, -1},
	{NULL,		NULL,		NULL,			 0, 0, 0, -1}
};


struct value
hello(struct activation *ar)
{
	printf("Hello from IO!\n");
	printf("You gave me: ");
	value_print(activation_get_value(ar, 0, 0));
	printf("!!!\n");
	return value_new_integer(42);
}

struct type *
btype_hello(void)
{
	return(
	  type_new_closure(
	    type_new(TYPE_INTEGER),
	    type_new(TYPE_VOID)
	  )
	);
}
