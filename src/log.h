#ifndef LOGD_LOG_H
#define LOGD_LOG_H

#include <assert.h>
#include <stdlib.h>

#define KEY_THREAD "thread"
#define KEY_CLASS "class"
#define KEY_LEVEL "level"
#define KEY_TIME "time"
#define KEY_DATE "date"
#define KEY_TIMESTAMP "timestamp"
#define KEY_MESSAGE "msg"


#define LOG_GET_STRLEN(str) ((size_t)((str)[0]))
#define LOG_SET_STRLEN(str, len) ((str)[0] = ((char)(len)))

#define LOG_GET_STRVALUE(str) ((char*)(&(str)[1]))

/* anything of this type should be 1 byte length-prefixed */
typedef char* str_t;

typedef struct prop_s {
	struct prop_s* next;
	str_t key;
	str_t value;
} prop_t;

typedef struct log_s {
	prop_t* props;
} log_t;

log_t* log_create();
void log_init(log_t* l);
str_t log_get(log_t* l, const char* key, size_t klen);
prop_t* log_remove(log_t* l, const char* key, size_t klen);
void log_set(log_t* l, prop_t* prop, const str_t key, const str_t value);
void log_free(log_t* l);

#endif
