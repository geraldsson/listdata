#include "listdata.h"

typedef listdata_type object;

/* atoms */
#define JSON_TRUE  3
#define JSON_FALSE 4

object json_parse_start(const char *);
object json_parse(const char *, object);
int json_parse_done(object);
