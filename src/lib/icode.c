#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "mem.h"
#include "icode.h"
#include "ast.h"
#include "vm.h"
#include "builtin.h"
#include "value.h"
#include "closure.h"
#include "utf8.h"

/*** iprograms ***/

struct iprogram	*
iprogram_new(void)
{
	struct iprogram *ip;

	ip = bhuna_malloc(sizeof(struct iprogram));
	ip->head = NULL;
	ip->tail = NULL;

	return(ip);
}
	
void
iprogram_free(struct iprogram *ip)
{
	struct icode *ic, *next;

	if (ip == NULL) return;

	for (ic = ip->head; ic != NULL; ic = next) {
		next = ic->next;
		icode_free(NULL, ic);
	}

	bhuna_free(ip);
}

/*** icodes ***/

/*
static void
iprogram_append(struct iprogram *ip, struct icode *ic)
{
	ic->next = NULL;
	ic->prev = ip->tail;

	if (ip->tail == NULL) {
		ip->head = ic;
		ip->tail = ic;
	} else {
		ip->tail->next = ic;
		ip->tail = ic;
	}

	return(ic);
}
*/

struct icode *
icode_new(struct iprogram *ip, int opcode)
{
	struct icode *ic;

	ic = bhuna_malloc(sizeof(struct icode));
	ic->opcode = opcode;
	ic->referrers = NULL;
	ic->label = NULL;

	/* leave operand unitialized */

	/* append to program */
	ic->next = NULL;
	ic->prev = ip->tail;

	if (ip->tail == NULL) {
		ip->head = ic;
		ip->tail = ic;
		ic->number = 1;
	} else {
		ip->tail->next = ic;
		ip->tail = ic;
		ic->number = ic->prev->number + 1;
	}

	return(ic);
}

struct icode *
icode_new_local(struct iprogram *ip, int opcode, int index, int upcount)
{
	struct icode *ic;

	ic = icode_new(ip, opcode);
	ic->operand.local.index = index;
	ic->operand.local.upcount = upcount;

	return(ic);
}

struct icode *
icode_new_value(struct iprogram *ip, int opcode, struct value value)
{
	struct icode *ic;

	ic = icode_new(ip, opcode);
	ic->operand.value = value;

	return(ic);
}

struct icode *
icode_new_builtin(struct iprogram *ip, struct builtin *bi)
{
	struct icode *ic;

	ic = icode_new(ip, INSTR_EXTERNAL);

	if (bi->index >= 0) {
		ic->opcode = bi->index;
	} else {
		ic->operand.builtin = bi;
	}

	return(ic);
}

/*** destructor ***/

/*
 * Since this might be done while optimizing, take care to leave the
 * rest of the program intact.
 */
void
icode_free(struct iprogram *ip, struct icode *ic)
{
	struct icomefrom *icf, *icf_next;

	if (ic == NULL) return;

	if (ic->opcode == INSTR_JMP || ic->opcode == INSTR_JZ) {
		referrer_unwire(ic, ic->operand.branch);
	}

	for (icf = ic->referrers; icf != NULL; icf = icf_next) {
		icf_next = icf->next;
		bhuna_free(icf);
	}

	if (ip != NULL) {
		if (ip->head == ic)
			ip->head = ic->next;
		else if (ic->prev != NULL)
			ic->prev->next = ic->next;

		if (ip->tail == ic)
			ip->tail = ic->prev;
		else if (ic->next != NULL)
			ic->next->prev = ic->prev;
	}

	bhuna_free(ic);
}

/*** util ***/

void
icode_set_branch(struct icode *ic, struct icode *branch)
{
	struct icomefrom *icf;

	ic->operand.branch = branch;
	icf = bhuna_malloc(sizeof(struct icomefrom));
	icf->type = ICOMEFROM_ICODE;
	icf->referrer.icode = ic;
	icf->next = branch->referrers;
	branch->referrers = icf;
}

void
icode_set_closure_entry_point(struct closure *k, struct icode *ic)
{
	struct icomefrom *icf;

	k->icode = ic;
	icf = bhuna_malloc(sizeof(struct icomefrom));
	icf->type = ICOMEFROM_CLOSURE;
	icf->referrer.closure = k;
	icf->next = ic->referrers;
	ic->referrers = icf;
}

/*************** intermediate code generator ****************/

static void
ast_gen_r(struct iprogram *ip, struct ast *a)
{
	struct icode *ic1, *ic2, *ic3, *ic4;
	struct value v;

	if (a == NULL)
		return;

	/* a->icode = gptr; */
	switch (a->type) {
	case AST_LOCAL:
		icode_new_local(ip, INSTR_PUSH_LOCAL,
		    a->u.local.index, a->u.local.upcount);
		break;
	case AST_VALUE:
		icode_new_value(ip, INSTR_PUSH_VALUE, a->u.value.value);
		if (a->u.value.value.type == VALUE_CLOSURE) {
			icode_new(ip, INSTR_SET_ACTIVATION);
			ic1 = icode_new(ip, INSTR_JMP);
			ic2 = icode_new(ip, INSTR_NOP);
			icode_set_closure_entry_point(a->u.value.value.v.s->v.k, ic2);
			ast_gen_r(ip, a->u.value.value.v.s->v.k->ast);
			icode_new(ip, INSTR_RET);
			ic3 = icode_new(ip, INSTR_NOP);
			icode_set_branch(ic1, ic3);
		}
		break;
	case AST_BUILTIN:
		if (a->u.builtin.bi->index == INDEX_BUILTIN_STORE) {
			struct ast *lvalue;
			
			lvalue = ast_find_local(a);
			icode_new_local(ip, INSTR_COW_LOCAL,
			    lvalue->u.local.index, lvalue->u.local.upcount);
		}
		ast_gen_r(ip, a->u.builtin.right);
		if (a->u.builtin.bi->arity == -1) {
			v = value_new_integer(ast_count_args(a->u.builtin.right));
			value_deregister(v);
			icode_new_value(ip, INSTR_PUSH_VALUE, v);
		}
		icode_new_builtin(ip, a->u.builtin.bi);
		break;
	case AST_APPLY:
		ast_gen_r(ip, a->u.apply.right);
		ast_gen_r(ip, a->u.apply.left);
		icode_new(ip, INSTR_CALL);
		break;
	case AST_ROUTINE:
		ast_gen_r(ip, a->u.routine.body);
		break;
	case AST_ARG:
		ast_gen_r(ip, a->u.arg.left);
		ast_gen_r(ip, a->u.arg.right);
		break;
	case AST_STATEMENT:
		ast_gen_r(ip, a->u.statement.left);
		ast_gen_r(ip, a->u.statement.right);
		break;
	case AST_ASSIGNMENT:
		assert(a->u.assignment.left != NULL);
		assert(a->u.assignment.left->type == AST_LOCAL);
		ast_gen_r(ip, a->u.assignment.right);
		if (a->u.assignment.defining) {
			icode_new_local(ip, INSTR_INIT_LOCAL,
			    a->u.assignment.left->u.local.index,
			    a->u.assignment.left->u.local.upcount);
		} else {
			icode_new_local(ip, INSTR_POP_LOCAL,
			    a->u.assignment.left->u.local.index,
			    a->u.assignment.left->u.local.upcount);
		}
		break;
	case AST_CONDITIONAL:
		ast_gen_r(ip, a->u.conditional.test);
		ic1 = icode_new(ip, INSTR_JZ);
		ast_gen_r(ip, a->u.conditional.yes);
		if (a->u.conditional.no != NULL) {
			ic2 = icode_new(ip, INSTR_JMP);
			ic3 = icode_new(ip, INSTR_NOP);
			icode_set_branch(ic1, ic3);
			ast_gen_r(ip, a->u.conditional.no);
			ic4 = icode_new(ip, INSTR_NOP);
			icode_set_branch(ic2, ic4);
		} else {
			ic3 = icode_new(ip, INSTR_NOP);
			icode_set_branch(ic1, ic3);
		}
		break;
	case AST_WHILE_LOOP:
		ic1 = icode_new(ip, INSTR_NOP);
		ast_gen_r(ip, a->u.while_loop.test);
		ic2 = icode_new(ip, INSTR_JZ);
		ast_gen_r(ip, a->u.while_loop.body);
		ic3 = icode_new(ip, INSTR_JMP);
		icode_set_branch(ic3, ic1);
		ic4 = icode_new(ip, INSTR_NOP);
		icode_set_branch(ic2, ic4);
		break;
	case AST_RETR:
		ast_gen_r(ip, a->u.retr.body);
		icode_new(ip, INSTR_RET);
		break;
	}
}

struct iprogram *
ast_gen_iprogram(struct ast *a)
{
	struct iprogram *ip;

	ip = iprogram_new();
	ast_gen_r(ip, a);
	icode_new(ip, INSTR_HALT);

	return(ip);
}

char *
icode_addr(struct icode *ic, vm_label_t program)
{
	static char s[256];

	if (program == NULL) {
		snprintf(s, 256, "No.%d", ic->number);
	} else {
		snprintf(s, 256, "#%3d", ic->label - program);
	}

	return(s);
}
	
void
iprogram_dump(struct iprogram *ip, vm_label_t program)
{
	struct icode *ic;
	struct icomefrom *icf;

	for (ic = ip->head; ic != NULL; ic = ic->next) {
		printf("%s: ", icode_addr(ic, program));
		if (ic->referrers != NULL) {
			printf("{");
			for (icf = ic->referrers; icf != NULL; icf = icf->next) {
				if (icf->type == ICOMEFROM_ICODE) {
					printf("%s", icode_addr(icf->referrer.icode, program));
				} else {
					printf("#(closure)");
				}
				if (icf->next != NULL)
					printf(", ");
			}
			printf("} ");
		}
		switch (ic->opcode) {
		case INSTR_HALT:
			printf("HALT");
			break;
		case INSTR_PUSH_ZERO:
			printf("PUSH_ZERO");
			break;
		case INSTR_PUSH_ONE:
			printf("PUSH_ONE");
			break;
		case INSTR_PUSH_TWO:
			printf("PUSH_TWO");
			break;
		case INSTR_PUSH_VALUE:
			printf("PUSH_VALUE ");
			value_print(ic->operand.value);
			break;
		case INSTR_PUSH_LOCAL:
			printf("PUSH_LOCAL (%d,%d)",
			    ic->operand.local.index, ic->operand.local.upcount);
			break;
		case INSTR_POP_LOCAL:
			printf("POP_LOCAL (%d,%d)",
			    ic->operand.local.index, ic->operand.local.upcount);
			break;
		case INSTR_INIT_LOCAL:
			printf("INIT_LOCAL (%d,%d)",
			    ic->operand.local.index, ic->operand.local.upcount);
			break;
		case INSTR_JZ:
			printf("JZ %s", icode_addr(ic->operand.branch, program));
			break;
		case INSTR_JMP:
			printf("JMP %s", icode_addr(ic->operand.branch, program));
			break;
		case INSTR_CALL:
			printf("CALL");
			break;
		case INSTR_RET:
			printf("RET");
			break;
		case INSTR_GOTO:
			printf("GOTO");
			break;
		case INSTR_SET_ACTIVATION:
			printf("SET_ACTIVATION");
			break;
		case INSTR_COW_LOCAL:
			printf("COW_LOCAL (%d,%d)",
			    ic->operand.local.index, ic->operand.local.upcount);
		case INSTR_EXTERNAL:
			printf("EXTERNAL `"),
			fputsu8(stdout, ic->operand.builtin->name);
			printf("'");
			break;
		case INSTR_NOP:
			printf("NOP");
			break;

		default:
			printf("BUILTIN `");
			fputsu8(stdout, builtins[ic->opcode].name);
			printf("'");
		}
		printf("\n");
	}
}

/*** rewiring ***/

void
referrer_unwire(struct icode *from, struct icode *to)
{
	struct icomefrom *icf, *icf_next, *icf_prev;

	icf_prev = NULL;
	for (icf = to->referrers; icf != NULL; icf = icf_next) {
		icf_next = icf->next;
		if (icf->type == ICOMEFROM_ICODE &&
		    icf->referrer.icode == from) {
			if (icf_prev != NULL)
				icf_prev->next = icf_next;
			else
				to->referrers = icf_next;
			bhuna_free(icf);
		} else {
			icf_prev = icf;
		}
	}
}

void
referrers_rewire(struct icode *from, struct icode *to)
{
	struct icomefrom *icf, *icf_next;

	for (icf = from->referrers; icf != NULL; icf = icf_next) {
		icf_next = icf->next;
		if (icf->type == ICOMEFROM_ICODE) {
			icf->referrer.icode->operand.branch = to;
		} else {
			icf->referrer.closure->icode = to;
		}
		icf->next = to->referrers;
		to->referrers = icf;
	}
	from->referrers = NULL;
}

/********** OPTIMIZATIONS **********/

void
iprogram_eliminate_nops(struct iprogram *ip)
{
	struct icode *ic, *ic_next;

	for (ic = ip->head; ic != NULL; ic = ic_next) {
		ic_next = ic->next;
		if (ic->opcode == INSTR_NOP) {
			assert(ic->next != NULL);
			referrers_rewire(ic, ic->next);
			icode_free(ip, ic);
		}
	}
}

void
iprogram_eliminate_dead_code(struct iprogram *ip)
{
	struct icode *ic, *ic_next;
	int dead = 0;

	for (ic = ip->head; ic != NULL; ic = ic_next) {
		ic_next = ic->next;
		/* pre */
		if (ic->referrers != NULL) {
			dead = 0;
		}
		/* in */
		if (dead) {
			/*printf("#%3d is DEAD\n", ic->number);*/
			icode_free(ip, ic);
			/*ic = ip->head;
			dead = 0;
			continue;*/
		}

		/* post */
		if (ic->opcode == INSTR_JMP || ic->opcode == INSTR_RET ||
		    ic->opcode == INSTR_GOTO || ic->opcode == INSTR_HALT) {
			dead = 1;
		}
	}
}

void
iprogram_optimize_tail_calls(struct iprogram *ip)
{
	struct icode *ic, *ic_next;

	for (ic = ip->head; ic != NULL; ic = ic_next) {
		ic_next = ic->next;
		if (ic->opcode == INSTR_RET && ic->referrers == NULL &&
		    ic->prev != NULL && ic->prev->opcode == INSTR_CALL) {
			ic->prev->opcode = INSTR_GOTO;
			icode_free(ip, ic);
		}
	}
}

void
iprogram_optimize_push_small_ints(struct iprogram *ip)
{
	struct icode *ic, *ic_next;

	for (ic = ip->head; ic != NULL; ic = ic_next) {
		ic_next = ic->next;
		if (ic->opcode == INSTR_PUSH_VALUE &&
		    ic->operand.value.type == VALUE_INTEGER) {
			switch (ic->operand.value.v.i) {
			case 0:
				ic->opcode = INSTR_PUSH_ZERO;
				break;
			case 1:
				ic->opcode = INSTR_PUSH_ONE;
				break;
			case 2:
				ic->opcode = INSTR_PUSH_TWO;
				break;
			}
		}
	}
}

int
count_referrers(struct icode *ic)
{
	struct icomefrom *icf;
	int i = 0;

	for (icf = ic->referrers; icf != NULL; icf = icf->next)
		i++;

	return(i);
}

void
iprogram_eliminate_useless_jumps(struct iprogram *ip)
{
	struct icode *ic, *ic_next;

	for (ic = ip->head; ic != NULL; ic = ic_next) {
		ic_next = ic->next;
		if (ic->opcode == INSTR_JMP &&
		    ic->operand.branch->opcode == INSTR_RET) {
			referrer_unwire(ic, ic->operand.branch);
			ic->opcode = INSTR_RET;
		}
	}
}
