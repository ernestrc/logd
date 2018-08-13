#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

#include "parser.h"
#include "util.h"

static const char* util_log_get(log_t* l, const char* key)
{
	const char* value = log_get(l, key);
	if (value == NULL)
		return "-";

	return value;
}

#define ADD_OFFSET(want, buf, blen, res)                                       \
	blen -= want;                                                              \
	if (blen < 0) {                                                            \
		blen = 0;                                                              \
		buf = NULL;                                                            \
	} else {                                                                   \
		buf += want;                                                           \
	}                                                                          \
	res += want;

static int snprintp(char* buf, int blen, log_t* l)
{
	prop_t* p;
	int want = 0;
	int res = 0;

	for (p = l->props; p != NULL && p->key != NULL; p = p->next) {
		if (strcmp(p->key, KEY_DATE) != 0 && strcmp(p->key, KEY_TIME) != 0 &&
		  strcmp(p->key, KEY_LEVEL) != 0 && strcmp(p->key, KEY_CLASS) != 0 &&
		  strcmp(p->key, KEY_THREAD) != 0 && strcmp(p->key, KEY_CALLTYPE)) {
			want = snprintf(buf, blen, "%s: %s, ", p->key,
			  p->value == NULL ? "null" : p->value);
			ADD_OFFSET(want, buf, blen, res);
		}
	}

	return res;
}

int snprintl(char* buf, int blen, log_t* l)
{
	int want;
	int res = 0;

	want =
	  snprintf(buf, blen, "%s %s\t%s\t[%s]\t%s\t", util_log_get(l, KEY_DATE),
		util_log_get(l, KEY_TIME), util_log_get(l, KEY_LEVEL),
		util_log_get(l, KEY_THREAD), util_log_get(l, KEY_CLASS));

	ADD_OFFSET(want, buf, blen, res);

	const char* call_type = log_get(l, KEY_CALLTYPE);
	if (call_type != NULL) {
		want = snprintf(buf, blen, "%s: ", call_type);
		ADD_OFFSET(want, buf, blen, res);
	}

	want = snprintp(buf, blen, l);
	ADD_OFFSET(want, buf, blen, res);

	return res;
}

static void fprintp(FILE* stream, log_t* l)
{
	prop_t* p;
	for (p = l->props; p != NULL && p->key != NULL; p = p->next) {
		if (strcmp(p->key, KEY_DATE) != 0 && strcmp(p->key, KEY_TIME) != 0 &&
		  strcmp(p->key, KEY_LEVEL) != 0 && strcmp(p->key, KEY_CLASS) != 0 &&
		  strcmp(p->key, KEY_THREAD) != 0 && strcmp(p->key, KEY_CALLTYPE)) {
			fprintf(
			  stream, "%s: %s, ", p->key, p->value == NULL ? "null" : p->value);
		}
	}
}

void fprintl(FILE* stream, log_t* log)
{
	fprintf(stream, "%s %s\t%s\t[%s]\t%s\t", util_log_get(log, KEY_DATE),
	  util_log_get(log, KEY_TIME), util_log_get(log, KEY_LEVEL),
	  util_log_get(log, KEY_THREAD), util_log_get(log, KEY_CLASS));

	const char* call_type = log_get(log, KEY_CALLTYPE);
	if (call_type != NULL) {
		fprintf(stream, "%s: ", call_type);
	}

	fprintp(stream, log);
	fprintf(stream, "\n");
}

void printl(log_t* log) { fprintl(stdout, log); }

const char* util_get_date()
{
#define MAX_DATE_LEN 11 /* 10 + null terminator */
	static char date_buf[MAX_DATE_LEN];

	time_t raw_time = time(NULL);
	struct tm* local_time = localtime(&raw_time);

	int needs = strftime(date_buf, MAX_DATE_LEN, "%Y-%m-%d", local_time);
	DEBUG_ASSERT(needs + 1 <= MAX_DATE_LEN);

	return date_buf;
}

const char* util_get_time()
{
#define MAX_TIME_LEN 9 /* 8 + null terminator */
	static char time_buf[MAX_TIME_LEN];

	time_t raw_time = time(NULL);
	struct tm* local_time = localtime(&raw_time);

	int needs = strftime(time_buf, MAX_TIME_LEN, "%X", local_time);
	DEBUG_ASSERT(needs + 1 <= MAX_TIME_LEN);

	return time_buf;
}

int next_attempt_backoff(
  int start_delay, int curr_reopen_retry, int backoff_exponent)
{
	int timeout, iter;
	DEBUG_ASSERT(curr_reopen_retry > 0);
	DEBUG_ASSERT(backoff_exponent > 0);

	if (backoff_exponent == 1) {
		timeout = start_delay * curr_reopen_retry;
	} else {
		timeout = pow(start_delay, curr_reopen_retry);
	}

	return timeout;
}

int parse_non_negative_int(const char* str)
{
	char* endptr;
	long val;

	errno = 0;
	val = strtol(str, &endptr, 10);

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) ||
	  (errno != 0 && val == 0)) {
		perror("strtol");
		goto error;
	}

	if (endptr == str || *endptr != '\0')
		goto error;

	return val;

error:
	errno = EINVAL;
	return -1;
}
