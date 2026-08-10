#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef RULES_SOURCE_FILE
#define RULES_SOURCE_FILE "gamma.c"
#endif

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void __real_free(void *pointer);
void *__wrap_malloc(size_t size);
void *__wrap_calloc(size_t count, size_t size);
void __wrap_free(void *pointer);

enum { MAX_TRACKED_ALLOCATIONS = 1024 };

struct tracked_allocation
{
	void *pointer;
	size_t size;
	int active;
};

static struct tracked_allocation tracked[MAX_TRACKED_ALLOCATIONS];
static size_t tracked_count;
static size_t allocation_attempts;
static size_t successful_allocations;
static size_t frees;
static size_t active_allocations;
static size_t active_bytes;
static size_t invalid_frees;
static size_t fail_at_attempt;

static void
track_allocation(void *pointer, size_t size)
{
	if (!pointer)
		return;
	if (tracked_count >= MAX_TRACKED_ALLOCATIONS)
	{
		fprintf(stderr, "allocation tracker capacity exceeded\n");
		exit(2);
	}
	tracked[tracked_count].pointer = pointer;
	tracked[tracked_count].size = size;
	tracked[tracked_count].active = 1;
	tracked_count++;
	successful_allocations++;
	active_allocations++;
	active_bytes += size;
}

void *
__wrap_malloc(size_t size)
{
	void *pointer;

	allocation_attempts++;
	if (fail_at_attempt && allocation_attempts == fail_at_attempt)
		return NULL;
	pointer = __real_malloc(size);
	if (pointer)
		memset(pointer, 0, size);
	track_allocation(pointer, size);
	return pointer;
}

void *
__wrap_calloc(size_t count, size_t size)
{
	void *pointer;

	allocation_attempts++;
	if (fail_at_attempt && allocation_attempts == fail_at_attempt)
		return NULL;
	pointer = __real_calloc(count, size);
	track_allocation(pointer, count * size);
	return pointer;
}

void
__wrap_free(void *pointer)
{
	size_t i;

	if (!pointer)
		return;
	for (i = tracked_count; i > 0; i--)
	{
		struct tracked_allocation *allocation = &tracked[i - 1];

		if (allocation->active && allocation->pointer == pointer)
		{
			allocation->active = 0;
			active_allocations--;
			active_bytes -= allocation->size;
			frees++;
			__real_free(pointer);
			return;
		}
	}
	invalid_frees++;
}

#include RULES_SOURCE_FILE

#undef fprintf
#undef printf
#undef snprintf
#undef sprintf
#undef vfprintf
#undef vprintf
#undef vsnprintf
#undef vsprintf

int
pg_sprintf(char *buffer, const char *format, ...)
{
	int result;
	va_list arguments;

	va_start(arguments, format);
	result = vsnprintf(buffer, MAXSTRLEN + 1, format, arguments);
	va_end(arguments);
	return result;
}

static ERR_PARAM test_errors;
static char test_error_buffer[MAXSTRLEN + 1];

void
register_error(ERR_PARAM *err_p)
{
	(void) err_p;
}

static void
reset_errors(void)
{
	memset(&test_errors, 0, sizeof(test_errors));
	memset(test_error_buffer, 0, sizeof(test_error_buffer));
	test_errors.error_buf = test_error_buffer;
}

static void
release_test_leaks(void)
{
	size_t i;

	for (i = 0; i < tracked_count; i++)
	{
		if (tracked[i].active)
		{
			__real_free(tracked[i].pointer);
			tracked[i].active = 0;
		}
	}
	active_allocations = 0;
	active_bytes = 0;
}

static void
begin_case(void)
{
	if (active_allocations)
	{
		fprintf(stderr, "previous test left active allocations\n");
		exit(2);
	}
	memset(tracked, 0, sizeof(tracked));
	tracked_count = 0;
	allocation_attempts = 0;
	successful_allocations = 0;
	frees = 0;
	active_allocations = 0;
	active_bytes = 0;
	invalid_frees = 0;
	fail_at_attempt = 0;
	reset_errors();
}

static int
finish_case(const char *name, int operation_ok)
{
	int passed = operation_ok && active_allocations == 0 && invalid_frees == 0;

	printf(
		"case=%s operation=%s allocations=%zu frees=%zu active=%zu "
		"active_bytes=%zu invalid_frees=%zu result=%s\n",
		name,
		operation_ok ? "expected" : "unexpected",
		successful_allocations,
		frees,
		active_allocations,
		active_bytes,
		invalid_frees,
		passed ? "PASS" : "FAIL");
	release_test_leaks();
	return passed;
}

static int
check_init_free(void)
{
	RULES *rules;

	begin_case();
	rules = rules_init(&test_errors);
	if (rules)
		rules_free(rules);
	return finish_case("init-free", rules != NULL);
}

static int
check_add_free(void)
{
	int rule[] = {1, FAIL, 5, FAIL, 2, 7};
	RULES *rules;
	int add_result = -1;

	begin_case();
	rules = rules_init(&test_errors);
	if (rules)
	{
		add_result = rules_add_rule(
			rules, sizeof(rule) / sizeof(rule[0]), rule);
		rules_free(rules);
	}
	return finish_case("add-free", rules != NULL && add_result == 0);
}

static int
check_ready_free(void)
{
	int rule[] = {1, FAIL, 5, FAIL, 2, 7};
	RULES *rules;
	int add_result = -1;
	int ready_result = -1;

	begin_case();
	rules = rules_init(&test_errors);
	if (rules)
	{
		add_result = rules_add_rule(
			rules, sizeof(rule) / sizeof(rule[0]), rule);
		if (add_result == 0)
			ready_result = rules_ready(rules);
		rules_free(rules);
	}
	return finish_case(
		"ready-free", rules != NULL && add_result == 0 && ready_result == 0);
}

static int
check_init_failure(size_t relative_failure)
{
	char name[64];
	RULES *rules;

	begin_case();
	fail_at_attempt = relative_failure;
	rules = rules_init(&test_errors);
	fail_at_attempt = 0;
	if (rules)
		rules_free(rules);
	snprintf(name, sizeof(name), "init-failure-%zu", relative_failure);
	return finish_case(name, rules == NULL);
}

static int
check_add_failure(size_t relative_failure)
{
	char name[64];
	int rule[] = {1, FAIL, 5, FAIL, 2, 7};
	RULES *rules;
	int add_result = 0;

	begin_case();
	rules = rules_init(&test_errors);
	if (rules)
	{
		fail_at_attempt = allocation_attempts + relative_failure;
		add_result = rules_add_rule(
			rules, sizeof(rule) / sizeof(rule[0]), rule);
		fail_at_attempt = 0;
		rules_free(rules);
	}
	snprintf(name, sizeof(name), "add-failure-%zu", relative_failure);
	return finish_case(name, rules != NULL && add_result != 0);
}

static int
check_malformed_add(void)
{
	int rule[] = {1};
	RULES *rules;
	int add_result = 0;

	begin_case();
	rules = rules_init(&test_errors);
	if (rules)
	{
		add_result = rules_add_rule(
			rules, sizeof(rule) / sizeof(rule[0]), rule);
		rules_free(rules);
	}
	return finish_case("add-malformed", rules != NULL && add_result != 0);
}

static int
check_ready_failure(size_t relative_failure)
{
	char name[64];
	int rule[] = {1, FAIL, 5, FAIL, 2, 7};
	RULES *rules;
	int add_result = -1;
	int ready_result = 0;

	begin_case();
	rules = rules_init(&test_errors);
	if (rules)
	{
		add_result = rules_add_rule(
			rules, sizeof(rule) / sizeof(rule[0]), rule);
		if (add_result == 0)
		{
			fail_at_attempt = allocation_attempts + relative_failure;
			ready_result = rules_ready(rules);
			fail_at_attempt = 0;
		}
		rules_free(rules);
	}
	snprintf(name, sizeof(name), "ready-failure-%zu", relative_failure);
	return finish_case(
		name, rules != NULL && add_result == 0 && ready_result != 0);
}

static int
run_normal_cases(void)
{
	int passed = 1;

	passed &= check_init_free();
	passed &= check_add_free();
	passed &= check_ready_free();
	return passed;
}

static int
run_failure_cases(void)
{
	size_t failure;
	int passed = 1;

	for (failure = 1; failure <= 8; failure++)
		passed &= check_init_failure(failure);
	for (failure = 1; failure <= 2; failure++)
		passed &= check_add_failure(failure);
	passed &= check_malformed_add();
	for (failure = 1; failure <= 5; failure++)
		passed &= check_ready_failure(failure);
	return passed;
}

int
main(int argc, char **argv)
{
	int passed;

	if (argc != 2 ||
	    (strcmp(argv[1], "normal") && strcmp(argv[1], "failures")))
	{
		fprintf(stderr, "usage: %s normal|failures\n", argv[0]);
		return 2;
	}
	passed = !strcmp(argv[1], "normal") ?
		run_normal_cases() : run_failure_cases();
	return passed ? 0 : 1;
}
