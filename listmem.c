/* Dynamically growing memory pools
 * for strings and conses (as in Lisp)
 *
 * 2010-05-25 VN
 */
#include <stdlib.h>
#include "listmem.h"

#define STACK_BOTTOM 1
#define STACK_END_MIN 256

static unsigned grow(unsigned end)
{
	return end<<1 > end ? end<<1 : 0;
}

static str_cell *realloc_str(str_cell *mem, unsigned end)
{
	return end ? realloc(mem, end*sizeof(str_cell)) : NULL;
}

static cons_cell *realloc_cons(cons_cell *mem, unsigned end)
{
	return end ? realloc(mem, end*sizeof(cons_cell)) : NULL;
}

static int copy_str(const char *s, str_cell t)
{
	int i;
	for (i=0; i<sizeof(str_cell); i++) {
		if (!(t[i] = s[i]))
			return 1;
	}
	return 0;
}

unsigned push_str(struct str_stack *stack, const char *s)
{
	unsigned top = stack->top,
		 end = stack->end,
		 str;
	str_cell *mem = stack->mem;

	if (!mem) {
		top = STACK_BOTTOM;
		end = STACK_END_MIN;
		mem = malloc(end*sizeof(str_cell));
	}
	str = top;
	while (mem) {
		stack->end = end;
		stack->mem = mem;
		for (; top < end; top++) {
			if (copy_str(s, mem[top])) {
				stack->top = top+1;
				return str;
			}
			s += sizeof(str_cell);
		}
		end = grow(end);
		mem = realloc_str(mem, end);
	}
	return 0;
}

unsigned push_cons(struct cons_stack *stack, Q head, Q tail)
{
	unsigned top = stack->top,
		 end = stack->end;
	cons_cell *mem = stack->mem;

	if (!mem) {
		top = STACK_BOTTOM;
		end = STACK_END_MIN;
		mem = malloc(end*sizeof(cons_cell));
	} else if (top >= end) {
		end = grow(end);
		mem = realloc_cons(mem, end);
	}
	if (!mem)
		return 0;

	stack->top = top+1;
	stack->end = end;
	stack->mem = mem;

	mem[top][0] = head;
	mem[top][1] = tail;

	return top;
}

void restore_str(struct str_stack *stack, unsigned p)
{
	unsigned top = stack->top,
		 end = stack->end;
	str_cell *mem = stack->mem;
	if (mem) {
		if (top < end && top >= STACK_END_MIN) {
			mem = realloc_str(mem, top);
			if (mem) {
				stack->end = top;
				stack->mem = mem;
			}
		}
		stack->top = p > STACK_BOTTOM ? p : STACK_BOTTOM;
	}
}

void restore_cons(struct cons_stack *stack, unsigned p)
{
	unsigned top = stack->top,
		 end = stack->end;
	cons_cell *mem = stack->mem;
	if (mem) {
		if (top < end && top >= STACK_END_MIN) {
			mem = realloc_cons(mem, top);
			if (mem) {
				stack->end = top;
				stack->mem = mem;
			}
		}
		stack->top = p > STACK_BOTTOM ? p : STACK_BOTTOM;
	}
}
