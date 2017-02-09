/* Dynamically growing memory pool
 * 2010-07-21 VN
 */
#include <stdlib.h>
#include <limits.h>
#include "mstack.h"

#define NUM_OF(a) (sizeof(a) / sizeof(a[0]))

void mstack_init(struct mstack *m)
{
	m->top = 0;
	m->end = NUM_OF(m->mblocks_static);
	m->limit = UINT_MAX / sizeof(struct mblock);
	m->mblocks = m->mblocks_static;
	m->mblocks[0].mem = NULL;
}

unsigned mstack_push(struct mstack *m, void *mem)
{
	unsigned top = m->top + 1,
		 end = m->end;
	struct mblock *bs = m->mblocks;
	int i = 0,
	    n = NUM_OF(m->mblocks_static);

	if (top >= end) {
		end <<= 1;
		if (end <= top || end > m->limit)
			return 0;
		if (bs == m->mblocks_static)
			bs = malloc(end * sizeof(struct mblock));
		else {
			bs = realloc(bs, end * sizeof(struct mblock));
			i = n;
		}
		if (!bs)
			return 0;

		for (; i < n; i++)
			bs[i] = m->mblocks_static[i];

		m->end = end;
		m->mblocks = bs;
	}
	m->top = top;

	bs[top].freeable = 0;
	bs[top].mem = mem;

	return top;
}

unsigned mstack_alloc(struct mstack *m, unsigned n)
{
	void    *mem = malloc(n);
	unsigned top = 0;
	if (mem) {
		top = mstack_push(m, mem);
		if (top)
			m->mblocks[top].freeable = 1;
		else
			free(mem);
	}
	return top;
}

void mstack_free(struct mstack *m, unsigned p)
{
	unsigned top = m->top;
	struct mblock *bs = m->mblocks;

	for (; top && top >= p; top--) {
		if (bs[top].freeable)
			free(bs[top].mem);
	}
	m->top = top;

	if (top < NUM_OF(m->mblocks_static) && bs != m->mblocks_static) {
		for (; top; top--)
			m->mblocks_static[top] = bs[top];
		free(bs);
		m->end = NUM_OF(m->mblocks_static);
		m->mblocks = m->mblocks_static;
	}
}
