struct symbol_table;
struct symbol;
struct value;

struct activation {
	int			 refcount;
	struct alist		*alist;
	struct activation	*enclosing;	/* lexically enclosing activation record */
};

struct alist {
	struct alist	*next;
	struct symbol	*sym;
	struct value	*value;
};


struct activation	*activation_new(struct symbol_table *, struct activation *);
void			 activation_free(struct activation *);

void			 activation_grab(struct activation *);
void			 activation_release(struct activation *);
	
struct value		*activation_get_value(struct activation *, struct symbol *);
void			 activation_set_value(struct activation *, struct symbol *, struct value *);

void			 activation_dump(struct activation *, int);
