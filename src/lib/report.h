/*
 * report.h
 * Error/warning reporter for Bhuna.
 * $Id$
 */

#ifndef __REPORT_H_
#define __REPORT_H_

#include <stdio.h>
#include <wchar.h>

struct scan_st;

#define REPORT_ERROR	1
#define REPORT_WARNING	0

extern void	report_start(void);
extern int	report_finish(void);

extern void	report(int, struct scan_st *, char *, ...);

#endif /* !__SCAN_H_ */
