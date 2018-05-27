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

#define SKIP_KEY(p)                                                            \
	(p)->chunk++;                                                              \
	SET_KEY(p, (p)->chunk)

#define SKIP_VALUE(p)                                                          \
	(p)->chunk++;                                                              \
	SET_VALUE(p, (p)->chunk)

#define REMOVE_PROP(p)                                                         \
	p->result.props = p->result.props->next;                                   \
	p->pnext--;

INLINE static void parser_clean_result(parser_t* p)
{
	switch (p->state) {
	/* invalid prop key */
	case INIT_PSTATE:
	case VERIFYCALLTYPE_PSTATE:
	case KEY_PSTATE:
		REMOVE_PROP(p);
		break;
	/* invalid prop value */
	case DATE_PSTATE:
	case TIME_PSTATE:
	case LEVEL_PSTATE:
	case THREAD_PSTATE:
	case THREADBRACKET_PSTATE:
	case THREADNOBRACKET_PSTATE:
	case CLASS_PSTATE:
	case CALLTYPE_PSTATE:
	case VALUE_PSTATE:
	case TRANSITIONLEVEL_PSTATE:
	case TRANSITIONTHREAD_PSTATE:
	case TRANSITIONCALLTYPE_PSTATE:
	case TRANSITIONCLASS_PSTATE:
		p->result.props->value = NULL;
		break;
	case ERROR_PSTATE:
		abort(); /* not possible */
	}
}

INLINE static void parser_parse_error(parser_t* p, const char* msg)
{
	p->res.error.msg = msg;
	p->res.error.remaining = p->chunk;
	parser_clean_result(p);
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
	/* trim left spaces */
	case ' ':
	case '\t':
		if (p->blen == 0) {
			SKIP_KEY(p);
		} else {
			p->blen++;
		}
		break;
	case ':':
	case '\x00': /* re-submitted partial data */
		COMMIT_KEY(p);
		SET_VALUE(p, p->chunk);
		p->state = VALUE_PSTATE;
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
		SET_KEY(p, p->chunk);
		p->state = KEY_PSTATE;
		break;
	/* trim left spaces */
	case ' ':
	case '\t':
		if (p->blen == 0) {
			SKIP_VALUE(p);
		} else {
			p->blen++;
		}
		break;
	default:
		p->blen++;
		break;
	}
}

INLINE static bool parser_parse_skip(parser_t* p)
{
	switch (p->token) {
	case '\t':
	case ' ':
		SKIP_VALUE(p);
		return true;
	default:
		p->state++;
		return false;
	}
}

INLINE static void parser_parse_next_date(parser_t* p, const char* next_key)
{
	switch (p->token) {
	case '[':
		SKIP_VALUE(p);
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
		p->blen++;
		break;
	}
}

INLINE static void parser_parse_next_header(parser_t* p, const char* next_key)
{
	switch (p->token) {
	case '[':
		SKIP_VALUE(p);
		break;
	case '\t':
	case ' ':
	case ']':
	case '\x00':
		COMMIT_VALUE(p);
		TRY_ADD_PROP(p);
		SET_KEY(p, next_key);
		SET_VALUE(p, p->chunk);
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
		SET_VALUE(p, p->chunk);
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
		SKIP_VALUE(p);
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
		SET_KEY(p, p->chunk);
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
	DEBUG_ASSERT(p->result.props != NULL);

	switch (p->state) {
	case INIT_PSTATE:
	case DATE_PSTATE:
	case TIME_PSTATE:
	case LEVEL_PSTATE:
	case THREAD_PSTATE:
	case THREADBRACKET_PSTATE:
	case THREADNOBRACKET_PSTATE:
	case CLASS_PSTATE:
		parser_parse_error(p, "incomplete header");
		/* fallthrough */
	case ERROR_PSTATE:
		p->res.type = PARSE_ERROR;
		COMMIT_VALUE(p);
		DEBUG_ASSERT(p->res.error.msg != NULL);
		DEBUG_ASSERT(p->res.error.remaining != NULL);
		return;
	case CALLTYPE_PSTATE:
	case VERIFYCALLTYPE_PSTATE:
	case KEY_PSTATE:
		SET_KEY(p, KEY_MESSAGE);
		SET_VALUE(p, p->chunk);
		COMMIT_VALUE(p);
		break;
	case VALUE_PSTATE:
	case TRANSITIONLEVEL_PSTATE:
	case TRANSITIONTHREAD_PSTATE:
	case TRANSITIONCALLTYPE_PSTATE:
	case TRANSITIONCLASS_PSTATE:
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
		parse:
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
				parser_parse_next_header(p, KEY_THREAD);
				break;
			case THREAD_PSTATE:
				parser_parse_next_thread(p);
				break;
			case THREADBRACKET_PSTATE:
				parser_parse_next_thread_bracket(p);
				break;
			case THREADNOBRACKET_PSTATE:
				parser_parse_next_header(p, KEY_CLASS);
				break;
			case CLASS_PSTATE:
				parser_parse_next_header(p, KEY_CALLTYPE);
				break;
			case CALLTYPE_PSTATE:
				parser_parse_next_calltype(p);
				break;
			case VERIFYCALLTYPE_PSTATE:
				parser_parse_verify_calltype(p);
				break;
			case KEY_PSTATE:
				parser_parse_next_key(p);
				break;
			case VALUE_PSTATE:
				parser_parse_next_value(p);
				break;
			case TRANSITIONLEVEL_PSTATE:
			case TRANSITIONTHREAD_PSTATE:
			case TRANSITIONCALLTYPE_PSTATE:
			case TRANSITIONCLASS_PSTATE:
				if (!parser_parse_skip(p)) {
					goto parse;
				}
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
