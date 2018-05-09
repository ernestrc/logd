#ifndef LOGD_UTIL_H
#define LOGD_UTIL_H

#include "log.h"

#define __STR_HELPER(x) #x
#define STR(x) __STR_HELPER(x)

#ifdef LOGD_DEBUG
#define DEBUG_ASSERT(cond) assert(cond);
#else
#define DEBUG_ASSERT(cond)
#endif

void printl(log_t* l);
int snprintl(char* buf, int blen, log_t* l);
const char* util_get_time();
const char* util_get_date();

#endif
