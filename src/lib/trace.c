#ifdef DEBUG
int trace_activations = 0;
int trace_valloc = 0;
int trace_vm = 0;
int trace_gen = 0;
int trace_pool = 0;
int trace_type_inference = 0;
int trace_gc = 0;
int trace_scheduling = 0;

int num_vars_created = 0;
int num_vars_cached = 0;
int num_vars_freed = 0;

int activations_allocated = 0;
int activations_freed = 0;
#endif
