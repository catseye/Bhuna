#ifndef __PROCESS_H_
#define	__PROCESS_H_

#include "value.h"

struct vm;
struct closure;

struct process {
	int		 asleep;
	int		 number;
	struct process	*next;
	struct process	*prev;
	struct vm	*vm;
	struct message	*msg_head;
};

struct message {
	struct message	*next;
	struct value	 payload;
};

extern struct process *current_process;
extern struct process *run_head;
extern struct process *wait_head;

struct process	*process_new(struct vm *);
void		 process_free(struct process *);
void		 process_scheduler(void);
struct process	*process_spawn(struct closure *);

void		 process_send(struct process *, struct value);
int		 process_recv(struct value *);

void		 process_sleep(struct process *);
void		 process_awaken(struct process *);

#endif
