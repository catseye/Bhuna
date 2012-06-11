/*
 * trace.h
 * $Id$
 */

#ifdef DEBUG
extern int trace_activations;
extern int trace_valloc;
extern int trace_vm;
extern int trace_gen;
extern int trace_pool;
extern int trace_type_inference;
extern int trace_gc;
extern int trace_scheduling;

extern int num_vars_created;
extern int num_vars_cached;
extern int num_vars_freed;

extern int activations_allocated;
extern int activations_freed;
#endif
