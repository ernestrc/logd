#include <stdlib.h>
#include <errno.h>

#include "parser.h"

parser_t* parser_create()
{
	parser_t* p;

	if ((p = calloc(1, sizeof(parser_t))) == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	parser_init(p);

	return p;
}

void parser_init(parser_t* p) {
	p->end = NULL;
	p->start = NULL;
	p->raw = NULL;
	p->state = DATE_PSTATE;
	log_init(&p->log);
}

void parser_free(parser_t* p) {

}
