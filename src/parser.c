#include <errno.h>
#include <stdlib.h>

#include "debug.h"
#include "parser.h"

#ifndef PARSER_SLAB_CAP
#define PARSER_SLAB_CAP 100
#endif

parser_t* parser_create()
{
	parser_t* p;

	if ((p = calloc(1, sizeof(parser_t))) == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	slab_t* slab = slab_create(PARSER_SLAB_CAP, sizeof(prop_t));
	parser_init(p, slab);

	return p;
}

void parser_init(parser_t* p, slab_t* pslab)
{
	p->bstart = NULL;
	p->blen = 0;
	p->state = DATE_PSTATE;
	p->pslab = pslab;
	log_init(&p->result);
}

void parser_free(parser_t* p)
{
	if (p == NULL)
		return;

	slab_free(p->pslab);
	free(p);
}

static void parser_result_reset(parser_t* p)
{
	DEBUG_ASSERT(p != NULL);

	// TODO bummer
	for (prop_t* prop = p->result.props; prop != NULL; prop = prop->next)
		slab_put(p->pslab, prop);

	log_init(&p->result);
}

presult_t parser_parse(parser_t* p, char* chunk, size_t clen)
{
	DEBUG_ASSERT(p != NULL);
	DEBUG_ASSERT(chunk != NULL);
	DEBUG_ASSERT(clen != 0);

	p->bstart = chunk;
	p->blen = 0;
	parser_result_reset(p);

	while (clen > 0) {
		char next = *chunk;
		chunk++;

		switch (next) {
		case '\n':
			p->state = DATE_PSTATE;
			return (presult_t){true, chunk};
		default:
			break;
		}

		clen--;
	}

	return (presult_t){false, chunk};
}
