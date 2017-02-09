#ifndef listdata_h
#define listdata_h

/* tagged pointer or immediate data */
typedef unsigned listdata_type;

#define T listdata_type

typedef T mpoint[3];

/* mark the allocation state (save stack pointers) */
void listdata_mark(T *p);

/* release all memory allocated since the mark */
void listdata_release(const T *p);

/* Push data objects */

T store_str(const char *);
T store_int(int);
T cons(T head, T tail);
T cons_nil(T head);		/* same as cons(x, EMPTY_LIST) */

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
enum typ type_of(T);

char *load_str(T);
int   load_int(T);
T    *load_cons(T);

T get_head(T);		/* load_cons(x)[0] */
T get_tail(T);		/* load_cons(x)[1] */

int is_cons(T);

/* follow tail n times (or return 0 if end was reached) */
T nth_tail(T, int n);

/* pointer to the nth element (counting from 0) or NULL */
T *nth_elem(T, int n);
T *first(T);
T *second(T);
T *third(T);

/* last n conses or the terminating object if n=0 */
T last_tail(T, int n);

T pop(T *);

/* return reversed list (destructive) */
T reverse_list(T);

int equals_str(T, const char *);
int equals(T, T);

T *dict_get(T dict, T key);
T  dict_set(T dict, T key, T val);

/* Consed string manipulation */

/* copy string to buf of size n.
   return length of stored string */
int copy_str(T, char *buf, int n);

/* split string by delimeter char (destructive) */
T split(T, int sep);

/* immutable cons concatenation (of strings) */
T concat(T, T);

#undef T

#endif
