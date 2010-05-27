/* Dynamically growing memory pools */

typedef char str_cell[4];
typedef unsigned cons_cell[2];

struct str_stack {
	unsigned top, end;
	str_cell *mem;
};

struct cons_stack {
	unsigned top, end;
	cons_cell *mem;
};

unsigned push_str (struct str_stack *, const char *);
unsigned push_cons(struct cons_stack *, unsigned head, unsigned tail);

/* restore stack pointer: the next push will use memory at p */
void restore_str (struct str_stack *, unsigned p);
void restore_cons(struct cons_stack *, unsigned p);
