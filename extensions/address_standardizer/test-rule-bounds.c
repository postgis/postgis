#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef RULE_PARSER_SOURCE_FILE
#define RULE_PARSER_SOURCE_FILE "std_pg_hash.c"
#endif

#include RULE_PARSER_SOURCE_FILE

#undef fprintf
#undef printf

enum
{
	BEFORE_CANARY = 0x31415926,
	AFTER_CANARY = 0x5a17beef
};

struct guarded_rule
{
	volatile int before;
	int rule[MAX_RULE_LENGTH];
	volatile int after;
};

_Static_assert(
	offsetof(struct guarded_rule, after) ==
		offsetof(struct guarded_rule, rule) +
			sizeof(((struct guarded_rule *) 0)->rule),
	"the after canary must immediately follow the rule array");

static void
make_rule(char *text, size_t text_size, size_t terms)
{
	char *write = text;
	size_t i;

	if (text_size < terms * 2 + 1)
	{
		fprintf(stderr, "test input buffer is too small\n");
		exit(2);
	}

	for (i = 0; i < terms; i++)
	{
		*write++ = '1';
		*write++ = ' ';
	}
	*write = '\0';
}

static int
check_case(size_t terms, int expected_return)
{
	char text[(MAX_RULE_LENGTH + 1) * 2 + 1];
	struct guarded_rule guarded;
	int actual_return;
	int passed;

	memset(guarded.rule, 0, sizeof(guarded.rule));
	guarded.before = BEFORE_CANARY;
	guarded.after = AFTER_CANARY;
	make_rule(text, sizeof(text), terms);

	actual_return = parse_rule(text, guarded.rule);
	passed = actual_return == expected_return &&
		guarded.before == BEFORE_CANARY && guarded.after == AFTER_CANARY;

	printf(
		"terms=%zu expected_return=%d actual_return=%d "
		"before_canary=%s after_canary=%s result=%s\n",
		terms,
		expected_return,
		actual_return,
		guarded.before == BEFORE_CANARY ? "intact" : "changed",
		guarded.after == AFTER_CANARY ? "intact" : "changed",
		passed ? "PASS" : "FAIL");

	return passed;
}

int
main(void)
{
	int passed_128 = check_case(MAX_RULE_LENGTH, MAX_RULE_LENGTH);
	int passed_129 = check_case(MAX_RULE_LENGTH + 1, -1);

	return passed_128 && passed_129 ? 0 : 1;
}
