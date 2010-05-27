
static int add_ifnz(int a, int b)
{
	return a && b ? a+b : 0;
}

static int match_chars(const char *s)
{
	int n = 1;
	switch (*s) {
	case '"':
		return 0;
	case '\\':
		n = 2;
		break;
	default:
		if ((unsigned char) *s < 32)
			return 0;
	}
	return n + match_chars(s+n);
}

static int match_string(const char *s)
{
	int n;
	if (*s == '"') {
		n = 1 + match_chars(s+1);
		if (s[n] == '"')
			return n+1;
	}
	return 0;
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

static int match_frac(const char *s)
{
	return *s == '.' ? add_ifnz(1, match_digits(s+1)) : 0;
}

static int match_e(const char *s)
{
	switch (*s) {
	case 'e':
	case 'E':
		switch (s[1]) {
		case '+':
		case '-':
			return 2;
		default:
			return 1;
		}
	default:
		return 0;
	}
}

static int match_exp(const char *s)
{
	int n = match_e(s);
	return add_ifnz(n, match_digits(s+n));
}

static int match_int(const char *s)
{
	switch (*s) {
	case '0':
		return 1;
	case '-':
		return add_ifnz(s[1] != '-', match_int(s+1));
	default:
		return match_digits(s);
	}
}

static int match_number(const char *s)
{
	int n = match_int(s);
	if (n) {
		n += match_frac(s+n);
		n += match_exp(s+n);
	}
	return n;
}
