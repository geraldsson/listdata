/* cons pointer or non-pointer data */
typedef unsigned obj_T;

/* special atoms */
#define NULL_OBJECT  0x100
#define EMPTY_STRING 0x101
#define EMPTY_OBJECT 0x102
#define TRUE_OBJECT  0x103
#define FALSE_OBJECT 0x104

/* true if x is a proper list */
int is_list(obj_T x);

obj_T new_string(const char *);
obj_T prefix_string(const char *prefix, obj_T str);

void free_listdata();
