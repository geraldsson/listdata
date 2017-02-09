/* Dynamically growing memory pool */

#ifndef mstack_h
#define mstack_h

struct mblock {
	int freeable;
	void *mem;
};

struct mstack {
	unsigned top, end, limit;
	struct mblock *mblocks, mblocks_static[8];
};

void mstack_init(struct mstack *);

/* push memory block */
unsigned mstack_push(struct mstack *, void *mem);

/* allocate and push freeable memory block of n bytes */
unsigned mstack_alloc(struct mstack *, unsigned n);

/* free mblocks[p] and everything on top of it */
void mstack_free(struct mstack *, unsigned p);

#endif
