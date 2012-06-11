/*
 * buffer.h
 * $Id$
 */

#ifndef __BUFFER_H_
#define	__BUFFER_H_

struct buffer {
	char	*buf;
	size_t	 len;
	size_t	 size;
	size_t	 pos;
};

struct buffer		*buffer_new(size_t);
void			 buffer_free(struct buffer *);
char			*buffer_buf(struct buffer *);
size_t			 buffer_len(struct buffer *);
size_t			 buffer_size(struct buffer *);

void			 buffer_ensure_size(struct buffer *, size_t);
void			 buffer_set(struct buffer *, char *, size_t);
void			 buffer_append(struct buffer *, char *, size_t);

void			 buffer_cpy(struct buffer *, char *);
void			 buffer_cat(struct buffer *, char *);
int			 buffer_cat_file(struct buffer *, char *, ...);
int			 buffer_cat_pipe(struct buffer *, char *, ...);

int			 buffer_seek(struct buffer *, size_t);
size_t			 buffer_tell(struct buffer *);
int			 buffer_eof(struct buffer *);
char			 buffer_peek_char(struct buffer *);
char			 buffer_scan_char(struct buffer *);
int			 buffer_compare(struct buffer *, char *);
int			 buffer_expect(struct buffer *, char *);

void			 buffer_push(struct buffer *, void *, size_t);
int			 buffer_pop(struct buffer *, void *, size_t);

#endif /* !__BUFFER_H_ */
