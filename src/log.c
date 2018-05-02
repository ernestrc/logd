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
		perror("calloc");
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

const char* log_get(log_t* l, const char* key)
{
	DEBUG_ASSERT(l != NULL);
	DEBUG_ASSERT(key != NULL);

	for (prop_t* next = l->props; next != NULL; next = next->next) {
		DEBUG_ASSERT(next->key != NULL);
		if (strcmp(key, next->key) == 0)
			return next->value;
	}

	return NULL;
}

prop_t* log_remove(log_t* l, const char* key)
{
	DEBUG_ASSERT(l != NULL);
	DEBUG_ASSERT(key != NULL);

	prop_t* ret = NULL;

	for (prop_t** next = &l->props; *next != NULL; next = &((*next)->next)) {
		DEBUG_ASSERT((*next)->key != NULL);
		if (strcmp(key, (*next)->key) == 0) {
			ret = (*next);
			(*next) = (*next)->next;
			break;
		}
	}

	return ret;
}

void log_set(log_t* l, prop_t* prop, const char* key, const char* value)
{
	DEBUG_ASSERT(l != NULL);
	DEBUG_ASSERT(prop != NULL);

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
