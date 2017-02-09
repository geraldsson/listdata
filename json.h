#include "listdata.h"

#define JSON_TRUE  3
#define JSON_FALSE 4

Q json_parse_start(const char *);
Q json_parse(const char *, Q);
int json_parse_done(Q);
