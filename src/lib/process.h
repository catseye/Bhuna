struct vm;
struct closure;

struct process {
	int		 number;
	struct process	*next;
	struct process	*prev;
	struct vm	*vm;
};

extern struct process *current_process;

struct process	*process_new(struct vm *);
void		 process_free(struct process *);
void		 process_scheduler(void);
struct process	*process_spawn(struct closure *);
