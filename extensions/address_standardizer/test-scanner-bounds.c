#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef STANDARD_SOURCE_FILE
#define STANDARD_SOURCE_FILE "standard.c"
#endif

#include STANDARD_SOURCE_FILE

#undef fprintf
#undef printf
#undef vsnprintf

static int morph_calls;
static DEFDEF morph_token;
static int morph_length;
static char morph_text[MAXSTRLEN];
static int term_calls;
static int term_kind;
static char term_text[MAXSTRLEN];

int
pg_sprintf(char *dest, const char *format, ...)
{
	va_list args;
	int result;

	va_start(args, format);
	result = vsnprintf(dest, MAXSTRLEN, format, args);
	va_end(args);
	return result;
}

void
register_error(ERR_PARAM *errors)
{
	(void) errors;
}

int
new_morph(STAND_PARAM *stand, DEFDEF token, const char *text, int length)
{
	(void) stand;
	morph_calls++;
	morph_token = token;
	morph_length = length;
	if (length < 0 || length >= MAXSTRLEN)
		return FALSE;
	memcpy(morph_text, text, (size_t) length + 1);
	return TRUE;
}

void
set_term(STAND_PARAM *stand, int kind, const char *text)
{
	(void) stand;
	term_calls++;
	term_kind = kind;
	strncpy(term_text, text, sizeof(term_text) - 1);
}

static void
reset_observations(void)
{
	morph_calls = 0;
	morph_token = (DEFDEF) -1;
	morph_length = -1;
	memset(morph_text, 0, sizeof(morph_text));
	term_calls = 0;
	term_kind = -1;
	memset(term_text, 0, sizeof(term_text));
}

static int
scan(const char *label, const char *input, int expect_null,
     int expect_morph_token, int expect_morph_length,
     int expect_term_calls, int expect_term_kind, int expect_offset)
{
	static STAND_PARAM stand;
	static ERR_PARAM errors;
	static char error_text[MAXSTRLEN];
	char *copy;
	char *result;
	int expected_morph_calls = expect_morph_token < 0 ? 0 : 1;

	memset(&stand, 0, sizeof(stand));
	memset(&errors, 0, sizeof(errors));
	memset(error_text, 0, sizeof(error_text));
	errors.error_buf = error_text;
	stand.errors = &errors;
	reset_observations();

	copy = strdup(input);
	if (copy == NULL)
		return 99;
	result = _Scan_Next_(&stand, copy);

	if ((result == NULL) != expect_null ||
	    morph_calls != expected_morph_calls ||
	    (int) morph_token != expect_morph_token ||
	    morph_length != expect_morph_length ||
	    term_calls != expect_term_calls ||
	    term_kind != expect_term_kind ||
	    (!expect_null && result != copy + expect_offset) ||
	    (expected_morph_calls == 1 &&
	     (memcmp(morph_text, copy, (size_t) expect_morph_length) != 0 ||
	      morph_text[expect_morph_length] != '\0')) ||
	    (expect_term_calls == 1 && term_text[0] != '\0') ||
	    (expect_null &&
	     strcmp(error_text,
	            "_Scan_Next_: Token exceeds maximum length") != 0))
	{
		fprintf(stderr,
		        "%s: result_null=%d morph_calls=%d token=%d length=%d "
		        "term_calls=%d term_kind=%d offset=%td error=%s\n",
		        label, result == NULL, morph_calls, (int) morph_token,
		        morph_length, term_calls, term_kind,
		        result == NULL ? -1 : result - copy, error_text);
		free(copy);
		return 1;
	}

	free(copy);
	return 0;
}

static void
fill(char *buffer, size_t count, char byte)
{
	memset(buffer, byte, count);
	buffer[count] = '\0';
}

static int
run_case(const char *selected)
{
	char input[400];

	if (strcmp(selected, "collect-while-safe") == 0)
	{
		fill(input, 255, 'A');
		return scan(selected, input, 0, DWORDT, 255, 0, -1, 255);
	}

	if (strcmp(selected, "collect-while-over") == 0)
	{
		fill(input, 256, 'A');
		return scan(selected, input, 1, -1, -1, 0, -1, -1);
	}

	if (strcmp(selected, "collect-lookahead-safe") == 0)
	{
		fill(input, 253, '4');
		input[253] = 't';
		input[254] = 'h';
		input[255] = '\0';
		return scan(selected, input, 0, DORD, 255, 0, -1, 255);
	}

	if (strcmp(selected, "collect-lookahead-over") == 0)
	{
		fill(input, 254, '4');
		input[254] = 't';
		input[255] = 'h';
		input[256] = '\0';
		return scan(selected, input, 1, -1, -1, 0, -1, -1);
	}

	if (strcmp(selected, "spacers-newline") == 0)
	{
		fill(input, 300, ' ');
		input[300] = '\n';
		input[301] = '\0';
		return scan(selected, input, 0, -1, -1, 1, 2, 300);
	}

	if (strcmp(selected, "spacers-sentinel") == 0)
	{
		fill(input, 300, ' ');
		return scan(selected, input, 0, -1, -1, 1, 2, 300);
	}

	fprintf(stderr, "unknown scanner test case: %s\n", selected);
	return 2;
}

int
main(int argc, char **argv)
{
	static const char *const cases[] = {
		"collect-while-safe",
		"collect-while-over",
		"collect-lookahead-safe",
		"collect-lookahead-over",
		"spacers-newline",
		"spacers-sentinel"
	};
	size_t i;

	if (argc == 2)
		return run_case(argv[1]);
	if (argc != 1)
		return 2;

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
	{
		if (run_case(cases[i]) != 0)
			return 1;
	}
	return 0;
}
