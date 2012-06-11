#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mem.h"
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
#include "gc.h"
#include "trace.h"
#include "process.h"
#include "icode.h"

#ifdef POOL_VALUES
#include "pool.h"
#endif

#ifdef DEBUG
#define OPTS "cdgG:ilmnopsvy"
#define RUN_PROGRAM run_program
#else
#define OPTS "G:i"
#define RUN_PROGRAM 1
#endif

struct activation *global_ar;

extern int gc_trigger;
extern int gc_target;

void
usage(char **argv)
{
	fprintf(stderr, "Usage: %s [-" OPTS "] source\n",
	    argv[0]);
#ifdef DEBUG
	fprintf(stderr, "  -c: trace process context switching\n");
	fprintf(stderr, "  -d: trace pooling\n");
	fprintf(stderr, "  -g: trace garbage collection\n");
#endif
	fprintf(stderr, "  -G int: set garbage collection threshold\n");
	fprintf(stderr, "  -i: create and dump intermediate format\n");
#ifdef DEBUG
	fprintf(stderr, "  -l: trace bytecode generation (implies -x)\n");
	fprintf(stderr, "  -m: trace virtual machine\n");
	fprintf(stderr, "  -n: don't actually run program\n");
	fprintf(stderr, "  -o: trace allocations\n");
	fprintf(stderr, "  -p: dump program AST before run\n");
	fprintf(stderr, "  -s: dump symbol table before run\n");
	fprintf(stderr, "  -v: trace activation records\n");
	fprintf(stderr, "  -y: trace type inference\n");
#endif
	exit(1);
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
	int err_count = 0;
#ifdef DEBUG
	int run_program = 1;
	int dump_symbols = 0;
	int dump_program = 0;
#endif
	int make_icode = 0;

#ifdef DEBUG
	setvbuf(stdout, NULL, _IOLBF, 0);
#endif

	/*
	 * Get command-line arguments.
	 */
	while ((opt = getopt(argc, argv, OPTS)) != -1) {
		switch(opt) {
#ifdef DEBUG
		case 'c':
			trace_scheduling++;
			break;
#ifdef POOL_VALUES
		case 'd':
			trace_pool++;
			break;
#endif
		case 'g':
			trace_gc++;
			break;
#endif
		case 'G':
			gc_trigger = atoi(optarg);
			break;
		case 'i':
			make_icode++;
			break;
#ifdef DEBUG
		case 'l':
			trace_gen++;
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
		case 's':
			dump_symbols = 1;
			break;
		case 'v':
			trace_activations++;
			break;
		case 'y':
			trace_type_inference++;
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

#ifdef POOL_VALUES
	value_pool_new();
#endif

	gc_target = gc_trigger;
	if ((sc = scan_open(source)) != NULL) {
		stab = symbol_table_new(NULL, 0);
		global_ar = activation_new_on_heap(100, NULL, NULL);
		register_std_builtins(stab);
		report_start();
		a = parse_program(sc, stab);
		scan_close(sc);
#ifdef DEBUG
		if (dump_symbols)
			symbol_table_dump(stab, 1);
		if (dump_program) {
			ast_dump(a, 0);
		}
#endif
#ifndef DEBUG
		symbol_table_free(stab);
		types_free();
#endif
		err_count = report_finish();
		if (err_count == 0) {
			struct iprogram *ip;
			struct vm *vm;
			struct process *p;
			unsigned char *program;

			program = bhuna_malloc(16384);

			if (make_icode > 0) {
				ip = ast_gen_iprogram(a);
				iprogram_eliminate_nops(ip);
				iprogram_optimize_tail_calls(ip);
				iprogram_optimize_push_small_ints(ip);
				iprogram_eliminate_dead_code(ip);
				iprogram_gen(&program, ip);
				if (make_icode > 1)
					iprogram_dump(ip, program);
			} else {
				ast_gen(&program, a);
			}

			vm = vm_new(program, 16384);
			vm_set_pc(vm, program);
			vm->current_ar = global_ar;
			p = process_new(vm);
			/* ast_dump(a, 0); */
			if (RUN_PROGRAM) {
				process_scheduler();
			}
			vm_free(vm);
			bhuna_free(program);
			/*value_dump_global_table();*/
		}

		ast_free(a);	/* XXX move on up */
		/* gc(); */		/* actually do a full blow out at the end */
		/* activation_free_from_stack(global_ar); */
#ifdef DEBUG
		symbol_table_free(stab);
		types_free();
		if (trace_valloc > 0) {
			value_dump_global_table();
			printf("Created:  %8d\n", num_vars_created);
			printf("Cached:   %8d\n", num_vars_cached);
			printf("Freed:    %8d\n", num_vars_freed);
		}
		if (trace_activations > 0) {
			printf("AR's alloc'ed:  %8d\n", activations_allocated);
			printf("AR's freed:     %8d\n", activations_freed);
		}
#ifdef POOL_VALUES
		if (trace_pool > 0) {
			pool_report();
		}
#endif
#endif
		return(0);
	} else {
		fprintf(stderr, "Can't open `%s'\n", source);
		return(1);
	}
}
