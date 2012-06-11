#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"
#include "ast.h"
#include "value.h"
#include "list.h"
#include "closure.h"
#include "activation.h"
#include "builtin.h"

extern int trace_gen;

static vm_label_t gptr;
static unsigned char program[65536];

vm_label_t patch_stack[4096];
int psp = 0;

unsigned char *last_ins_at = NULL;
unsigned char last_ins = 255;

/*** backpatcher ***/

static int
request_backpatch(vm_label_t *bp)
{
#ifdef DEBUG
	if (trace_gen)
		printf("[--> #%d]", *bp - program);
#endif
	patch_stack[psp++] = *bp;
	*bp += sizeof(vm_label_t);
	return(psp - 1);
}

static void
backpatch(int bpid)
{
	vm_label_t bp;

	bp = patch_stack[bpid];
	*(vm_label_t *)bp = gptr;
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: [<-- #%d] $%d:\n", gptr - program, bp - program, bpid);
#endif
}

/*** generators ***/

static void
gen(unsigned char ins)
{
	last_ins_at = gptr;
	*gptr++ = last_ins = ins;
}

static void
gen_push_value(struct value *v)
{
#ifdef DEBUG
	if (trace_gen) {
		printf("#%d: *PUSH_VALUE(", gptr - program);
		value_print(v);
		printf(")\n");
	}
#endif

	gen(INSTR_PUSH_VALUE);
	*(((struct value **)gptr)++) = v;
}

static void
gen_push_local(int index, int upcount)
{
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: *PUSH_LOCAL(%d,%d)\n", gptr - program, index, upcount);
#endif

	gen(INSTR_PUSH_LOCAL);
	*gptr++ = (unsigned char)index;
	*gptr++ = (unsigned char)upcount;
}

static void
gen_pop_local(int index, int upcount)
{
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: *POP_LOCAL(%d,%d)\n", gptr - program, index, upcount);
#endif

	gen(INSTR_POP_LOCAL);
	*gptr++ = (unsigned char)index;
	*gptr++ = (unsigned char)upcount;
}

static void
gen_builtin(struct builtin *bi)
{
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: *BUILTIN(%s)\n", gptr - program, bi->name);
#endif

	gen(bi->index);
}

static void
gen_jz_bp(int *bpid)
{
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: *JZ", gptr - program);
#endif
	gen(INSTR_JZ);
	*bpid = request_backpatch(&gptr);
#ifdef DEBUG
	if (trace_gen)
		printf("($%d)\n", *bpid);
#endif
}

static void
gen_jmp_bp(int *bpid)
{
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: *JMP", gptr - program);
#endif
	gen(INSTR_JMP);
	*bpid = request_backpatch(&gptr);
#ifdef DEBUG
	if (trace_gen)
		printf("($%d)\n", *bpid);
#endif
}

static void
gen_jmp(vm_label_t label)
{
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: *JMP", gptr - program);
#endif
	gen(INSTR_JMP);
	*(vm_label_t *)gptr = label;
	gptr += sizeof(vm_label_t);
#ifdef DEBUG
	if (trace_gen)
		printf("(#%d)\n", label - program);
#endif
}

static void
gen_apply(void)
{
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: *CALL\n", gptr - program);
#endif
	gen(INSTR_CALL);
}

static void
gen_ret(void)
{
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: *RET\n", gptr - program);
#endif
	if (last_ins == INSTR_CALL) {
#ifdef DEBUG
		if (trace_gen)
			printf("*** ELIMINATING TAIL CALL\n");
#endif
		*last_ins_at = INSTR_GOTO;
	}
	gen(INSTR_RET);
}

static void
gen_set_activation_bp(int *bpid)
{
#ifdef DEBUG
	if (trace_gen)
		printf("#%d: *SET_ACTIVATION", gptr - program);
#endif
	gen(INSTR_SET_ACTIVATION);
	*bpid = request_backpatch(&gptr);
#ifdef DEBUG
	if (trace_gen)
		printf("($%d)\n", *bpid);
#endif
}

static void
ast_gen_r(struct ast *a)
{
	int bpid_1, bpid_2;
	vm_label_t label;

	if (a == NULL)
		return;

	switch (a->type) {
	case AST_LOCAL:
		gen_push_local(a->u.local.index, a->u.local.upcount);
		break;
	case AST_VALUE:
		gen_push_value(a->u.value.value);
		if (a->u.value.value->type == VALUE_CLOSURE) {
			gen_set_activation_bp(&bpid_1);
			gen_jmp_bp(&bpid_2);
			backpatch(bpid_1);
			ast_gen_r(a->u.value.value->v.k->ast);
			gen_ret();
			backpatch(bpid_2);
		}
		break;
	case AST_BUILTIN:
		ast_gen_r(a->u.builtin.left);
		ast_gen_r(a->u.builtin.right);
		gen_builtin(a->u.builtin.bi);
		break;
	case AST_APPLY:
		ast_gen_r(a->u.apply.right);
		ast_gen_r(a->u.apply.left);
		gen_apply();
		break;
	case AST_ARG:
		ast_gen_r(a->u.arg.left);
		ast_gen_r(a->u.arg.right);
		break;
	case AST_STATEMENT:
		ast_gen_r(a->u.statement.left);
		ast_gen_r(a->u.statement.right);
		break;
	case AST_ASSIGNMENT:
		assert(a->u.assignment.left != NULL);
		assert(a->u.assignment.left->type == AST_LOCAL);
		ast_gen_r(a->u.assignment.right);
		gen_pop_local(a->u.assignment.left->u.local.index,
		    a->u.assignment.left->u.local.upcount);
		break;
	case AST_CONDITIONAL:
		ast_gen_r(a->u.conditional.test);
		gen_jz_bp(&bpid_1);
		ast_gen_r(a->u.conditional.yes);
		if (a->u.conditional.no != NULL)
			gen_jmp_bp(&bpid_2);
		backpatch(bpid_1);
		if (a->u.conditional.no != NULL) {
			ast_gen_r(a->u.conditional.no);
			backpatch(bpid_2);
		}
		break;
	case AST_WHILE_LOOP:
		label = gptr;
		ast_gen_r(a->u.while_loop.test);
		gen_jz_bp(&bpid_1);
		ast_gen_r(a->u.while_loop.body);
		gen_jmp(label);
		backpatch(bpid_1);
		break;
	case AST_RETR:
		ast_gen_r(a->u.retr.body);
		gen_ret();
		break;
	}
}

vm_label_t
ast_gen(struct ast *a)
{
	gptr = program;
	ast_gen_r(a);
	*gptr++ = INSTR_HALT;
	return(program);
}
