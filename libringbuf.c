/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2015 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Derived from FreeBSD's bufring.c
 *
 **************************************************************************
 *
 * Copyright (c) 2007,2008 Kip Macy kmacy@freebsd.org
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. The name of Kip Macy nor the names of other
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>

#include "libringbuf.h"

/* true if x is a power of 2 */
#define POWEROF2(x) ((((x)-1) & (x)) == 0)
#define RING_SZ_MASK  (unsigned)(0x0fffffff) /**< Ring size mask */
#define ALIGN_FLOOR(val, align) \
	(typeof(val))((val) & (~((typeof(val))((align) - 1))))

/* return the size of memory occupied by a ring */
ssize_t
libringbuf_get_memsize(unsigned count)
{
	ssize_t sz;

	/* count must be a power of 2 */
	if ((!POWEROF2(count)) || (count > RING_SZ_MASK )) {
		printf("Requested size is invalid, must be power of 2, and "
			"do not exceed the size limit %u\n", RING_SZ_MASK);
		return -1;
	}

	sz = sizeof(struct ringbuf) + count * sizeof(void *);
	sz = ALIGN_FLOOR(sz, CACHE_LINE_SIZE);
	return sz;
}

int
libringbuf_init(struct ringbuf *r, unsigned count,
	unsigned flags)
{
	/* init the ring structure */
	memset(r, 0, sizeof(*r));
	r->flags = flags;
	r->prod.watermark = count;
	r->prod.sp_enqueue = !!(flags & RING_F_SP_ENQ);
	r->cons.sc_dequeue = !!(flags & RING_F_SC_DEQ);
	r->prod.size = r->cons.size = count;
	r->prod.mask = r->cons.mask = count-1;
	r->prod.head = r->cons.head = 0;
	r->prod.tail = r->cons.tail = 0;

	return 0;
}

/* create the ring */
struct ringbuf *
libringbuf_create(unsigned count, unsigned flags)
{
	struct ringbuf *r;
	ssize_t ring_size;

	ring_size = libringbuf_get_memsize(count);
	if (ring_size < 0) {
		return NULL;
	}

	r = (struct ringbuf *)malloc(ring_size);
	if (r != NULL) {
		libringbuf_init(r, count, flags);
	} else {
		r = NULL;
		printf("ringbuf Cannot reserve memory\n");
	}

	return r;
}

/* free the ring */
void
libringbuf_free(struct ringbuf *r)
{
	if (r == NULL)
		return;
	free(r);
}

/*
 * change the high water mark. If *count* is 0, water marking is
 * disabled
 */
int
libringbuf_set_water_mark(struct ringbuf *r, unsigned count)
{
	if (count >= r->prod.size)
		return -EINVAL;

	/* if count is 0, disable the watermarking */
	if (count == 0)
		count = r->prod.size;

	r->prod.watermark = count;
	return 0;
}

