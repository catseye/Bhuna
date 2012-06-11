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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mem.h"

#include "dict.h"
#include "value.h"

/*** CONSTRUCTOR ***/

/*
 * Create a new dictionary.
 */
struct dict *
dict_new(void)
{
	struct dict *d;
	int i;

	d = bhuna_malloc(sizeof(struct dict));
	d->num_buckets = 31;
	d->bucket = bhuna_malloc(sizeof(struct chain *) * d->num_buckets);
	for (i = 0; i < d->num_buckets; i++) {
		d->bucket[i] = NULL;
	}
	d->cursor = NULL;
	d->cur_bucket = 0;

	return(d);
}

/*** DESTRUCTORS ***/

static void
chain_free(struct chain *c)
{
	assert(c != NULL);

	value_release(c->key);
	value_release(c->value);

	bhuna_free(c);
}

void
dict_free(struct dict *d)
{
	struct chain *c;
	size_t bucket_no;

	for (bucket_no = 0; bucket_no < d->num_buckets; bucket_no++) {
		c = d->bucket[bucket_no];
		while (c != NULL) {
			d->bucket[bucket_no] = c->next;
			chain_free(c);
			c = d->bucket[bucket_no];
		}
	}
	bhuna_free(d);
}

/*** UTILITIES ***/

/*
 * Hash function, taken from "Compilers: Principles, Techniques, and Tools"
 * by Aho, Sethi, & Ullman (a.k.a. "The Dragon Book", 2nd edition.)
 */
static size_t
hashpjw(struct value *key, size_t table_size) {
	char *p;
	unsigned long int h = 0, g;

	/*
	 * XXX ecks ecks ecks XXX
	 * This is naff... for certain values this will work.
	 * For others, it won't...
	 */
	for (p = (char *)key; p - (char *)key < sizeof(struct value); p++) {
		h = (h << 4) + (*p);
		if ((g = h & 0xf0000000))
			h = (h ^ (g >> 24)) ^ g;
	}
	
	return(h % table_size);
}

/*
 * Create a new bucket (not called directly by client code.)
 */
static struct chain *
chain_new(struct value *key, struct value *value)
{
	struct chain *c;

	c = bhuna_malloc(sizeof(struct chain));

	c->next = NULL;
	/* XXX grab? */
	c->key = key;
	c->value = value;

	return(c);
}

/*
 * Locate the bucket number a particular key would be located in, and the
 * chain link itself if such a key exists (or NULL if it could not be found.)
 */
static void
dict_locate(struct dict *d, struct value *key,
	    size_t *b_index, struct chain **c)
{
	*b_index = hashpjw(key, d->num_buckets);
	for (*c = d->bucket[*b_index]; *c != NULL; *c = (*c)->next) {
		if (value_equal(key, (*c)->key))
			break;
	}
}

/*** OPERATIONS ***/

/*
 * Retrieve a value from a dictionary, given its key.
 */
struct value *
dict_fetch(struct dict *d, struct value *k)
{
	struct chain *c;
	size_t i;

	dict_locate(d, k, &i, &c);
	if (c != NULL) {
		/* XXX grab? */
		return(c->value);
	} else {
		return(NULL);
	}
}

/*
 * Insert a value into a dictionary.
 */
void
dict_store(struct dict *d, struct value *k, struct value *v)
{
	struct chain *c;
	size_t i;

	dict_locate(d, k, &i, &c);
	if (c == NULL) {
		/* Chain does not exist, add a new one. */
		c = chain_new(k, v);
		c->next = d->bucket[i];
		d->bucket[i] = c;
	} else {
		/* Chain already exists, replace the value. */
		value_release(c->value);
		c->value = v;
	}
}

int
dict_exists(struct dict *d, struct value *key)
{
	struct value *v;

	v = dict_fetch(d, key);
	return(v != NULL);
}

/*
 * Finds the next bucket with data in it.
 * If d->cursor == NULL after this, there is no more data.
 */
static void
dict_advance(struct dict *d)
{
	while (d->cursor == NULL) {
		if (d->cur_bucket == d->num_buckets - 1) {
			/* We're at eof.  Do nothing. */
			break;
		} else {
			d->cur_bucket++;
			d->cursor = d->bucket[d->cur_bucket];
		}
	}
}

void
dict_rewind(struct dict *d)
{
	d->cur_bucket = 0;
	d->cursor = d->bucket[d->cur_bucket];
	dict_advance(d);
}

int
dict_eof(struct dict *d)
{
	return(d->cursor == NULL);
}

struct value *
dict_getkey(struct dict *d)
{
	if (d->cursor == NULL) {
		return(NULL);
	} else {
		/* XXX grab? */
		return(d->cursor->key);
	}
}

void
dict_next(struct dict *d)
{
	if (d->cursor != NULL)
		d->cursor = d->cursor->next;
	dict_advance(d);
}

size_t
dict_size(struct dict *d)
{
	struct chain *c;
	int bucket_no;
	size_t count = 0;

	for (bucket_no = 0; bucket_no < d->num_buckets; bucket_no++) {
		for (c = d->bucket[bucket_no]; c != NULL; c = c->next)
			count++;
	}

	return(count);
}

/*** debugging ***/

void
dict_dump(struct dict *d)
{
#ifdef DEBUG
	printf("dict[%08lx]", (unsigned long)d);
#endif
}
