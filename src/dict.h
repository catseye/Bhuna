/*
 * Copyright (c) 2004 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Chris Pressey <cpressey@catseye.mine.nu>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
