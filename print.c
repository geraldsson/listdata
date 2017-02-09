#include <stdio.h>
#include <ctype.h>
#include "listdata.h"

#define NUM_NAMES 3
static const char *names[NUM_NAMES] = {"null", "()", "{}"};

void print(Q x)
{
	switch (type_of(x)) {
	case TYP_CONS:
		putchar('(');
		print(get_head(x));
		while (is_cons(x = get_tail(x))) {
			putchar(' ');
			print(get_head(x));
		}
		if (x != EMPTY_LIST) {
			printf(" . ");
			print(x);
		}
		putchar(')');
		break;
	case TYP_STR:
		printf("\"%s\"", load_str(x));
		break;
	case TYP_INT:
		printf("%d", load_int(x));
		break;
	case TYP_ATOM:
		if (x < NUM_NAMES)
			printf(names[x]);
		else if (x >= 0x20 && x < 0x7F)
			printf("'%c'", x);
		else
			printf("0x%X", x);
	}
}
