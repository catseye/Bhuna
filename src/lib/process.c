#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "process.h"
#include "vm.h"
#include "closure.h"
#include "ast.h"
#include "activation.h"

#define TIMESLICE	4096
extern int trace_scheduling;

struct process		*current_process = NULL;
struct process		*run_head = NULL;
struct process		*wait_head = NULL;

static int		 procno = 1;

struct process *
process_new(struct vm *vm)
{
	struct process *p;

	p = bhuna_malloc(sizeof(struct process));
	p->vm = vm;
	p->number = procno++;
	p->next = run_head;
	p->prev = NULL;
	if (run_head != NULL)
		run_head->prev = p;
	run_head = p;

	return(p);
}

void
process_free(struct process *p)
{
	if (p == NULL) return;

#ifdef DEBUG
	if (trace_scheduling)
		printf("vm activation stack hiwater was %d bytes\n",
		    p->vm->astack_hi - p->vm->astack);
#endif

	if (p->prev != NULL)
		p->prev->next = p->next;
	else if (p == run_head)
		run_head = p->next;
	else if (p == wait_head)
		wait_head = p->next;

	if (p->next != NULL)
		p->next->prev = p->prev;

	bhuna_free(p);
}

struct process *
process_spawn(struct closure *k)
{
	struct vm *vm;
	struct process *p;

	vm = vm_new(current_process->vm->program, current_process->vm->prog_size);
	vm_set_pc(vm, k->ast->label);

	vm->current_ar = activation_new_on_heap(
	    k->arity + k->locals, NULL, k->ar);

	p = process_new(vm);
#ifdef DEBUG
	if (trace_scheduling)
		printf("process #%d created\n", p->number);
#endif

	return(p);
}

void
process_scheduler(void)
{
	struct process *next;

	if ((current_process = run_head) == NULL)
		return;

	for (;;) {
		next = current_process->next;
#ifdef DEBUG
		if (trace_scheduling)
			printf("context switched to process #%d\n", current_process->number);
#endif
		switch (vm_run(current_process->vm, TIMESLICE)) {
		case VM_TERMINATED:
#ifdef DEBUG
			if (trace_scheduling)
				printf("process #%d terminated\n", current_process->number);
#endif
			process_free(current_process);
			break;
		case VM_RETURNED:
#ifdef DEBUG
			if (trace_scheduling)
				printf("process #%d returned\n", current_process->number);
#endif
			process_free(current_process);
			break;
		case VM_TIME_EXPIRED:
		case VM_WAITING:
		default:
			break;
		}
		if ((current_process = next) == NULL)
			if ((current_process = run_head) == NULL)
				return;
	}
}
