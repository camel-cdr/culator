/* culator -- A simple infix notation floating-point cli calculator.
 * Based on Per Vognsen's Bitwise day 3: https://youtu.be/0woxSWjWsb8
 * By Olaf Bernstein <camel-cdr@protonmail.com>
 */

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg.h"

#define MAX_FUNC_ARGS 256
#define SIZEOF_ARR(arr) (sizeof(arr) / sizeof(*arr))
#define REAL_FMT "Lg"

typedef long double real;

enum TokenKind {
	TOKEN_VAL,
	TOKEN_ADD,
	TOKEN_SUB,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_POW,
	TOKEN_FUNC,
	TOKEN_CONST,
	TOKEN_LPARAM,
	TOKEN_RPARAM,
	TOKEN_COMMA,
	TOKEN_EOF,
	NUM_TOKENS
};

struct Token {
	enum TokenKind kind;
	char const *start, *end;
	union {
		real val;
		struct {
			int nargs;
			real (*func)(real *reals);
		} func;
	} as;
};

#define X(name, func, nargs, args) \
	static real name##_real(real *x) { return func args; }
#include "functions.c"

/* implement custom functions */
static real torad(real *x) { return x[0] * M_PI / 180.0L; }

static struct {
	char *name;
	int nargs;
	real (*func)(real *reals);
} functions[] = {
#define X(name, func, nargs, args) \
	{ #name, nargs, name##_real },
#include "functions.c"
	/* register custom functions */
	{ "torad", 1, torad },
};

static struct {
	char *name;
	real val;
} constants[] = {
	{ "pi", 3.141592653589793238462643383279502884L },
	{ "e", 2.718281828459045235360287471352662498L },
	{ "M_E", 2.718281828459045235360287471352662498L },
	{ "M_LOG2E", 1.442695040888963407359924681001892137L },
	{ "M_LOG10E", 0.434294481903251827651128918916605082L },
	{ "M_LN2", 0.693147180559945309417232121458176568L },
	{ "M_LN10", 2.302585092994045684017991454684364208L },
	{ "M_PI", 3.141592653589793238462643383279502884L },
	{ "M_PI_2", 1.570796326794896619231321691639751442L },
	{ "M_PI_4", 0.785398163397448309615660845819875721L },
	{ "M_1_PI", 0.318309886183790671537767526745028724L },
	{ "M_2_PI", 0.636619772367581343075535053490057448L },
	{ "M_2_SQRTPI", 1.128379167095512573896158903121545172L },
	{ "M_SQRT2", 1.414213562373095048801688724209698079L },
	{ "M_SQRT1_2", 0.707106781186547524400844362104849039L },
	/* add custom constants */
};

static int precision = 15;

static struct Token token = {0};
static char const *stream;

static void
error(char const *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(EXIT_FAILURE);
}

static void
warning(char const *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "WARNING: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}


static void
next_token(void)
{
repeat:
	token.start = stream;
	switch (*stream) {
	case ' ': case '\n': case '\r': case '\t': case '\v':
		while (isspace(*stream)) {
			++stream;
		}
		goto repeat;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9': {
		while (isdigit(*stream)) { ++stream;}
		if (*stream == '.') { ++stream; }
		while (isdigit(*stream)) { ++stream; }
		token.kind = TOKEN_VAL;
		sscanf(token.start, "%"REAL_FMT, &token.as.val);
		break;
	}
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
	case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
	case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
	case 'v': case 'w': case 'x': case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
	case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
	case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case '_': {
		size_t i, len;
		while (isalnum(*stream) || *stream == '_') {
			++stream;
		}
		len = stream - token.start;

		for (i = 0; i < SIZEOF_ARR(constants); ++i) {
			if (strlen(constants[i].name) == len && strncmp(token.start, constants[i].name, len) == 0) {
				token.kind = TOKEN_CONST;
				token.as.val = constants[i].val;
				goto end;
			}
		}
		for (i = 0; i < SIZEOF_ARR(functions); ++i) {
			if (strlen(functions[i].name) == len && strncmp(token.start, functions[i].name, len) == 0) {
				token.kind = TOKEN_FUNC;
				token.as.func.func = functions[i].func;
				token.as.func.nargs = functions[i].nargs;
				goto end;
			}
		}
		warning("Unknown name '%.*s', skipping", len, token.start);
		break;
	}
	case '*':
		token.kind = TOKEN_MUL;
		if (*++stream == '*') { case '^':
			token.kind = TOKEN_POW;
			++stream;
		}
		break;
	case '+': token.kind = TOKEN_ADD; ++stream; break;
	case '-': token.kind = TOKEN_SUB; ++stream; break;
	case '/': token.kind = TOKEN_DIV; ++stream; break;
	case '(': token.kind = TOKEN_LPARAM; ++stream; break;
	case ')': token.kind = TOKEN_RPARAM; ++stream; break;
	case ',': token.kind = TOKEN_COMMA; ++stream; break;
	case '\0': token.kind = TOKEN_EOF; ++stream; break;
	default:
		warning("Invalid '%c' token, skipping", *stream);
		++stream;
		goto repeat;
	}
end:
	token.end = stream;
}

static char const *tokenToNames[NUM_TOKENS] = {
	"Val",
	"+",
	"-",
	"*",
	"/",
	"^",
	"Func",
	"Const",
	"(",
	")",
	",",
	"EOF",
};

#define is_token(k) (token.kind == k)
#define match_token(k) (is_token(k) ? (next_token(), 1) : 0)
#define expect_token(k) \
	(is_token(k) \
		? (next_token(), 1) \
		: (error("Expected token '%s', got '%s'", tokenToNames[k], tokenToNames[token.kind]), 0))

static real parse_expr(void);

static real
parse_expr4(void)
{
	if (is_token(TOKEN_VAL)) {
		real val = token.as.val;
		next_token();
		return val;
	} else if (is_token(TOKEN_CONST)) {
		real val = token.as.val;
		next_token();
		return val;
	} else if (is_token(TOKEN_FUNC)) {
		static real args[MAX_FUNC_ARGS] = {0};
		int i, nargs = token.as.func.nargs;
		real (*func)(real *) = token.as.func.func;

		next_token();
		expect_token(TOKEN_LPARAM);
		for (i = 0; i < nargs-1; ++i) {
			args[i] = parse_expr();
			expect_token(TOKEN_COMMA);
		}
		args[nargs - 1] = parse_expr();
		expect_token(TOKEN_RPARAM);

		return func(args);
	} else if (match_token(TOKEN_LPARAM)) {
		real val = parse_expr();
		expect_token(TOKEN_RPARAM);
		return val;
	} else {
		error("Unexpected token");
		return 0;
	}
}

static real
parse_expr3(void)
{
	if (match_token(TOKEN_SUB))
		return -parse_expr3();
	else if (match_token(TOKEN_ADD))
		return parse_expr3();
	else
		return parse_expr4();

}

static real
parse_expr2(void)
{
	real val = parse_expr3();
	while (is_token(TOKEN_POW)) {
		next_token();
		val = pow(val, parse_expr3());
	}
	return val;
}

static real
parse_expr1(void)
{
	real val = parse_expr2();
	while (is_token(TOKEN_MUL) || is_token(TOKEN_DIV)) {
		enum TokenKind op = token.kind;
		next_token();
		if (op == TOKEN_MUL) {
			val *= parse_expr2();
		} else {
			assert(op == TOKEN_DIV);
			val /= parse_expr2();
		}
	}
	return val;
}

static real
parse_expr(void)
{
	real val = parse_expr1();
	while (is_token(TOKEN_ADD) || is_token(TOKEN_SUB)) {
		enum TokenKind op = token.kind;
		next_token();
		if (op == TOKEN_ADD) {
			val += parse_expr1();
		} else {
			assert(op == TOKEN_SUB);
			val -= parse_expr1();
		}
	}
	return val;
}

static void
parse_str(char const *str)
{
	static struct Token nul = {0};
	token = nul;
	stream = str;
	next_token();
	if (!is_token(TOKEN_EOF))
		printf("%.*"REAL_FMT "\n", precision, parse_expr());
}

static int
parse_expr_stdin()
{
	static char *str = NULL;
	static size_t len = 0, cap = 0;

	while (1) {
		if (len >= cap) {
			cap = cap * 2 + 128;
			str = realloc(str, cap);
		}
		while (len < cap) {
			int c = getchar();
			switch (c) {
			case '\n': goto end;
			case EOF: return 0;
			}
			str[len++] = c;
		}
	}
end:
	str[len] = '\0';
	len = 0;
	parse_str(str);
	return 1;
}

static void
usage(char *argv0)
{
	printf("usage: %s [OPTIONS] [EXPRESSION ...]\n", argv0);
	puts("A simple infix notation floating-point cli calculator.");
	puts("Reads from stdin if no EXPRESSION is given.\n");
	puts("  -p, --precision=NUM  quit after NUM outputs");
	puts("  -h, --help           display this help and exit");
}

int
main(int argc, char **argv)
{
	int i;
	char *argv0 = argv[0];

	ARG_BEGIN {
		if (ARG_LONG("help")) case 'h': case '?': {
			usage(argv0);
			return EXIT_SUCCESS;
		} else if (ARG_LONG("precision")) case 'p': {
			precision = atoi(ARG_VAL());
		} else { default:
			fprintf(stderr,
			        "%s: invalid option '%s'\n"
			        "Try '%s --help' for more information.\n",
			        argv0, *argv, argv0);
			return EXIT_FAILURE;
		}
	} ARG_END;

	if (argc > 0) {
		for (i = 0; i < argc; ++i)
			parse_str(argv[i]);
	} else {
		while (parse_expr_stdin());
	}

	return EXIT_SUCCESS;
}

