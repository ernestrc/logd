#ifndef LOGD_LOG_H
#define LOGD_LOG_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define KEY_THREAD "thread"
#define KEY_CLASS "class"
#define KEY_LEVEL "level"
#define KEY_TIME "time"
#define KEY_DATE "date"
#define KEY_MESSAGE "msg"
#define KEY_CALLTYPE "callType"

typedef struct prop_s {
	struct prop_s* next;
	const char* key;
	const char* value;
} prop_t;

typedef struct log_s {
	prop_t* props;
	bool is_safe;
} log_t;

log_t* log_create();
void log_init(log_t* l);
const char* log_get(log_t* l, const char* key);
prop_t* log_remove(log_t* l, const char* key);
void log_set(log_t* l, prop_t* prop, const char* key, const char* value);
int log_size(log_t* l);
void log_free(log_t* l);

#endif
