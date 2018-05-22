#ifndef LOGD_UTIL_H
#define LOGD_UTIL_H
#include <stdio.h>

#include "log.h"
#include "lua/lauxlib.h"
#include "lua/lua.h"

#define __STR_HELPER(x) #x
#define STR(x) __STR_HELPER(x)

#ifdef LOGD_DEBUG
#define DEBUG_ASSERT(cond) assert(cond);
#else
#define DEBUG_ASSERT(cond)
#endif

#ifdef LOGD_DEBUG
#define DEBUG_LOG(fmt, ...)                                                    \
	{                                                                          \
		log_t __log;                                                           \
		prop_t __prop[7];                                                      \
		log_init(&__log);                                                      \
		int __size = snprintf(NULL, 0, fmt, __VA_ARGS__);                      \
		char* __msg = malloc(__size + 1);                                      \
		sprintf(__msg, fmt, __VA_ARGS__);                                      \
		log_set(&__log, &__prop[0], KEY_MESSAGE, __msg);                       \
		log_set(&__log, &__prop[1], "line", STR(__LINE__));                    \
		log_set(&__log, &__prop[2], "func", __func__);                         \
		log_set(&__log, &__prop[3], KEY_LEVEL, "DEBUG");                       \
		log_set(&__log, &__prop[4], KEY_CLASS, __FILE__);                      \
		log_set(&__log, &__prop[5], KEY_TIME, util_get_time());                \
		log_set(&__log, &__prop[6], KEY_DATE, util_get_date());                \
		fprintl(stderr, &__log);                                               \
		free(__msg);                                                           \
	}
#else
#define DEBUG_LOG(fmt, ...)
#endif

void printl(log_t* l);
void fprintl(FILE* stream, log_t* l);
int snprintl(char* buf, int blen, log_t* l);
void sanitize_prop_key(char* str);
void sanitize_prop_value(char* str);
const char* util_get_time();
const char* util_get_date();

#endif
