#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./config.h"
#include "./parser.h"
#include "./util.h"

#define TRIM_SPACES(p, SET_MACRO, label, END_MACRO)                            \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		END_MACRO(p);                                                          \
		return p->res;                                                         \
	case '\t':                                                                 \
	case ' ':                                                                  \
		SKIP(p);                                                               \
		break;                                                                 \
	default:                                                                   \
		SET_MACRO(p, p->chunk);                                                \
		p->state++;                                                            \
		goto label;                                                            \
	}

#define ADD_PROP(p)                                                            \
	p->pslab[p->pnext].next = p->result.props;                                 \
	p->result.props = &p->pslab[p->pnext];                                     \
	p->pnext++;

#define TRY_ADD_PROP(p)                                                        \
	if (p->pnext == LOGD_SLAB_CAP) {                                         \
		parser_error(                                                          \
		  p, "reached max number of log properties: " STR(LOGD_SLAB_CAP));   \
		continue;                                                              \
	}                                                                          \
	ADD_PROP(p);

#define SET_VALUE(p, chunk) p->result.props->value = chunk;

#define SET_KEY(p, chunk) p->result.props->key = chunk;

#define COMMIT(p)                                                              \
	(p)->chunk[(p)->blen] = 0;                                                 \
	(p)->chunk += (p)->blen + 1;                                               \
	(p)->blen = 0;

#define SKIP(p) (p)->chunk++;

#define REMOVE_PROP(p)                                                         \
	p->result.props = p->result.props->next;                                   \
	p->pnext--;

#define PARSER_END_ERROR(p)                                                    \
	p->res.type = PARSE_ERROR;                                                 \
	COMMIT(p);                                                                 \
	DEBUG_ASSERT(p->res.error.msg != NULL);                                    \
	DEBUG_ASSERT(p->res.error.at != NULL);

#define PARSER_END_ERROR_INCOMPLETE(p)                                         \
	parser_error(p, "incomplete header");                                      \
	SET_VALUE(p, "");                                                          \
	PARSER_END_ERROR(p);

#define PARSE_END_MESSAGE(p)                                                   \
	SET_KEY(p, KEY_MESSAGE);                                                   \
	SET_VALUE(p, p->chunk);                                                    \
	COMMIT(p);                                                                 \
	p->res.type = PARSE_COMPLETE;

#define PARSE_END(p)                                                           \
	SET_VALUE(p, p->chunk);                                                    \
	COMMIT(p);                                                                 \
	p->res.type = PARSE_COMPLETE;

#define PARSER_PARSE_NEXT_KEY(p)                                               \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		PARSE_END_MESSAGE(p);                                                  \
		return p->res;                                                         \
	case ':':                                                                  \
	case '\x00': /* re-submitted partial data */                               \
		COMMIT(p);                                                             \
		p->state = TRANSITIONVALUE_PSTATE;                                     \
		break;                                                                 \
	default:                                                                   \
		p->blen++;                                                             \
		break;                                                                 \
	}

#define PARSER_PARSE_NEXT_VALUE(p)                                             \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		PARSE_END(p);                                                          \
		return p->res;                                                         \
	case ',':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		TRY_ADD_PROP(p);                                                       \
		p->state = TRANSITIONKEY_PSTATE;                                       \
		break;                                                                 \
	default:                                                                   \
		p->blen++;                                                             \
		break;                                                                 \
	}

#define PARSER_PARSE_NEXT_DATE(p, next_key)                                    \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		PARSER_END_ERROR_INCOMPLETE(p);                                        \
		return p->res;                                                         \
	case '[':                                                                  \
		SKIP(p);                                                               \
		SET_VALUE(p, p->chunk);                                                \
		break;                                                                 \
	case '0':                                                                  \
	case '1':                                                                  \
	case '2':                                                                  \
	case '3':                                                                  \
	case '4':                                                                  \
	case '5':                                                                  \
	case '6':                                                                  \
	case '7':                                                                  \
	case '8':                                                                  \
	case '9':                                                                  \
	case ':':                                                                  \
	case ',':                                                                  \
	case '.':                                                                  \
	case '-':                                                                  \
	case 'Z':                                                                  \
	case '+':                                                                  \
		p->blen++;                                                             \
		break;                                                                 \
	case 'T':                                                                  \
	case ' ':                                                                  \
	case ']':                                                                  \
	case '\t':                                                                 \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		ADD_PROP(p);                                                           \
		SET_KEY(p, next_key);                                                  \
		SET_VALUE(p, p->chunk);                                                \
		p->state++;                                                            \
		break;                                                                 \
	default:                                                                   \
		parser_error(p, "invalid date or time in log header");                 \
		REMOVE_PROP(p);                                                        \
		p->blen++;                                                             \
		break;                                                                 \
	}

#define PARSER_PARSE_NEXT_HEADER(p, next_key)                                  \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		PARSER_END_ERROR_INCOMPLETE(p);                                        \
		return p->res;                                                         \
	case '[':                                                                  \
		SKIP(p);                                                               \
		SET_VALUE(p, p->chunk);                                                \
		break;                                                                 \
	case '\t':                                                                 \
	case ' ':                                                                  \
	case ']':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		ADD_PROP(p);                                                           \
		SET_KEY(p, next_key);                                                  \
		p->state++;                                                            \
		break;                                                                 \
	default:                                                                   \
		p->blen++;                                                             \
		break;                                                                 \
	}

#define PARSE_PARSE_NEXT_THREAD_BRACKET(p)                                     \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		PARSER_END_ERROR_INCOMPLETE(p);                                        \
		return p->res;                                                         \
	case ']':                                                                  \
	case '\t':                                                                 \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		ADD_PROP(p);                                                           \
		SET_KEY(p, KEY_CLASS);                                                 \
		p->state = TRANSITIONCLASS_PSTATE;                                     \
		break;                                                                 \
	default:                                                                   \
		p->blen++;                                                             \
		break;                                                                 \
	}

#define PARSER_PARSE_NEXT_THREAD(p)                                            \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		PARSER_END_ERROR_INCOMPLETE(p);                                        \
		return p->res;                                                         \
	case '[':                                                                  \
		p->state = THREADBRACKET_PSTATE;                                       \
		SKIP(p);                                                               \
		SET_VALUE(p, p->chunk);                                                \
		break;                                                                 \
	default:                                                                   \
		p->state = THREADNOBRACKET_PSTATE;                                     \
		PARSER_PARSE_NEXT_HEADER(p, KEY_CLASS);                                \
		break;                                                                 \
	}

#define PARSER_PARSE_NEXT_CALLTYPE(p)                                          \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		PARSE_END_MESSAGE(p);                                                  \
		return p->res;                                                         \
	case ':':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		ADD_PROP(p);                                                           \
		p->state++;                                                            \
		break;                                                                 \
	default:                                                                   \
		p->blen++;                                                             \
		break;                                                                 \
	}

#define PARSER_PARSE_VERIFY_CALLTYPE(p)                                        \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		PARSE_END_MESSAGE(p);                                                  \
		return p->res;                                                         \
	case ',':                                                                  \
		p->result.props->next->key = p->result.props->next->value;             \
		p->result.props->next->value = p->chunk;                               \
		REMOVE_PROP(p);                                                        \
		PARSER_PARSE_NEXT_VALUE(p);                                            \
		break;                                                                 \
	default:                                                                   \
		PARSER_PARSE_NEXT_KEY(p);                                              \
		break;                                                                 \
	}

#define PARSER_PARSE_ERROR(p)                                                  \
	switch (p->token) {                                                        \
	case '\n':                                                                 \
		PARSER_END_ERROR(p);                                                   \
		return p->res;                                                         \
	default:                                                                   \
		p->blen++;                                                             \
		break;                                                                 \
	}

INLINE static void parser_error(parser_t* p, const char* msg)
{
	p->res.error.msg = msg;
	p->res.error.at = p->chunk;
	p->state = ERROR_PSTATE;
}

parser_t* parser_create()
{
	parser_t* p;

	if ((p = calloc(1, sizeof(parser_t))) == NULL) {
		perror("calloc");
		return NULL;
	}

	prop_t* slab = calloc(LOGD_SLAB_CAP, sizeof(prop_t));
	parser_init(p, slab);

	return p;
}

INLINE void parser_reset(parser_t* p)
{
	DEBUG_ASSERT(p != NULL);

	p->state = INIT_PSTATE;
	p->pnext = 0;
	memset(p->pslab, 0, LOGD_SLAB_CAP);
	log_init(&p->result);

	p->res.type = PARSE_PARTIAL;

	ADD_PROP(p);
	SET_KEY(p, KEY_DATE);
}

void parser_init(parser_t* p, prop_t* pslab)
{
	DEBUG_ASSERT(p != NULL);

	p->pslab = pslab;
	p->res.log = &p->result;

	parser_reset(p);
}

void parser_free(parser_t* p)
{
	if (p == NULL)
		return;

	free(p->pslab);
	free(p);
}

INLINE parse_res_t parser_parse(parser_t* p, char* chunk, size_t clen)
{
	DEBUG_ASSERT(p != NULL);
	DEBUG_ASSERT(chunk != NULL);

	p->chunk = chunk;
	p->blen = 0;
	p->res.consumed = 0;

	while (p->res.consumed < clen) {
		p->token = chunk[p->res.consumed++];
		switch (p->state) {
		case INIT_PSTATE:
			TRIM_SPACES(p, SET_VALUE, date, PARSER_END_ERROR_INCOMPLETE);
			break;
		case DATE_PSTATE:
		date:
			PARSER_PARSE_NEXT_DATE(p, KEY_TIME);
			break;
		case TIME_PSTATE:
			PARSER_PARSE_NEXT_DATE(p, KEY_LEVEL);
			break;
		case LEVEL_PSTATE:
		level:
			PARSER_PARSE_NEXT_HEADER(p, KEY_THREAD);
			break;
		case THREAD_PSTATE:
		thread:
			PARSER_PARSE_NEXT_THREAD(p);
			break;
		case THREADBRACKET_PSTATE:
			PARSE_PARSE_NEXT_THREAD_BRACKET(p);
			break;
		case THREADNOBRACKET_PSTATE:
			PARSER_PARSE_NEXT_HEADER(p, KEY_CLASS);
			break;
		case CLASS_PSTATE:
		clazz:
			PARSER_PARSE_NEXT_HEADER(p, KEY_CALLTYPE);
			break;
		case CALLTYPE_PSTATE:
		calltype:
			PARSER_PARSE_NEXT_CALLTYPE(p);
			break;
		case VERIFYCALLTYPE_PSTATE:
		verifycalltype:
			PARSER_PARSE_VERIFY_CALLTYPE(p);
			break;
		case TRANSITIONVERIFYCALLTYPE_PSTATE:
			TRIM_SPACES(p, SET_KEY, verifycalltype, PARSE_END_MESSAGE);
			break;
		case TRANSITIONKEY_PSTATE:
			TRIM_SPACES(p, SET_KEY, key, PARSE_END_MESSAGE);
			break;
		key:
		case KEY_PSTATE:
			PARSER_PARSE_NEXT_KEY(p);
			break;
		case VALUE_PSTATE:
		value:
			PARSER_PARSE_NEXT_VALUE(p);
			break;
		case TRANSITIONLEVEL_PSTATE:
			TRIM_SPACES(p, SET_VALUE, level, PARSER_END_ERROR_INCOMPLETE);
			break;
		case TRANSITIONTHREAD_PSTATE:
			TRIM_SPACES(p, SET_VALUE, thread, PARSER_END_ERROR_INCOMPLETE);
			break;
		case TRANSITIONCALLTYPE_PSTATE:
			TRIM_SPACES(p, SET_VALUE, calltype, PARSE_END_MESSAGE);
			break;
		case TRANSITIONCLASS_PSTATE:
			TRIM_SPACES(p, SET_VALUE, clazz, PARSER_END_ERROR_INCOMPLETE);
			break;
		case TRANSITIONVALUE_PSTATE:
			TRIM_SPACES(p, SET_VALUE, value, PARSE_END);
			break;
		/* skip input until newline is found and state is reset */
		case ERROR_PSTATE:
			PARSER_PARSE_ERROR(p);
			break;
		}
	}

	return p->res;
}
