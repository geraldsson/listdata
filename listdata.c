/* Representation and storage of simple data objects
 * based on lists as in LISP and stacks.
 * 2010-07-07 VN
 */
#include <stdlib.h>
#include <string.h>
#include "listdata.h"
#include "mstack.h"

typedef Q cons_cell[2];

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MAX3(a,b,c) MAX(a,MAX(b,c))

/* Q structure (base tag offset)
		msb           0  */

#define OFFSET_BITS 10
#define OFFSET_NUM ((Q)1 << OFFSET_BITS)
#define OFFSET_MAX (OFFSET_NUM - 1)
#define OFFSET(x) ((x) & OFFSET_MAX)
#define TAGGED(x) ((Q)(x) << OFFSET_BITS)
#define TAG_BITS  3
#define TAG_MASK  TAGGED((1 << TAG_BITS) - 1)
#define BASE_BIT  (OFFSET_BITS + TAG_BITS)
#define BASE(x)   ((x) >> BASE_BIT)
#define BASE_MASK (~(TAG_MASK | OFFSET_MAX))
#define BASE_MAX  BASE(BASE_MASK)

#define INUM_MAX ((Q)(-1) >> (TAG_BITS+1))
#define MSB    (~((Q)(-1) >> 1))

enum ttag {
	TAG_ATOM,	/* must by zero */
	TAG_CONS,
	TAG_STR,
	TAG_INT,
	TAG_INUM	/* immediate integer */
};

static struct mstack mstack;

static Q str_top, int_top, cons_top;
static char	 *str_p;
static int	 *int_p;
static cons_cell *cons_p;

static void *getmem(Q pointer)
{
	return mstack.mblocks[BASE(pointer)].mem;
}

void listdata_mark(Q *p)
{
	if (!mstack.mblocks) {
		mstack_init(&mstack);
		if (BASE_MAX < mstack.limit)
			mstack.limit = BASE_MAX;
	}
	p[0] = str_top;
	p[1] = int_top;
	p[2] = cons_top;
}

void listdata_release(const Q *p)
{
	Q x = MAX3(p[0], p[1], p[2]),
	  y = MAX3(str_top, int_top, cons_top);
	if (!x)
		mstack_free(&mstack, 0);
	else if (BASE(x) < BASE(y))
		mstack_free(&mstack, BASE(y));

	str_top  = p[0];
	int_top  = p[1];
	cons_top = p[2];

	str_p = load_str(str_top);
	int_p = (int *) getmem(int_top) + OFFSET(int_top);
	cons_p = (cons_cell *) load_cons(cons_top);
}

static Q tag_base(enum ttag tag, unsigned b)
{
	return (tag | ((Q) b << TAG_BITS)) << OFFSET_BITS;
}

Q store_str(const char *s)
{
	unsigned b;
	Q str;
	if (str_p && OFFSET(str_top)+1 < OFFSET_MAX) {
		str_top++;
		str_p++;
	} else {
		b = mstack_alloc(&mstack, OFFSET_NUM);
		if (!b)
			return 0;
		str_top = tag_base(TAG_STR, b);
		str_p = mstack.mblocks[b].mem;
	}
	str = str_top;
	while (OFFSET(str_top) < OFFSET_MAX && (*str_p = *s)) {
		str_top++;
		str_p++;
		s++;
	}
	*str_p = '\0';
	return *s ? cons(str, store_str(s)) : str;
}

static Q push_int(int x)
{
	unsigned b;
	if (int_p && OFFSET(int_top) < OFFSET_MAX) {
		int_top++;
		int_p++;
	} else {
		b = mstack_alloc(&mstack, OFFSET_NUM * sizeof(int));
		if (!b)
			return 0;
		int_top = tag_base(TAG_INT, b);
		int_p = mstack.mblocks[b].mem;
	}
	*int_p = x;
	return int_top;
}

Q store_int(int x)
{
	return abs(x) > INUM_MAX ? push_int(x) :
		TAGGED(TAG_INUM) | (((Q) x << TAG_BITS) & BASE_MASK)
				 | ((Q) x & (OFFSET_MAX | MSB));
}

static int extract_inum(Q x)
{
	return (x & MSB) ? extract_inum(x ^ MSB) - (int)(INUM_MAX+1) :
		((x & BASE_MASK) >> TAG_BITS) | OFFSET(x);
}

Q cons(Q head, Q tail)
{
	unsigned b;
	if (cons_p && OFFSET(cons_top) < OFFSET_MAX) {
		cons_top++;
		cons_p++;
	} else {
		b = mstack_alloc(&mstack, OFFSET_NUM * sizeof(cons_cell));
		if (!b)
			return 0;
		cons_top = tag_base(TAG_CONS, b);
		cons_p = mstack.mblocks[b].mem;
	}
	(*cons_p)[0] = head;
	(*cons_p)[1] = tail;
	return cons_top;
}

Q cons_nil(Q head)
{
	return cons(head, EMPTY_LIST);
}

enum typ type_of(Q x)
{
	switch (x & TAG_MASK) {
	case TAGGED(TAG_CONS):
		return TYP_CONS;
	case TAGGED(TAG_STR):
		return TYP_STR;
	case TAGGED(TAG_INT):
	case TAGGED(TAG_INUM):
		return TYP_INT;
	default:
		return TYP_ATOM;
	}
}

static int tagged(Q x, enum ttag tag)
{
	return (x & TAG_MASK) == TAGGED(tag);
}

char *load_str(Q str)
{
	return (char *) getmem(str) + OFFSET(str);
}

int load_int(Q x)
{
	switch (x & TAG_MASK) {
	case TAGGED(TAG_INT):
		return ((int *) getmem(x))[OFFSET(x)];
	case TAGGED(TAG_INUM):
		return extract_inum(x);
	default:
		return 0;
	}
}

Q *load_cons(Q cons)
{
	return &((cons_cell *) getmem(cons))[OFFSET(cons)][0];
}

Q get_head(Q cons) { return load_cons(cons)[0]; }
Q get_tail(Q cons) { return load_cons(cons)[1]; }

int is_cons(Q x)
{
	return tagged(x, TAG_CONS);
}

Q nth_tail(Q x, int n)
{
	for (; n>0 && is_cons(x); n--)
		x = get_tail(x);
	return !n ? x : 0;
}

Q *nth_elem(Q x, int n)
{
	x = nth_tail(x, n);
	return is_cons(x) ? load_cons(x) : NULL;
}

Q *first (Q x) { return nth_elem(x, 0); }
Q *second(Q x) { return nth_elem(x, 1); }
Q *third (Q x) { return nth_elem(x, 2); }

Q last_tail(Q x, int n)
{
	return is_cons(nth_tail(x, n)) ? last_tail(get_tail(x), n) : x;
}

Q pop(Q *p)
{
	Q elem = 0;
	if (is_cons(*p)) {
		elem = get_head(*p);
		*p = get_tail(*p);
	}
	return elem;
}

Q reverse_list(Q list)
{
	Q tail = EMPTY_LIST,
	  next, *p;
	while (is_cons(list)) {
		p = load_cons(list);
		next = p[1];
		p[1] = tail;
		tail = list;
		list = next;
	}
	return tail;
}

static const char *match_prefix(const char *s, Q prefix)
{
	const char *t;
	if (!tagged(prefix, TAG_STR))
		return NULL;
	t = load_str(prefix);
	for (; *t; s++, t++) {
		if (*s != *t)
			return NULL;
	}
	return s;
}

int equals_str(Q x, const char *s)
{
	for (; is_cons(x); x = get_tail(x)) {
		s = match_prefix(s, get_head(x));
		if (!s)
			return 0;
	}
	s = match_prefix(s, x);
	return s && !*s;
}

/* Iterate over consed strings:
   for (s=str_begin(x, &x, NULL); s; s=str_next(s, &x, NULL)) ... */

static char *str_begin(Q x, Q *p, Q *q)
{
	char *s;
	*p = x;
	if (is_cons(x))
		x = get_head(x);
	if (q)
		*q = x;
	if (tagged(x, TAG_STR)) {
		if (*(s = load_str(x)))
			return s;
		if (is_cons(*p))
			return str_begin(get_tail(*p), p, q);
	}
	return NULL;
}

static char *str_next(char *s, Q *p, Q *q)
{
	if (s[1]) {
		if (q)
			(*q)++;
		return s+1;
	}
	if (is_cons(*p))
		return str_begin(get_tail(*p), p, q);
	return NULL;
}

int equals(Q x, Q y)
{
	char *s, *t;
	if (x == y)
		return 1;
	if (tagged(x, TAG_STR))
		return equals_str(y, load_str(x));
	if (tagged(y, TAG_STR))
		return equals_str(x, load_str(y));
	if (tagged(x, TAG_INT))
		return load_int(x) == load_int(y);
	if (is_cons(x) && is_cons(y)) {
		if (equals(get_head(x), get_head(y)))
			return equals(get_tail(x), get_tail(y));
		if (tagged(get_head(x), TAG_STR)) {
			s = str_begin(x, &x, NULL);
			t = str_begin(y, &y, NULL);
			while (s && t) {
				if (*s != *t)
					return 0;
				s = str_next(s, &x, NULL);
				t = str_next(t, &y, NULL);
			}
			return !s && !t && tagged(x, TAG_STR) &&
					   tagged(y, TAG_STR);
		}
	}
	return 0;
}

Q *dict_get(Q x, Q key)
{
	Q *p = nth_elem(x, 1);
	return !p || equals(key, get_head(x)) ? p : dict_get(p[1], key);
}

Q dict_set(Q x, Q key, Q val)
{
	return cons(key, cons(val, x));
}

int copy_str(Q x, char *buf, int n)
{
	char *s = str_begin(x, &x, NULL);
	int i;
	for (i=0; i+1 < n && s; i++) {
		buf[i] = *s;
		s = str_next(s, &x, NULL);
	}
	buf[i] = '\0';
	return i;
}

Q split(Q x, int sep)
{
	Q y, z, *p;
	char *s = str_begin(x, &y, &z);
	for (; s; s = str_next(s, &y, &z)) {
		if (*s == sep) {
			*s = '\0';
			z++;
			if (is_cons(y)) {
				z = cons(z, get_tail(y));
				p = &x;
				while (*p != y)
					p = load_cons(*p)+1;
				*p = get_head(y);
			}
			return cons(x, split(z, sep));
		}
	}
	return cons_nil(x);
}

Q concat(Q x, Q y)
{
	if (is_cons(x)) {
		y = concat(get_tail(x), y);
		x = get_head(x);
	}
	return cons(x, y);
}
