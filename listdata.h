#ifndef listdata_h
#define listdata_h

/* tagged pointer or immediate data */
typedef unsigned Q;

typedef Q mpoint[3];

/* mark the allocation state (save stack pointers) */
void listdata_mark(Q *p);

/* release all memory allocated since the mark */
void listdata_release(const Q *p);

/* Push data objects */

Q store_str(const char *);
Q store_int(int);
Q cons(Q head, Q tail);
Q cons_nil(Q head);		/* same as cons(x, EMPTY_LIST) */

/* special atoms   0 null */
#define EMPTY_LIST 1		/* [] JSON array */
#define EMPTY_DICT 2		/* {} JSON object */

/* Atoms use 0-1023 */

enum typ {
	TYP_CONS,
	TYP_STR,
	TYP_INT,
	TYP_ATOM
};
enum typ type_of(Q);

char *load_str(Q);
int   load_int(Q);
Q    *load_cons(Q);

Q get_head(Q);		/* load_cons(x)[0] */
Q get_tail(Q);		/* load_cons(x)[1] */

int is_cons(Q);

/* follow tail n times (or return 0 if end was reached) */
Q nth_tail(Q, int n);

/* pointer to the nth element (counting from 0) or NULL */
Q *nth_elem(Q, int n);
Q *first(Q);
Q *second(Q);
Q *third(Q);

/* last n conses or the terminating object if n=0 */
Q last_tail(Q, int n);

Q pop(Q *);

/* return reversed list (destructive) */
Q reverse_list(Q);

int equals_str(Q, const char *);
int equals(Q, Q);

Q *dict_get(Q dict, Q key);
Q  dict_set(Q dict, Q key, Q val);

/* Consed string manipulation */

/* copy string to buf of size n.
   return length of stored string */
int copy_str(Q, char *buf, int n);

/* split string by delimeter char (destructive) */
Q split(Q, int sep);

/* immutable cons concatenation (of strings) */
Q concat(Q, Q);

#endif
