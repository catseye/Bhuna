#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "scan.h"
#include "parse.h"
#include "symbol.h"
#include "ast.h"
#include "builtin.h"
#include "value.h"
#include "activation.h"

#ifdef DEBUG
int trace_ast = 0;
int trace_activations = 0;
int trace_calls = 0;
int trace_assignments = 0;
int trace_refcounting = 0;
int trace_closures = 0;

int num_vars_created = 0;
int num_vars_grabbed = 0;
int num_vars_released = 0;
int num_vars_freed = 0;
int num_vars_cowed = 0;
int debug_frame = 0;
#endif

struct activation *global_ar;
struct activation *current_ar;

void
usage(char **argv)
{
	fprintf(stderr, "Usage: %s "
#ifdef DEBUG
	    "[-acfknprstvz] "
#endif
	    "source\n",
	    argv[0]);
#ifdef DEBUG
	fprintf(stderr, "  -a: trace assignments\n");
	fprintf(stderr, "  -c: trace calls\n");
	fprintf(stderr, "  -f: trace frames\n");
	fprintf(stderr, "  -k: trace closures\n");
	fprintf(stderr, "  -n: don't actually run program\n");
	fprintf(stderr, "  -p: dump program AST before run\n");
	fprintf(stderr, "  -r: show reference counting stats (-rr = trace refcounting)\n");
	fprintf(stderr, "  -s: dump symbol table before run\n");
	fprintf(stderr, "  -t: trace AST transitions in evaluator\n");
	fprintf(stderr, "  -v: trace activation records\n");
	fprintf(stderr, "  -z: dump symbol table after run\n");
#endif
	exit(1);
}

void
load_builtins(struct symbol_table *stab, struct builtin_desc *b)
{
	int i;
	struct value *v;
	struct symbol *sym;

	for (i = 0; b[i].name != NULL; i++) {
		sym = symbol_define(stab, b[i].name, SYM_KIND_COMMAND);
		v = value_new_builtin(b[i].fn);
		activation_set_value(global_ar, sym, v);
		value_release(v);
	}
	
	/* XXX */
	sym = symbol_define(stab, "EoL", SYM_KIND_VARIABLE);
	v = value_new_string("\n");
	activation_set_value(global_ar, sym, v);
	value_release(v);

	sym = symbol_define(stab, "True", SYM_KIND_VARIABLE);
	v = value_new_boolean(1);
	activation_set_value(global_ar, sym, v);
	value_release(v);

	sym = symbol_define(stab, "False", SYM_KIND_VARIABLE);
	v = value_new_boolean(0);
	activation_set_value(global_ar, sym, v);
	value_release(v);
}

int
main(int argc, char **argv)
{
	char **real_argv = argv;
	struct scan_st *sc;
	struct symbol_table *stab;
	struct ast *a;
	struct value *v;
	char *source = NULL;
	int opt;
#ifdef DEBUG
	int run_program = 1;
	int dump_symbols_beforehand = 0;
	int dump_symbols_afterwards = 0;
	int dump_program = 0;
#define OPTS "acfknprstvz"
#define RUN_PROGRAM run_program
#else
#define OPTS ""
#define RUN_PROGRAM 1
#endif

#ifdef DEBUG
	setvbuf(stdout, NULL, _IOLBF, 0);
#endif

	/*
	 * Get command-line arguments.
	 */
	while ((opt = getopt(argc, argv, OPTS)) != -1) {
		switch(opt) {
#ifdef DEBUG
		case 'a':
			trace_assignments++;
			break;
		case 'c':
			trace_calls++;
			break;
		case 'f':
			debug_frame++;
			break;
		case 'k':
			trace_closures++;
			break;
		case 'n':
			run_program = 0;
			break;
		case 'p':
			dump_program = 1;
			break;
		case 'r':
			trace_refcounting++;
			break;
		case 's':
			dump_symbols_beforehand = 1;
			break;
		case 't':
			trace_ast++;
			break;
		case 'v':
			trace_activations++;
			break;
		case 'z':
			dump_symbols_afterwards = 1;
			break;
#endif
		case '?':
		default:
			usage(argv);
		}
	}
	argc -= optind;
	argv += optind;

	if (*argv != NULL)
		source = *argv;
	else
		usage(real_argv);
	
	if ((sc = scan_open(source)) != NULL) {
		stab = symbol_table_new(NULL);
		global_ar = activation_new(stab, NULL);
		load_builtins(stab, builtins);
		a = parse_program(sc, stab);
		scan_close(sc);
#ifdef DEBUG
		if (dump_symbols_beforehand)
			symbol_table_dump(stab, 1);
		if (dump_program) {
			ast_dump(a, 0);
		}
#endif
		if (sc->errors == 0 && RUN_PROGRAM) {
			v = value_new_integer(76);
			current_ar = global_ar;
			ast_eval(a, &v);
			value_release(v);
		}
#ifdef DEBUG
		if (dump_symbols_afterwards)
			symbol_table_dump(stab, 1);
#endif
		ast_free(a);
		symbol_table_free(stab);
#ifdef DEBUG
		if (trace_refcounting > 0) {
			value_dump_global_table();
			printf("Created:  %8d\n", num_vars_created);
			printf("Grabbed:  %8d\n", num_vars_grabbed);
			printf("Released: %8d\n", num_vars_released);
			printf("Freed:    %8d\n", num_vars_freed);
			printf("CoW'ed:   %8d\n", num_vars_cowed);
		}
#endif
		return(0);
	} else {
		fprintf(stderr, "Can't open `%s'\n", source);
		return(1);
	}
}
