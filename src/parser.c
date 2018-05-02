#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "parser.h"

#ifndef PARSER_SLAB_CAP
#define PARSER_SLAB_CAP 100
#endif

#define APPEND_NEW_PROP(p, prop, key)                                          \
	prop = slab_get(p->pslab);                                                 \
	if (prop == NULL) {                                                        \
		parser_parse_error(p, "slab_get()");                                   \
		return;                                                                \
	}                                                                          \
	log_set(&p->result, prop, key, NULL);

#define RESET_OFFSET(p)                                                        \
	p->bstart += p->blen + 1;                                                  \
	p->blen = 0;

#define CONSUME_KEY(p)                                                         \
	p->bstart[p->blen] = 0;                                                    \
	p->result.props->key = p->bstart;                                          \
	RESET_OFFSET(p);

#define CONSUME_VALUE(p)                                                       \
	p->bstart[p->blen] = 0;                                                    \
	p->result.props->value = p->bstart;                                        \
	RESET_OFFSET(p);

#define SKIP(p) p->bstart++;

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

	for (prop_t* prop = p->result.props; prop != NULL; prop = prop->next) {
		prop->next = NULL;
		prop->key = NULL;
		prop->value = NULL;
		slab_put(p->pslab, prop);
	}

	log_init(&p->result);
}

void parser_parse_error(parser_t* p, const char* msg)
{
	p->state = ERROR_PSTATE;
}

void parser_parse_next_key(parser_t* p, char r)
{
	prop_t* prop;

	switch (r) {
	case ',':
		parser_parse_error(p, "unexpected double ',' character");
		break;
	case ' ':
		// trim left spaces
		if (p->blen == 0) {
			SKIP(p);
		}
		break;
	case ':':
		APPEND_NEW_PROP(p, prop, NULL);
		CONSUME_KEY(p);
		p->state = VALUE_PSTATE;
		break;
	default:
		p->blen++;
		break;
	}
}

void parser_parse_next_multikey(parser_t* p, char r)
{
	switch (r) {
	case ',':
		parser_parse_error(p, "unexpected double ',' character");
		break;
	case ' ':
	case '\t':
		// backtrack, clean first key and skip space
		p->blen--;
		CONSUME_KEY(p);
		p->bstart++;
		p->state = VALUE_PSTATE;
		break;
	default:
		// reset state as char:char is valid
		p->state = VALUE_PSTATE;
		p->blen++;
		break;
	}
}

void parser_parse_next_value(parser_t* p, char r)
{
	switch (r) {
	case ',':
		CONSUME_VALUE(p);
		p->state = KEY_PSTATE;
		break;
	case ':':
		p->state = MULTIKEY_PSTATE;
		p->blen++;
		break;
	case ' ':
	case '\t':
		// trim left spaces
		if (p->blen == 0) {
			SKIP(p);
		} else {
			p->blen++;
		}
		break;
	default:
		p->blen++;
		break;
	}
}

bool parser_parse_skip(parser_t* p, char r)
{
	switch (r) {
	case '\t':
	case ' ':
		SKIP(p);
		return true;
	default:
		p->state++;
		return false;
	}
}

void parser_parse_next_date(parser_t* p, const char* key, char r)
{
	prop_t* prop;

	switch (r) {
	case '[':
		SKIP(p);
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case ':':
	case ',':
	case '.':
	case '-':
		p->blen++;
		break;
	default:
		APPEND_NEW_PROP(p, prop, key);
		CONSUME_VALUE(p);
		p->state++;
		break;
	}
}

void parser_parse_next_header(parser_t* p, const char* key, char r)
{
	prop_t* prop;

	switch (r) {
	case '[':
		SKIP(p);
		break;
	case '\t':
	case ' ':
	case ']':
		APPEND_NEW_PROP(p, prop, key);
		CONSUME_VALUE(p);
		p->state++;
		break;
	default:
		p->blen++;
		break;
	}
}

void parser_parse_next_thread_bracket(parser_t* p, char r)
{
	prop_t* prop;

	switch (r) {
	case ']':
	case '\t':
		APPEND_NEW_PROP(p, prop, KEY_THREAD);
		CONSUME_VALUE(p);
		p->state = TRANSITIONCLASS_PSTATE;
		break;
	default:
		p->blen++;
		break;
	}
}

void parser_parse_next_thread(parser_t* p, char r)
{
	switch (r) {
	case '[':
		p->state = THREADBRACKET_PSTATE;
		SKIP(p);
		break;
	default:
		p->state = THREADNOBRACKET_PSTATE;
		parser_parse_next_header(p, KEY_THREAD, r);
		break;
	}
}

void parser_parse_next_calltype(parser_t* p, char r)
{
	prop_t* prop;

	switch (r) {
	case ':':
		APPEND_NEW_PROP(p, prop, KEY_CALLTYPE);
		CONSUME_VALUE(p);
		p->state++;
		break;
	default:
		p->blen++;
		break;
	}
}

void parser_parse_verify_calltype(parser_t* p, char r)
{
	switch (r) {
	case ',':
		p->result.props->key = p->result.props->value;
		parser_parse_next_value(p, r);
		break;
	default:
		parser_parse_next_key(p, r);
		break;
	}
}

void parser_parse_next(parser_t* p, char r)
{
	switch (p->state) {
	case DATE_PSTATE:
		parser_parse_next_date(p, KEY_DATE, r);
		break;
	case TIME_PSTATE:
		parser_parse_next_date(p, KEY_TIME, r);
		break;
	case LEVEL_PSTATE:
		parser_parse_next_header(p, KEY_LEVEL, r);
		break;
	case THREAD_PSTATE:
		parser_parse_next_thread(p, r);
		break;
	case THREADBRACKET_PSTATE:
		parser_parse_next_thread_bracket(p, r);
		break;
	case THREADNOBRACKET_PSTATE:
		parser_parse_next_header(p, KEY_THREAD, r);
		break;
	case CLASS_PSTATE:
		parser_parse_next_header(p, KEY_CLASS, r);
		break;
	case CALLTYPE_PSTATE:
		parser_parse_next_calltype(p, r);
		break;
	case VERIFYCALLTYPE_PSTATE:
		parser_parse_verify_calltype(p, r);
		break;
	case KEY_PSTATE:
		parser_parse_next_key(p, r);
		break;
	case VALUE_PSTATE:
		parser_parse_next_value(p, r);
		break;
	case MULTIKEY_PSTATE:
		parser_parse_next_multikey(p, r);
		break;
	case TRANSITIONLEVEL_PSTATE:
	case TRANSITIONTHREAD_PSTATE:
	case TRANSITIONCALLTYPE_PSTATE:
	case TRANSITIONCLASS_PSTATE:
		if (!parser_parse_skip(p, r)) {
			parser_parse_next(p, r);
		}
		break;
	case ERROR_PSTATE:
		// ignore until newline is found and state is reset
		break;
	}
}

void parser_parse_end(parser_t* p)
{
	prop_t* prop;

	if (p->result.props != NULL && p->result.props->value == NULL) {
		CONSUME_VALUE(p);
	} else {
		APPEND_NEW_PROP(p, prop, KEY_MESSAGE);
		CONSUME_VALUE(p);
	}
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
			parser_parse_end(p);
			p->state = DATE_PSTATE;
			return (presult_t){true, chunk};
		default:
			parser_parse_next(p, next);
			break;
		}

		clen--;
	}

	return (presult_t){false, chunk};
}
