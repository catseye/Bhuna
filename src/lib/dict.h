/*
 * dict.c
 * $Id$
 * Routines to manipulate Bhuna dictionaries.
 */

#ifndef	__DICT_H_
#define	__DICT_H_

struct value;

struct dict {
	struct chain **bucket;
	struct chain *cursor;
	int cur_bucket;
	int num_buckets;
};

struct chain {
	struct chain	*next;
	struct value	*key;
	struct value	*value;
};

struct dict		*dict_new(void);
struct dict		*dict_dup(struct dict *);
void			 dict_free(struct dict *);

struct value		*dict_fetch(struct dict *, struct value *);
int			 dict_exists(struct dict *, struct value *);
void			 dict_store(struct dict *, struct value *, struct value *);

void			 dict_rewind(struct dict *);
int			 dict_eof(struct dict *);
struct value		*dict_getkey(struct dict *);
void			 dict_next(struct dict *);

size_t			 dict_size(struct dict *);

void			 dict_dump(struct dict *);

#endif /* !__DICT_H_ */
