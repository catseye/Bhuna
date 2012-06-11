#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "process.h"
#include "vm.h"
#include "closure.h"
#include "ast.h"
#include "activation.h"

#define TIMESLICE	2048 /* 4096 */
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
	p->msg_head = NULL;
	p->asleep = 0;
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
	vm_set_pc(vm, k->label);

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
process_send(struct process *p, struct value v)
{
	struct message *m;

	m = bhuna_malloc(sizeof(struct message));
	m->next = p->msg_head;
	p->msg_head = m;
	m->payload = v;

#ifdef DEBUG
	if (trace_scheduling) {
		printf("send from process #%d to process #%d: ",
		    current_process->number, p->number);
		value_print(v);
		printf("\n");
	}
#endif

	process_awaken(p);
}

/*
 * Returns 1 if a message was obtained from the mailbox,
 * 0 if there were no messages waiting (indicating: go to sleep.)
 */
int
process_recv(struct value *v)
{
	struct message *m;

	if (current_process->msg_head == NULL)
		return(0);

	m = current_process->msg_head;
	*v = m->payload;
	current_process->msg_head = m->next;
	bhuna_free(m);

#ifdef DEBUG
	if (trace_scheduling) {
		printf("received in process #%d: ",
		    current_process->number);
		value_print(*v);
		printf("\n");
	}
#endif

	return(1);
}

void
process_sleep(struct process *p)
{
	if (p == NULL || p->asleep)
		return;

	/* remove from run list, add to wait list */
	if (p->prev != NULL)
		p->prev->next = p->next;
	else if (p == run_head)
		run_head = p->next;

	if (p->next != NULL)
		p->next->prev = p->prev;

	p->prev = NULL;
	p->next = wait_head;
	if (wait_head != NULL)
		wait_head->prev = p;
	wait_head = p;

	p->asleep = 1;
}

void
process_awaken(struct process *p)
{
	if (p == NULL || !p->asleep)
		return;

	/* remove from run list, add to run list */
	if (p->prev != NULL)
		p->prev->next = p->next;
	else if (p == wait_head)
		wait_head = p->next;

	if (p->next != NULL)
		p->next->prev = p->prev;

	p->prev = NULL;
	p->next = run_head;
	if (run_head != NULL)
		run_head->prev = p;
	run_head = p;

	p->asleep = 0;
}

/******** SCHEDULER ********/

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
		case VM_WAITING:
#ifdef DEBUG
			if (trace_scheduling)
				printf("process #%d falling asleep\n", current_process->number);
#endif
			process_sleep(current_process);
			break;
		case VM_TIME_EXPIRED:
		default:
			break;
		}
		if ((current_process = next) == NULL)
			if ((current_process = run_head) == NULL)
				return;
	}
}
