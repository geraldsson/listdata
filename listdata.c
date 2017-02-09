/* Representation and storage of simple data objects
 * based on lists as in LISP and stacks.
 * 2010-07-07 VN
 */
#include <stdlib.h>
#include <string.h>
#include "listdata.h"
#include "mstack.h"

#define T listdata_type

typedef T cons_cell[2];

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MAX3(a,b,c) MAX(a,MAX(b,c))

/* T structure (base tag offset)
		msb           0  */

#define OFFSET_BITS 10
#define OFFSET_NUM ((T)1 << OFFSET_BITS)
#define OFFSET_MAX (OFFSET_NUM - 1)
#define OFFSET(x) ((x) & OFFSET_MAX)
#define TAGGED(x) ((T)(x) << OFFSET_BITS)
#define TAG_BITS  3
#define TAG_MASK  TAGGED((1 << TAG_BITS) - 1)
#define BASE_BIT  (OFFSET_BITS + TAG_BITS)
#define BASE(x)   ((x) >> BASE_BIT)
#define BASE_MASK (~(TAG_MASK | OFFSET_MAX))
#define BASE_MAX  BASE(BASE_MASK)

#define INUM_MAX ((T)(-1) >> (TAG_BITS+1))
#define MSB    (~((T)(-1) >> 1))

enum ttag {
	TAG_ATOM,	/* must by zero */
	TAG_CONS,
	TAG_STR,
	TAG_INT,
	TAG_INUM	/* immediate integer */
};

static struct mstack mstack;

static T str_top, int_top, cons_top;
static char	 *str_p;
static int	 *int_p;
static cons_cell *cons_p;

static void *getmem(T pointer)
{
	return mstack.mblocks[BASE(pointer)].mem;
}

void listdata_mark(T *p)
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

void listdata_release(const T *p)
{
	T x = MAX3(p[0], p[1], p[2]),
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

static T tag_base(enum ttag tag, unsigned b)
{
	return (tag | ((T) b << TAG_BITS)) << OFFSET_BITS;
}

T store_str(const char *s)
{
	unsigned b;
	T str;
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

static T push_int(int x)
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

T store_int(int x)
{
	return abs(x) > INUM_MAX ? push_int(x) :
		TAGGED(TAG_INUM) | (((T) x << TAG_BITS) & BASE_MASK)
				 | ((T) x & (OFFSET_MAX | MSB));
}

static int extract_inum(T x)
{
	return (x & MSB) ? extract_inum(x ^ MSB) - (int)(INUM_MAX+1) :
		((x & BASE_MASK) >> TAG_BITS) | OFFSET(x);
}

T cons(T head, T tail)
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

T cons_nil(T head)
{
	return cons(head, EMPTY_LIST);
}

enum typ type_of(T x)
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

static int tagged(T x, enum ttag tag)
{
	return (x & TAG_MASK) == TAGGED(tag);
}

char *load_str(T str)
{
	return (char *) getmem(str) + OFFSET(str);
}

int load_int(T x)
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

T *load_cons(T cons)
{
	return &((cons_cell *) getmem(cons))[OFFSET(cons)][0];
}

T get_head(T cons) { return load_cons(cons)[0]; }
T get_tail(T cons) { return load_cons(cons)[1]; }

int is_cons(T x)
{
	return tagged(x, TAG_CONS);
}

T nth_tail(T x, int n)
{
	for (; n>0 && is_cons(x); n--)
		x = get_tail(x);
	return !n ? x : 0;
}

T *nth_elem(T x, int n)
{
	x = nth_tail(x, n);
	return is_cons(x) ? load_cons(x) : NULL;
}

T *first (T x) { return nth_elem(x, 0); }
T *second(T x) { return nth_elem(x, 1); }
T *third (T x) { return nth_elem(x, 2); }

T last_tail(T x, int n)
{
	return is_cons(nth_tail(x, n)) ? last_tail(get_tail(x), n) : x;
}

T pop(T *p)
{
	T elem = 0;
	if (is_cons(*p)) {
		elem = get_head(*p);
		*p = get_tail(*p);
	}
	return elem;
}

T reverse_list(T list)
{
	T tail = EMPTY_LIST,
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

static const char *match_prefix(const char *s, T prefix)
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

int equals_str(T x, const char *s)
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

static char *str_begin(T x, T *p, T *q)
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

static char *str_next(char *s, T *p, T *q)
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

int equals(T x, T y)
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

T *dict_get(T x, T key)
{
	T *p = nth_elem(x, 1);
	return !p || equals(key, get_head(x)) ? p : dict_get(p[1], key);
}

T dict_set(T x, T key, T val)
{
	return cons(key, cons(val, x));
}

int copy_str(T x, char *buf, int n)
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

T split(T x, int sep)
{
	T y, z, *p;
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

T concat(T x, T y)
{
	if (is_cons(x)) {
		y = concat(get_tail(x), y);
		x = get_head(x);
	}
	return cons(x, y);
}
