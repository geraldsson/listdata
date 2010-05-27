#include <stdlib.h>
#include "listdata.h"
#include "listmem.h"

#define STR_MEM  (listdata.str.mem)
#define CONS_MEM (listdata.cons.mem)

struct listdata {
	struct str_stack str;
	struct cons_stack cons;
};

static struct listdata listdata;

static unsigned pushstr(const char *s)
{
	return push_str(&listdata.str, s);
}

static unsigned pushcons(unsigned head, unsigned tail)
{
	unsigned x = push_cons(&listdata.cons, head, tail);
	return ~x > x ? x : 0;
}

obj_T cons_onto(obj_T head)
{
}

static obj_T cons_str(const char *head, obj_T tail)
{
}

new_cons(unsigned head, obj_T tail)
{
	obj_T x = pushcons(head, tail);
	return x ? ~x : 0;
}

static int iscons(obj_T x)
{
	return ~x < x;
}

static obj_T gettail(obj_T cons)
{
	return CONS_MEM[~cons][1];
}

int is_list(obj_T x)
{
	return x == EMPTY_LIST || (iscons(x) && is_list(gettail(x)));
}

static int isasciiprint(int c)
{
	return c >= 32 && c < 127;
}

static int ischardata(obj_T x)
{
	char *s = (char *) &x;
	int n = sizeof(obj_T);
	if (*s) {
		while (!s[n-1])
			n--;
		if (n < sizeof(obj_T)) {
			while (n>0 && isasciiprint(s[n-1]))
				n--;
			return !n;
		}
	}
	return 0;
}

int is_string(obj_T x)
{
	return x == EMPTY_STRING ||
		(iscons(x) ? is_string(gettail(x)) : ischardata(x));
}

obj_T new_string(const char *s)
{
	obj_T str = 0;
	int i;
	if (*s) {
		for (i=0; i<sizeof(obj_T); i++)
			((char *)&str)[i] = s[i];
		if (!ischardata(str))
			return pushcons(pushstr(s), EMPTY_STRING);
	}

	return prefix_string(s, EMPTY_STRING);
}

static obj_T

static obj_T prefix_str(obj_T prefix, obj_T str)
{
	return ~str == EMPTY_STRING ? prefix : pushcons(prefix, str);
}

obj_T prefix_string(const char *s, obj_T str)
{
	obj_T c = (unsigned char) *s;
	if (*s) {
		if (!s[1] && c >= 0x20 && c < 0x7f)
			return prefix_str(~(c << 8), str);
		else
			return prefix_str(pushstr(s), str);
	}
	return str;
}

void free_listdata()
{
	if (STR_MEM) {
		free(STR_MEM);
		STR_MEM = NULL;
	}
	if (CONS_MEM) {
		free(CONS_MEM);
		CONS_MEM = NULL;
	}
}
