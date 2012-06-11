#define DEFAULT_GC_TRIGGER	10240

struct value;

/*
 * Structure of an activation record.
 * This is actually only the header;
 * the frame itself (containing local variables)
 * follows immediately in memory.
 */
struct activation {
	struct activation	*next;		/* global list of all act recs */
	int			 marked;
	int			 size;
	struct activation	*caller;	/* recursively shallower activation record */
	struct activation	*enclosing;	/* lexically enclosing activation record */
	/*
	struct value		 *value[];
	*/
};

struct activation	*activation_new(int, struct activation *, struct activation *);
void			 activation_free(struct activation *);

struct value		*activation_get_value(struct activation *, int, int);
void			 activation_set_value(struct activation *, int, int, struct value *);

void			 activation_dump(struct activation *, int);

void			 activation_register(struct activation *);
void			 activation_gc(void);
