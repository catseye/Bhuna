#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "scan.h"
#include "parse.h"
#include "symbol.h"
#include "ast.h"
#include "builtin.h"
#include "value.h"
#include "activation.h"
#include "vm.h"
#include "type.h"
#include "report.h"

#ifdef POOL_VALUES
#include "pool.h"
#endif

#ifdef DEBUG
int trace_ast = 0;
int trace_activations = 0;
int trace_calls = 0;
int trace_assignments = 0;
int trace_valloc = 0;
int trace_refcounting = 0;
int trace_closures = 0;
int trace_vm = 0;
int trace_gen = 0;
int trace_pool = 0;
int trace_type_inference = 0;

int num_vars_created = 0;
int num_vars_grabbed = 0;
int num_vars_released = 0;
int num_vars_freed = 0;
int num_vars_cowed = 0;
int debug_frame = 0;

int activations_allocated = 0;
int activations_freed = 0;
#endif

#ifdef DEBUG
#define OPTS "acdfg:klmnoprstvxyz"
#define RUN_PROGRAM run_program
#else
#define OPTS "g:x"
#define RUN_PROGRAM 1
#endif

struct activation *global_ar;
struct activation *current_ar;
int gc_trigger = DEFAULT_GC_TRIGGER;
int gc_target;

void
usage(char **argv)
{
	fprintf(stderr, "Usage: %s [-" OPTS "] source\n",
	    argv[0]);
#ifdef DEBUG
	fprintf(stderr, "  -a: trace assignments\n");
	fprintf(stderr, "  -c: trace calls\n");
	fprintf(stderr, "  -d: trace pooling\n");
	fprintf(stderr, "  -f: trace frames\n");
#endif
	fprintf(stderr, "  -g int: set garbage collection threshold\n");
#ifdef DEBUG
	fprintf(stderr, "  -k: trace closures\n");
	fprintf(stderr, "  -l: trace bytecode generation (implies -x)\n");
	fprintf(stderr, "  -m: trace virtual machine\n");
	fprintf(stderr, "  -n: don't actually run program\n");
	fprintf(stderr, "  -o: trace allocations\n");
	fprintf(stderr, "  -p: dump program AST before run\n");
	fprintf(stderr, "  -r: show reference counting stats (-rr = trace refcounting)\n");
	fprintf(stderr, "  -s: dump symbol table before run\n");
	fprintf(stderr, "  -t: trace AST transitions in evaluator\n");
	fprintf(stderr, "  -v: trace activation records\n");
#endif
	fprintf(stderr, "  -x: execute bytecode (unless -n)\n");
#ifdef DEBUG
	fprintf(stderr, "  -y: trace type inference\n");
	fprintf(stderr, "  -z: dump symbol table after run\n");
#endif
	exit(1);
}

void
load_builtins(struct symbol_table *stab, struct builtin *b)
{
	int i;
	struct value *v;
	struct symbol *sym;

	for (i = 0; b[i].name != NULL; i++) {
		v = value_new_builtin(&b[i]);
		sym = symbol_define(stab, b[i].name, SYM_KIND_COMMAND, v);
		sym->is_pure = b[i].is_pure;
		sym->builtin = &b[i];
		sym->type = b[i].ty();
		value_release(v);
	}

	/* XXX */
	v = value_new_string("\n");
	sym = symbol_define(stab, "EoL", SYM_KIND_VARIABLE, v);
	sym->type = type_new(TYPE_STRING);
	value_release(v);

	v = value_new_boolean(1);
	sym = symbol_define(stab, "True", SYM_KIND_VARIABLE, v);
	sym->type = type_new(TYPE_BOOLEAN);
	value_release(v);

	v = value_new_boolean(0);
	sym = symbol_define(stab, "False", SYM_KIND_VARIABLE, v);
	sym->type = type_new(TYPE_BOOLEAN);
	value_release(v);
}

int
main(int argc, char **argv)
{
	char **real_argv = argv;
	struct scan_st *sc;
	struct symbol_table *stab;
	struct ast *a;
	char *source = NULL;
	int opt;
	int use_vm = 0;
	int err_count = 0;
#ifdef DEBUG
	int run_program = 1;
	int dump_symbols_beforehand = 0;
	int dump_symbols_afterwards = 0;
	int dump_program = 0;
#endif

#ifdef DEBUG
	setvbuf(stdout, NULL, _IOLBF, 0);
	/*
	printf("sizeof(value) = %d\n", sizeof(struct value));
	printf("sizeof(fat_value) = %d\n", sizeof(struct fat_value));
	*/
#endif

#ifdef POOL_VALUES
	value_pool_new();
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
		case 'd':
			trace_pool++;
			break;
		case 'f':
			debug_frame++;
			break;
#endif
		case 'g':
			gc_trigger = atoi(optarg);
			break;
#ifdef DEBUG
		case 'k':
			trace_closures++;
			break;
		case 'l':
			trace_gen++;
			use_vm = 1;
			break;
		case 'm':
			trace_vm++;
			break;
		case 'n':
			run_program = 0;
			break;
		case 'o':
			trace_valloc++;
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
#endif
		case 'x':
			use_vm = 1;
			break;
#ifdef DEBUG
		case 'y':
			trace_type_inference++;
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

	gc_target = gc_trigger;
	if ((sc = scan_open(source)) != NULL) {
		stab = symbol_table_new(NULL, 0);
		global_ar = activation_new_on_stack(100, NULL, NULL);
		load_builtins(stab, builtins);
		report_start();
		a = parse_program(sc, stab);
		scan_close(sc);
		current_ar = global_ar;
#ifdef DEBUG
		if (dump_symbols_beforehand)
			symbol_table_dump(stab, 1);
		if (dump_program) {
			ast_dump(a, 0);
		}
#endif
#ifndef DEBUG
		symbol_table_free(stab);
#endif
		err_count = report_finish();
		if (err_count == 0 && use_vm) {
			unsigned char *program;

			program = ast_gen(a);
			/* ast_dump(a, 0); */
			if (RUN_PROGRAM) {
				vm_run(program);
			}
			vm_release(program);
			/*value_dump_global_table();*/
#ifdef RECURSIVE_AST_EVALUATOR
		} else if (err_count == 0 && RUN_PROGRAM) {
			v = value_new_integer(76);
			ast_eval_init();
			ast_eval(a, &v);
			value_release(v);
#endif
		}
#ifdef DEBUG
		if (dump_symbols_afterwards)
			symbol_table_dump(stab, 1);
#endif
		ast_free(a);
		activation_gc();
		activation_free_from_stack(global_ar);
#ifdef DEBUG
		symbol_table_free(stab);
		if (trace_refcounting > 0) {
			value_dump_global_table();
			printf("Created:  %8d\n", num_vars_created);
			printf("Grabbed:  %8d\n", num_vars_grabbed);
			printf("Released: %8d\n", num_vars_released);
			printf("Freed:    %8d\n", num_vars_freed);
			printf("CoW'ed:   %8d\n", num_vars_cowed);
		}
		if (trace_activations > 0) {
			printf("AR's alloc'ed:  %8d\n", activations_allocated);
			printf("AR's freed:     %8d\n", activations_freed);
		}
#endif
		return(0);
	} else {
		fprintf(stderr, "Can't open `%s'\n", source);
		return(1);
	}
}
