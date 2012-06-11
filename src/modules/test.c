#include <stdio.h>

#include <dlfcn.h>

#include "value.h"
#include "activation.h"
#include "builtin.h"

int
main(int argc, char **argv)
{
	void *lib_handle;
	const char *error_msg;
	struct activation *ar;
	struct value *v = NULL, *result = NULL;
	struct builtin *builtins;
	int i, j;

	if ((lib_handle = dlopen("./io.so", RTLD_LAZY)) == NULL) {
		fprintf(stderr, "Error during dlopen(): %s\n", dlerror());
		exit(1);
	}

	builtins = dlsym(lib_handle, "builtins");
	if ((error_msg = dlerror()) != NULL) {
		fprintf(stderr, "Error locating 'builtins' - %s\n", error_msg);
		exit(1);
	}

	for (i = 0; builtins[i].name != NULL; i++) {
		printf("Calling `%s'...\n", builtins[i].name);
		ar = activation_new_on_heap(builtins[i].arity, NULL, NULL);
		for (j = 0; j < builtins[i].arity; j++) {
			v = value_new_integer(76);
			activation_set_value(ar, j, 0, v);
		}
		(*builtins[i].fn)(ar, &result);
		/*activation_free_from_stack(ar);*/
		printf("Done!  Result: ");
		value_print(result);
		printf("\n");
	}

	dlclose(lib_handle);
	return(0);
}

