#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "log.h"

log_t* log_create()
{
	log_t* l;

	if ((l = calloc(1, sizeof(log_t))) == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	log_init(l);

	return l;
}

void log_init(log_t* l)
{
	DEBUG_ASSERT(l != NULL);
	l->props = NULL;
}

str_t log_get(log_t* l, const char* kval, size_t klen)
{
	DEBUG_ASSERT(l != NULL);
	DEBUG_ASSERT(kval != NULL);

	for (prop_t* next = l->props; next != NULL; next = next->next) {
		DEBUG_ASSERT(next->key != NULL);
		if (LOG_GET_STRLEN(next->key) == klen &&
		  memcmp(kval, LOG_GET_STRVALUE(next->key), klen) == 0)
			return next->value;
	}

	return NULL;
}

prop_t* log_remove(log_t* l, const char* kval, size_t klen)
{
	DEBUG_ASSERT(l != NULL);
	DEBUG_ASSERT(kval != NULL);

	prop_t* ret = NULL;

	for (prop_t** next = &l->props; *next != NULL; next = &((*next)->next)) {
		DEBUG_ASSERT((*next)->key != NULL);
		if (LOG_GET_STRLEN((*next)->key) == klen &&
		  memcmp(kval, LOG_GET_STRVALUE((*next)->key), klen) == 0) {
			ret = (*next);
			(*next) = (*next)->next;
			break;
		}
	}

	return ret;
}

void log_set(log_t* l, prop_t* prop, const str_t key, const str_t value)
{
	DEBUG_ASSERT(l != NULL);
	DEBUG_ASSERT(prop != NULL);
	DEBUG_ASSERT(value != NULL);
	DEBUG_ASSERT(key != NULL);

	prop->key = key;
	prop->value = value;

	prop->next = l->props;
	l->props = prop;
}

void log_free(log_t* l)
{
	if (l)
		free(l);
}
