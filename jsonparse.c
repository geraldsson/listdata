/* JSON parser by Victor Nilsson.
 *
 *  Numbers are stored with possibly fewer digits and an exponent number
 *  if needed, using an int or a cons of ints.
 *
 *  Strings are C strings (Latin-1), consed strings or lists terminated
 *  by a string, with ints for characters above U+00FF.
 *
 * 2010-07-21
 */
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include "json.h"

#define LIT_NAME_0 8
#define LIT_NAME_T LIT_NAME_0
#define LIT_NAME_F (LIT_NAME_0 + 5)
#define LIT_NAME_N (LIT_NAME_0 + 11)

static const char lit_names[] = "true\0false\0null";
static const unsigned char lit_ok[] = {0,0,0,1,0,0,0,0,0,2,0,0,0,0,3};
static const unsigned char lit_val[] = {0, JSON_TRUE, JSON_FALSE, 0};

static object parse_value(const char *, const char **, object);

static int is_json_atom(object x)
{
	switch (x) {
	case 0:			/* null  */
	case EMPTY_LIST:	/* []    */
	case EMPTY_DICT:	/* {}    */
	case JSON_TRUE:		/* true  */
	case JSON_FALSE:	/* false */
		return 1;
	default:
		return 0;
	}
}

/* parser state symbol */
static int is_state_atom(object x)
{
	return x < 0x100 && !is_json_atom(x);
}

static const char *skip_ws(const char *s)
{
	switch (*s) {
	case ' ':
	case '\t':
	case '\n':
	case '\r':
		return skip_ws(s+1);
	default:
		return s;
	}
}

static int match_digits(const char *s)
{
	switch (*s) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return 1 + match_digits(s+1);
	default:
		return 0;
	}
}

static int convert_digits(const char *s, int *n, int i)
{
	while (*n>0 && i <= INT_MAX/10 && i*10 <= INT_MAX - (*s-'0')) {
		i = i*10 + (*s-'0');
		s++;
		(*n)--;
	}
	return i;
}

static void store_digits(const char *s, int *n, object *p)
{
	*p = store_int(convert_digits(s, n, load_int(*p)));
}

static object push_int(int i, object st)
{
	return cons(store_int(i), st);
}

static object push_zero(object st)
{
	return push_int(0, st);
}

static void add(object *p, int n)
{
	*p = store_int(load_int(*p) + n);
}

static void frac_part(const char *s, int n, object st)
{
	object *p = third(st);
	int m = n;
	if (p && type_of(*p) == TYP_INT) {
		store_digits(s, &n, p);
		if (n) {
			m -= n;
			*p = push_zero(*p);
		}
		add(second(st), m);
	}
}

static object parse_digits(const char *s, int n, object st)
{
	object *p = first(st);
	if (p) {
		switch (*p) {
		case '+':
		case '-':
			return parse_digits(s, n, push_zero(st));
		case '.':
			frac_part(s, n, st);
			break;
		case 0xE0:
		case 0xE1:
		case 0xE2:
			*p |= 4;
		case 0xE4:
		case 0xE5:
		case 0xE6:
			if (p = second(st))
				store_digits(s, &n, p);
			break;
		default:
			if (is_cons(*p))
				add(first(*p), n);
			else {
				store_digits(s, &n, p);
				if (n)
					*p = push_int(n, *p);
			}
		}
	}
	return st;
}

static object parse_number1(const char *s, const char **end, object st)
{
	object *p = first(st), a = 0xE0;
	int n;
	if (p && *p == 0xE0) {
		switch (*s) {
		case '-': (*p)++;
		case '+': (*p)++; s++;
		}
	}
	n = match_digits(s);
	if (n) {
		st = parse_digits(s, n, st);
		s += n;
	}
	switch (*s) {
	case '.':
		a = '.';
	case 'e':
	case 'E':
		return parse_number1(s+1, end, cons(a, push_zero(st)));
	}
	*end = s;
	return st;
}

static int pop_number(object *st, int *e)
{
	object top = pop(st), *p;
	int i;
	/* exponent part */
	switch (top) {
	case 0xE4:
	case 0xE5:
		*e += load_int(pop(st));
		top = pop(st);
		break;
	case 0xE6:
		*e -= load_int(pop(st));
		top = pop(st);
	}
	/* number of fraction digits */
	if (top == '.') {
		*e -= load_int(pop(st));
		top = pop(st);
	}
	/* number of excess int digits */
	*e += load_int(pop(&top));

	if (type_of(top) == TYP_INT) {
		i = load_int(top);
		if (p = first(*st)) {
			switch (*p) {
			case '+': return i;
			case '-': return -i;
			}
		}
	}
	*st = 0;
	return 0;
}

static object parse_number(const char *s, const char **end, object st)
{
	object *p;
	int i, e = 0;
	mpoint mp;
	listdata_mark(mp);
	st = parse_number1(s, &s, st);
	if (*s) {
		i = pop_number(&st, &e);
		listdata_release(mp);
		if (st) {
			*(p = first(st)) = store_int(i);
			if (e)
				*p = cons(*p, store_int(e));
		}
	}
	*end = s;
	return st;
}

static int esc_char(int c)
{
	switch (c) {
	case '"':
	case '\\':
	case '/': return c;
	case 'b': return '\b';
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	}
	return 0;
}

static int match_hex_quad(const char *s)
{
	int i;
	for (i=0; i<4; i++) {
		if (!isxdigit(*s++))
			return i;
	}
	return 4;
}

static int convert_hex_quad(const char *s)
{
	char hexs[6] = "";
	unsigned u;
	hexs[0] = s[0];
	hexs[1] = s[1];
	hexs[2] = s[2];
	hexs[3] = s[3];
	sscanf(hexs, "%x", &u);
	return u;
}

static object append(object x, object y)
{
	return x ? concat(x, y) : y;
}

static object append_str(object x, const char *s)
{
	return *s ? append(x, store_str(s)) : x;
}

static object append_uc(object x, int uc)
{
	object *p = first(last_tail(x, 1));
	if (p && type_of(*p) == TYP_STR && type_of(p[1]) == TYP_STR)
		return cons(x, store_int(uc));
	else
		return append(x, store_int(uc));
}

static object parse_chars(const char *s, const char **end, object st, object *p, int c)
{
	char buf[1024];
	int i = 0, n;
	if (c) {		/* prepend char */
		buf[0] = c;
		i = 1;
	}
	for (; *s && *s != '"'; s++, i++) {
		if (i >= sizeof(buf)-1) {
			buf[sizeof(buf)-1] = '\0';
			*p = append_str(*p, buf);
			i = 0;
		}
		if ((unsigned char) *s < 32)
			return 0;
		if (*s != '\\') {
			buf[i] = *s;
			continue;
		}

		/* escape sequence */
		if (!*++s) {
			st = cons('\\', st);
			break;
		}
		if (*s == 'u') {
			n = match_hex_quad(++s);
			if (n < 4) {
				st = cons('u', cons(store_str(s), st));
				s += n;
				break;
			}
			c = convert_hex_quad(s);
			if (c && (unsigned) c < 0x100)
				buf[i] = c;
			else {
				buf[i] = '\0';
				*p = append_uc(append_str(*p, buf), c);
				i = -1;
			}
			s += 3;
		} else {
			buf[i] = esc_char(*s);
			if (!buf[i])
				return 0;
		}
	}
	if (i > 0) {
		buf[i] = '\0';
		*p = append_str(*p, buf);
	}
	*end = s;
	return st;
}

static object parse_string1(const char *s, const char **end, object st, int c)
{
	object *p = first(st);
	if (!p)
		return 0;
	st = parse_chars(s, &s, st, p, c);
	if (*s == '"') {
		if (type_of(last_tail(*p, 0)) != TYP_STR)
			*p = append(*p, store_str(""));
		s++;
	} else if (*s)
		return 0;
	else if (first(st) == p)
		st = cons('"', st);
	*end = s;
	return st;
}

static object parse_string(const char *s, const char **end, object st)
{
	return parse_string1(s, end, cons(0, st), 0);
}

static object parse_element(const char *s, const char **end, object st)
{
	st = parse_value(s, &s, cons(',', st));
	*end = skip_ws(s);
	return st;
}

static object reduce_array(object st)
{
	object arr = st, *p = &arr, top;
	while (is_cons(st)) {
		if ((top = get_head(st)) == '[') {
			*p = EMPTY_LIST;
			*load_cons(st) = reverse_list(arr);
			return st;
		}
		if (is_state_atom(top))
			return 0;
		p = load_cons(st) + 1;
		st = *p;
	}
	return st == '[' ? reverse_list(arr) : 0;
}

static object parse_array(const char *s, const char **end, object st)
{
	s = skip_ws(s);
	if (*s == ',') {
		if (!is_cons(st) || get_head(st) == '[')
			return 0;
	} else if (*s && *s != ']')
		st = parse_element(s, &s, st);
	while (*s == ',' && st)
		st = parse_element(s+1, &s, st);
	if (*s == ']') {
		st = reduce_array(st);
		s++;
	} else if (*s)
		return 0;
	*end = s;
	return st;
}

static object reduce_object(object st)
{
	object obj = st, *p, *q = NULL, name;
	if (st == '{')
		return EMPTY_DICT;
	while ((p = first(st)) && !is_state_atom(*p) &&
	       (q = first(p[1])) && type_of(last_tail(*q, 0)) == TYP_STR) {
		name = *q;
		*q = *p;
		*p = name;
		st = q[1];
	}
	if (st == '{') {
		q[1] = EMPTY_DICT;
		return obj;
	}
	if (p && *p == '{') {
		if (!q)
			*p = EMPTY_DICT;
		else {
			*p = obj;
			q[1] = EMPTY_DICT;
		}
		return st;
	}
	return 0;
}

static object parse_object(const char *s, const char **end, object st, int n)
{
	s = skip_ws(s);
	if (n == 0) {
		if (*s == '"') {
			st = parse_string(s+1, &s, st);
			return parse_object(s, end, st, 1);
		}
	} else if (n % 2) {
		if (*s == ':') {
			st = parse_value(s+1, &s, cons(':', st));
			return parse_object(s, end, st, n+1);
		}
	} else if (*s == ',') {
		s = skip_ws(s+1);
		if (*s == '"')
			return parse_object(s, end, st, 0);
		st = cons(',', st);
	}
	if (*s == '}') {
		st = reduce_object(st);
		s++;
	} else if (*s)
		return 0;
	*end = s;
	return st;
}

static object parse_lit_name(const char *s, const char **end, object top)
{
	int i = top - LIT_NAME_0, j;
	while (*s && *s == lit_names[i+1]) { i++; s++; }
	*end = s;
	if (j = lit_ok[i])
		return lit_val[j];
	else
		return *s ? EMPTY_LIST : LIT_NAME_0 + i;
}

static object parse_value(const char *s, const char **end, object st)
{
	object *p;
	s = skip_ws(s);
	if (!*s) {
		*end = s;
		return st;
	}
	if (p = first(st)) {
		*p = '+';
		switch (*s) {
		case '"':
			*p = 0;
			return parse_string1(s+1, end, st, 0);
		case '{':
			*p = '{';
			return parse_object(s+1, end, st, 0);
		case '[':
			*p = '[';
			return parse_array(s+1, end, st);

		/* true false null */
		case 't': *p = LIT_NAME_T; break;
		case 'f': *p = LIT_NAME_F; break;
		case 'n': *p = LIT_NAME_N; break;

		/* number */
		case '-':
			*p = '-';
			s++;
		default:
			return parse_number(s, end, st);
		}
		*p = parse_lit_name(s+1, end, *p);
		if (*p != EMPTY_LIST)
			return st;
	}
	return 0;
}

/* continue parsing string */
static object parse_hex_quad(const char *s, const char **end, object st)
{
	object *p;
	char buf[8] = "\\u";
	int n = copy_str(pop(&st), buf+2, 4), uc;
	for (; n<4 && *s; n++)
		buf[n+2] = *s++;
	n = match_hex_quad(buf+2);
	if (n < 4) {
		if (*s)
			return 0;
		*end = s;
		return parse_string1(buf, &s, st, 0);
	}
	uc = convert_hex_quad(buf+2);
	if (uc && (unsigned) uc < 0x100)
		return parse_string1(s, end, st, uc);
	p = first(st);
	if (!p)
		return 0;
	*p = append_uc(*p, uc);
	return parse_string1(s, end, st, 0);
}

static object parse_esc(const char *s, const char **end, object st)
{
	int c;
	if (*s == 'u')
		return parse_hex_quad(s+1, end, cons(store_str(""), st));
	c = esc_char(*s);
	return c ? parse_string1(s+1, end, st, c) : 0;
}

static object parse(const char *s, object st)
{
	object t, *p;
	int n;
	while (*s) {
		t = st;
		n = 0;
		while (p = first(t)) {
			if (*p == '{' || *p == '[') {
				t = *p;
				break;
			}
			if (*p == '+' || *p == '-') {
				st = parse_number(s, &s, st);
				return parse(s, st);
			}
			t = p[1];
			n++;
		}
		switch (t) {
		case '{':
			st = parse_object(s, &s, st, n);
			break;
		case '[':
			st = parse_array(s, &s, st);
			break;
		default:
			return *skip_ws(s) ? 0 : st;
		}
		s = skip_ws(s);
	}
	return st;
}

object json_parse_start(const char *s)
{
	return json_parse(s, ' ');
}

object json_parse(const char *s, object st)
{
	object *p = first(st);
	if (st == ' ') {
		s = skip_ws(s);
		return *s ? parse(s+1, *s) : st;
	}
	if (!*s)
		return st;
	if (p) {
		switch (*p) {
		case ',':
		case ':':
			st = parse_value(s, &s, st);
			break;
		case '"':
			st = parse_string1(s, &s, p[1], 0);
			break;
		case '\\':
			st = parse_esc(s, &s, p[1]);
			break;
		case 'u':
			st = parse_hex_quad(s, &s, p[1]);
			break;
		case LIT_NAME_T:	/* tr */
		case LIT_NAME_T+1:	/* tru */
		case LIT_NAME_T+2:	/* true */
		case LIT_NAME_F:	/* fa */
		case LIT_NAME_F+1:	/* fal */
		case LIT_NAME_F+2:	/* fals */
		case LIT_NAME_F+3:	/* false */
		case LIT_NAME_N:	/* nu */
		case LIT_NAME_N+1:	/* nul */
		case LIT_NAME_N+2:	/* null */
			*p = parse_lit_name(s, &s, *p);
			if (*p == EMPTY_LIST)
				return 0;
			break;
		}
	}
	return parse(s, st);
}

int json_parse_done(object st)
{
	switch (last_tail(st, 0)) {
	case EMPTY_LIST:
	case EMPTY_DICT:
		return 1;
	default:
		return 0;
	}
}
