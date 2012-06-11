/*
 * scan.h
 * Lexical scanner structures and prototypes for Bhuna.
 * $Id$
 */

#ifndef __SCAN_H_
#define __SCAN_H_

#include <stdio.h>
#include <wchar.h>

#define	TOKEN_EOF	0
#define	TOKEN_NUMBER	1
#define	TOKEN_BAREWORD	2
#define	TOKEN_SYMBOL	3
#define	TOKEN_QSTRING	4

struct scan_st {
	FILE	*in;		/* file from which we are scanning */
	wchar_t	*token;		/* text content of token we just scanned */
	int	 type;		/* type of token that was scanned */
	int	 lino;		/* current line number, 1-based */
	int	 columno;	/* current column number, 1-based */
	int	 lastcol;	/* for putback */
};

#define tokeq(sc, x)	(wcscmp(sc->token, x) == 0)
#define tokne(sc, x)	(wcscmp(sc->token, x) != 0)

extern struct scan_st *scan_open(char *);
extern struct scan_st *scan_dup(struct scan_st *);
extern void scan_close(struct scan_st *);
extern void scan(struct scan_st *);
extern void scan_expect(struct scan_st *, wchar_t *);

#endif /* !__SCAN_H_ */
