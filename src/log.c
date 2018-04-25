#include <stdlib.h>
#include <errno.h>

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

}
