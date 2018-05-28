#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "util.h"

#ifdef LOGD_INLINE
#define INLINE inline
#else
#define INLINE
#endif

#define TRIM_SPACES(p, SET_MACRO, label)                                       \
	switch (p->token) {                                                        \
	case '\t':                                                                 \
	case ' ':                                                                  \
		SKIP(p);                                                               \
		break;                                                                 \
	default:                                                                   \
		SET_MACRO(p, p->chunk);                                                \
		p->state++;                                                            \
		goto label;                                                            \
	}

#define TRY_ADD_PROP(p)                                                        \
	if (p->pnext == PARSER_SLAB_CAP) {                                         \
		parser_parse_error(                                                    \
		  p, "reached max number of log properties: " STR(PARSER_SLAB_CAP));   \
		return;                                                                \
	}                                                                          \
	p->pslab[p->pnext].next = p->result.props;                                 \
	p->result.props = &p->pslab[p->pnext];                                     \
	p->pnext++;

#define SET_VALUE(p, chunk) p->result.props->value = chunk;

#define SET_KEY(p, chunk) p->result.props->key = chunk;

#define COMMIT_OFFSET(p)                                                       \
	(p)->chunk += (p)->blen + 1;                                               \
	(p)->blen = 0;

#define COMMIT_KEY(p)                                                          \
	(p)->chunk[(p)->blen] = 0;                                                 \
	COMMIT_OFFSET(p);

#define COMMIT_VALUE(p)                                                        \
	(p)->chunk[(p)->blen] = 0;                                                 \
	COMMIT_OFFSET(p);

#define SKIP(p) (p)->chunk++;

#define REMOVE_PROP(p)                                                         \
	p->result.props = p->result.props->next;                                   \
	p->pnext--;

INLINE static void parser_parse_error(parser_t* p, const char* msg)
{
	p->res.error.msg = msg;
	p->res.error.remaining = p->chunk;
	p->state = ERROR_PSTATE;
}

parser_t* parser_create()
{
	parser_t* p;

	if ((p = calloc(1, sizeof(parser_t))) == NULL) {
		perror("calloc");
		return NULL;
	}

	prop_t* slab = calloc(PARSER_SLAB_CAP, sizeof(prop_t));
	parser_init(p, slab);

	return p;
}

INLINE void parser_reset(parser_t* p)
{
	DEBUG_ASSERT(p != NULL);

	p->state = INIT_PSTATE;
	p->pnext = 0;
	memset(p->pslab, 0, PARSER_SLAB_CAP);
	log_init(&p->result);

	p->res.type = PARSE_PARTIAL;

	TRY_ADD_PROP(p);
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

INLINE static void parser_parse_next_key(parser_t* p)
{
	switch (p->token) {
	case ':':
	case '\x00': /* re-submitted partial data */
		COMMIT_KEY(p);
		p->state = TRANSITIONVALUE_PSTATE;
		break;
	default:
		p->blen++;
		break;
	}
}

INLINE static void parser_parse_next_value(parser_t* p)
{
	switch (p->token) {
	case ',':
	case '\x00':
		COMMIT_VALUE(p);
		TRY_ADD_PROP(p);
		p->state = TRANSITIONKEY_PSTATE;
		break;
	default:
		p->blen++;
		break;
	}
}

INLINE static void parser_parse_next_date(parser_t* p, const char* next_key)
{
	switch (p->token) {
	case '[':
		SKIP(p);
		SET_VALUE(p, p->chunk);
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
	case 'Z':
	case '+':
		p->blen++;
		break;
	case 'T':
	case ' ':
	case ']':
	case '\t':
	case '\x00':
		COMMIT_VALUE(p);
		TRY_ADD_PROP(p);
		SET_KEY(p, next_key);
		SET_VALUE(p, p->chunk);
		p->state++;
		break;
	default:
		parser_parse_error(p, "invalid date or time in log header");
		REMOVE_PROP(p);
		p->blen++;
		break;
	}
}

INLINE static void parser_parse_next_header(parser_t* p, const char* next_key)
{
	switch (p->token) {
	case '[':
		SKIP(p);
		SET_VALUE(p, p->chunk);
		break;
	case '\t':
	case ' ':
	case ']':
	case '\x00':
		COMMIT_VALUE(p);
		TRY_ADD_PROP(p);
		SET_KEY(p, next_key);
		p->state++;
		break;
	default:
		p->blen++;
		break;
	}
}

INLINE static void parser_parse_next_thread_bracket(parser_t* p)
{
	switch (p->token) {
	case ']':
	case '\t':
	case '\x00':
		COMMIT_VALUE(p);
		TRY_ADD_PROP(p);
		SET_KEY(p, KEY_CLASS);
		p->state = TRANSITIONCLASS_PSTATE;
		break;
	default:
		p->blen++;
		break;
	}
}

INLINE static void parser_parse_next_thread(parser_t* p)
{
	switch (p->token) {
	case '[':
		p->state = THREADBRACKET_PSTATE;
		SKIP(p);
		SET_VALUE(p, p->chunk);
		break;
	default:
		p->state = THREADNOBRACKET_PSTATE;
		parser_parse_next_header(p, KEY_CLASS);
		break;
	}
}

INLINE static void parser_parse_next_calltype(parser_t* p)
{
	switch (p->token) {
	case ':':
	case '\x00':
		COMMIT_VALUE(p);
		TRY_ADD_PROP(p);
		p->state++;
		break;
	default:
		p->blen++;
		break;
	}
}

INLINE static void parser_parse_verify_calltype(parser_t* p)
{
	switch (p->token) {
	case ',':
		p->result.props->next->key = p->result.props->next->value;
		p->result.props->next->value = p->chunk;
		REMOVE_PROP(p);
		parser_parse_next_value(p);
		break;
	default:
		parser_parse_next_key(p);
		break;
	}
}

INLINE static void parser_parse_end(parser_t* p)
{
	switch (p->state) {
	case INIT_PSTATE:
	case DATE_PSTATE:
	case TIME_PSTATE:
	case LEVEL_PSTATE:
	case THREAD_PSTATE:
	case THREADBRACKET_PSTATE:
	case THREADNOBRACKET_PSTATE:
	case TRANSITIONLEVEL_PSTATE:
	case TRANSITIONTHREAD_PSTATE:
	case TRANSITIONCALLTYPE_PSTATE:
	case TRANSITIONCLASS_PSTATE:
	case CLASS_PSTATE:
		parser_parse_error(p, "incomplete header");
		p->result.props = NULL;
		/* fallthrough */
	case ERROR_PSTATE:
		p->res.type = PARSE_ERROR;
		COMMIT_VALUE(p); // null terminator for error's remaining
		DEBUG_ASSERT(p->res.error.msg != NULL);
		DEBUG_ASSERT(p->res.error.remaining != NULL);
		return;
	case CALLTYPE_PSTATE:
	case VERIFYCALLTYPE_PSTATE:
	case TRANSITIONVERIFYCALLTYPE_PSTATE:
	case TRANSITIONKEY_PSTATE:
	case KEY_PSTATE:
		SET_KEY(p, KEY_MESSAGE);
		SET_VALUE(p, p->chunk);
		COMMIT_VALUE(p);
		break;
	case TRANSITIONVALUE_PSTATE:
	case VALUE_PSTATE:
		SET_VALUE(p, p->chunk);
		COMMIT_VALUE(p);
		break;
	}

	p->res.type = PARSE_COMPLETE;
}

INLINE parse_res_t parser_parse(parser_t* p, char* chunk, size_t clen)
{
	DEBUG_ASSERT(p != NULL);
	DEBUG_ASSERT(chunk != NULL);

	p->chunk = chunk;
	p->blen = 0;

	for (p->res.consumed = 0; clen > 0; clen--) {
		p->token = chunk[p->res.consumed++];
		switch (p->token) {
		case '\n':
			parser_parse_end(p);
			return p->res;
		default:
			switch (p->state) {
			case INIT_PSTATE:
				SET_VALUE(p, chunk);
				p->state++;
				/* fallthrough */
			case DATE_PSTATE:
				parser_parse_next_date(p, KEY_TIME);
				break;
			case TIME_PSTATE:
				parser_parse_next_date(p, KEY_LEVEL);
				break;
			case LEVEL_PSTATE:
			level:
				parser_parse_next_header(p, KEY_THREAD);
				break;
			case THREAD_PSTATE:
			thread:
				parser_parse_next_thread(p);
				break;
			case THREADBRACKET_PSTATE:
				parser_parse_next_thread_bracket(p);
				break;
			case THREADNOBRACKET_PSTATE:
				parser_parse_next_header(p, KEY_CLASS);
				break;
			case CLASS_PSTATE:
			clazz:
				parser_parse_next_header(p, KEY_CALLTYPE);
				break;
			case CALLTYPE_PSTATE:
			calltype:
				parser_parse_next_calltype(p);
				break;
			case VERIFYCALLTYPE_PSTATE:
			verifycalltype:
				parser_parse_verify_calltype(p);
				break;
			case TRANSITIONVERIFYCALLTYPE_PSTATE:
				TRIM_SPACES(p, SET_KEY, verifycalltype);
				break;
			case TRANSITIONKEY_PSTATE:
				TRIM_SPACES(p, SET_KEY, key);
				break;
			key:
			case KEY_PSTATE:
				parser_parse_next_key(p);
				break;
			case VALUE_PSTATE:
			value:
				parser_parse_next_value(p);
				break;
			case TRANSITIONLEVEL_PSTATE:
				TRIM_SPACES(p, SET_VALUE, level);
				break;
			case TRANSITIONTHREAD_PSTATE:
				TRIM_SPACES(p, SET_VALUE, thread);
				break;
			case TRANSITIONCALLTYPE_PSTATE:
				TRIM_SPACES(p, SET_VALUE, calltype);
				break;
			case TRANSITIONCLASS_PSTATE:
				TRIM_SPACES(p, SET_VALUE, clazz);
				break;
			case TRANSITIONVALUE_PSTATE:
				TRIM_SPACES(p, SET_VALUE, value);
				break;
			/* skip input until newline is found and state is reset */
			case ERROR_PSTATE:
				p->blen++;
				break;
			}
			break;
		}
	}

	return p->res;
}
