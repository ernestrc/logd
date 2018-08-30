#include "log.h"
#include "parser.h"
#include "stdio.h"
#include "string.h"
#include "util.h"

#define PARSER_ERROR_INVALID_KEY(p)                                            \
	REMOVE_PROP(p);                                                            \
	PARSER_SET_ERROR(p, "invalid log", ERROR_JSTATE);

#define PARSER_ERROR_INVALID_VAL(p)                                            \
	SET_VALUE(p, "");                                                          \
	PARSER_SET_ERROR(p, "invalid log value", ERROR_JSTATE);

#define PARSE_END(p) (p)->res.type = PARSE_COMPLETE;

typedef enum jstate_s {
	INIT_JSTATE,
	KEY_TRANS_JSTATE,
	KEY_STR_JSTATE,
	KEY_STR_ESCAPE_JSTATE,
	KEY_OTHER_JSTATE,
	VAL_SPLIT_JSTATE,
	VAL_TRANS_JSTATE,
	VAL_ARR_JSTATE,
	VAL_OBJ_JSTATE,
	VAL_ARR_ESCAPE_JSTATE,
	VAL_OBJ_ESCAPE_JSTATE,
	VAL_ARR_STR_ESCAPE_JSTATE,
	VAL_OBJ_STR_ESCAPE_JSTATE,
	VAL_NODE_COMMIT_JSTATE,
	VAL_OTHER_JSTATE,
	VAL_STR_JSTATE,
	VAL_STR_ESCAPE_JSTATE,
	ERROR_JSTATE,
} jstate_t;

typedef struct prop_parser_s {
	jstate_t state;
	log_t result;
	prop_t* pslab;
	int pnext, tnext;
	char token;
	char* chunk;
	int blen;
	int nest;
	parse_res_t res;
} prop_parser_t;

void parser_reset(void* _p)
{
	prop_parser_t* p = (prop_parser_t*)_p;

	DEBUG_ASSERT(p != NULL);

	p->state = INIT_JSTATE;
	p->nest = 0;

	LOGD_PARSER_RESET(p);
}

void parser_init(void* _p, prop_t* pslab)
{
	prop_parser_t* p = (prop_parser_t*)_p;

	DEBUG_ASSERT(p != NULL);

	LOGD_PARSER_INIT(p, pslab);
}

void parser_free(void* _p)
{
	prop_parser_t* p = (prop_parser_t*)_p;

	if (p == NULL)
		return;

	free(p->pslab);
	free(p);
}

void* parser_create()
{
	prop_parser_t* p = NULL;
	prop_t* pslab = NULL;

	if ((p = calloc(1, sizeof(prop_parser_t))) == NULL ||
	  (pslab = calloc(LOGD_SLAB_CAP, sizeof(prop_t))) == NULL) {
		perror("calloc");
		goto error;
	}

	parser_init(p, pslab);

	return (void*)p;

error:
	if (p)
		free(p);
	if (pslab)
		free(pslab);
	return NULL;
}

#define TRIM_OR(p)                                                             \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
	case '\t':                                                                 \
	case ' ':                                                                  \
		SKIP(p);                                                               \
		break;                                                                 \
	case '"':                                                                  \
		SKIP(p);                                                               \
		TRY_SET_NEW_KEY(p, (p)->chunk, ERROR_JSTATE);                          \
		(p)->state = KEY_STR_JSTATE;                                           \
		break;                                                                 \
	case '{':                                                                  \
		SKIP(p);                                                               \
		(p)->state = KEY_TRANS_JSTATE;                                         \
		break;                                                                 \
	default:                                                                   \
		TRY_SET_NEW_KEY(p, (p)->chunk, ERROR_JSTATE);                          \
		(p)->state = KEY_OTHER_JSTATE;                                         \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_KEY_TRANSITION(p)                                         \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		PARSE_END(p);                                                          \
		return (p)->res;                                                       \
	case '\t':                                                                 \
	case ' ':                                                                  \
	case ',':                                                                  \
		SKIP(p);                                                               \
		break;                                                                 \
	case '}':                                                                  \
		PARSE_END(p);                                                          \
		return (p)->res;                                                       \
	case '"':                                                                  \
		SKIP(p);                                                               \
		TRY_SET_NEW_KEY(p, (p)->chunk, ERROR_JSTATE);                          \
		(p)->state = KEY_STR_JSTATE;                                           \
		break;                                                                 \
	default:                                                                   \
		TRY_SET_NEW_KEY(p, (p)->chunk, ERROR_JSTATE);                          \
		(p)->state = KEY_OTHER_JSTATE;                                         \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_KEY_OTHER(p)                                              \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		PARSER_ERROR_INVALID_KEY(p);                                           \
		PARSER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	case ':':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		(p)->state = VAL_TRANS_JSTATE;                                         \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_KEY_STR(p)                                                \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		PARSER_ERROR_INVALID_KEY(p);                                           \
		PARSER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	case '\\':                                                                 \
		(p)->state = KEY_STR_ESCAPE_JSTATE;                                    \
		(p)->blen++;                                                           \
		break;                                                                 \
	case '"':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		(p)->state = VAL_SPLIT_JSTATE;                                         \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_VAL_SPLIT(p)                                              \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		PARSER_ERROR_INVALID_VAL(p);                                           \
		PARSER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	case '\t':                                                                 \
	case ' ':                                                                  \
		SKIP(p);                                                               \
		break;                                                                 \
	case ':':                                                                  \
		SKIP(p);                                                               \
		(p)->state = VAL_TRANS_JSTATE;                                         \
		break;                                                                 \
	default:                                                                   \
		PARSER_ERROR_INVALID_VAL(p);                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_VAL_TRANS(p)                                              \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SET_VALUE(p, "");                                                      \
		PARSE_END(p);                                                          \
		return (p)->res;                                                       \
	case '\t':                                                                 \
	case ' ':                                                                  \
		SKIP(p);                                                               \
		break;                                                                 \
	case '"':                                                                  \
		SKIP(p);                                                               \
		SET_VALUE(p, (p)->chunk);                                              \
		(p)->state = VAL_STR_JSTATE;                                           \
		break;                                                                 \
	case ',':                                                                  \
	case '\x00':                                                               \
		SKIP(p);                                                               \
		SET_VALUE(p, "");                                                      \
		(p)->state = KEY_TRANS_JSTATE;                                         \
		break;                                                                 \
	case '{':                                                                  \
		SET_VALUE(p, (p)->chunk);                                              \
		(p)->blen++;                                                           \
		(p)->state = VAL_OBJ_JSTATE;                                           \
		break;                                                                 \
	case '[':                                                                  \
		SET_VALUE(p, (p)->chunk);                                              \
		(p)->blen++;                                                           \
		(p)->state = VAL_ARR_JSTATE;                                           \
		break;                                                                 \
	default:                                                                   \
		SET_VALUE(p, (p)->chunk);                                              \
		(p)->blen++;                                                           \
		(p)->state = VAL_OTHER_JSTATE;                                         \
		break;                                                                 \
	}

#define PARSER_PARSE_VAL_OTHER(p)                                              \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
	case '}':                                                                  \
		COMMIT(p);                                                             \
		PARSE_END(p);                                                          \
		return (p)->res;                                                       \
	case ',':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		(p)->state = KEY_TRANS_JSTATE;                                         \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_VAL_STR(p)                                                \
	switch ((p)->token) {                                                      \
	case '\\':                                                                 \
		(p)->state = VAL_STR_ESCAPE_JSTATE;                                    \
		(p)->blen++;                                                           \
		break;                                                                 \
	case '\n':                                                                 \
		PARSER_ERROR_INVALID_VAL(p);                                           \
		PARSER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	case '"':                                                                  \
	case '\x00': /* re-submitted data */                                       \
		COMMIT(p);                                                             \
		(p)->state = KEY_TRANS_JSTATE;                                         \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_VAL_NODE_STR_SKIP(p, escaped_state, escape_escaped_state) \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		PARSER_ERROR_INVALID_VAL(p);                                           \
		PARSER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	case '\\':                                                                 \
		(p)->state = escape_escaped_state;                                     \
		(p)->blen++;                                                           \
		break;                                                                 \
	case '"':                                                                  \
		(p)->state = escaped_state;                                            \
		(p)->blen++;                                                           \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_HANDLE_ESCAPE(p, return_state)                            \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		PARSER_ERROR_INVALID_VAL(p);                                           \
		PARSER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	default:                                                                   \
		(p)->state = return_state;                                             \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_VAL_NODE_COMMIT(p)                                        \
	switch ((p)->token) {                                                      \
	case '}':                                                                  \
		COMMIT(p);                                                             \
		PARSE_END(p);                                                          \
		return (p)->res;                                                       \
	case '\n':                                                                 \
		PARSER_ERROR_INVALID_VAL(p);                                           \
		PARSER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	case '\t':                                                                 \
	case ' ':                                                                  \
	case ',':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		(p)->state = KEY_TRANS_JSTATE;                                         \
		break;                                                                 \
	default:                                                                   \
		PARSER_ERROR_INVALID_VAL(p);                                           \
		break;                                                                 \
	}

#define PARSER_PARSE_VAL_NODE(p, in, out, escape_state)                        \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		PARSER_ERROR_INVALID_VAL(p);                                           \
		PARSER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	case in:                                                                   \
		(p)->nest++;                                                           \
		(p)->blen++;                                                           \
		break;                                                                 \
	case '"':                                                                  \
		(p)->state = escape_state;                                             \
		(p)->blen++;                                                           \
		break;                                                                 \
	case out:                                                                  \
		(p)->blen++;                                                           \
		if ((p)->nest > 0) {                                                   \
			(p)->nest--;                                                       \
		} else {                                                               \
			(p)->state = VAL_NODE_COMMIT_JSTATE;                               \
		}                                                                      \
		break;                                                                 \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		(p)->state = KEY_TRANS_JSTATE;                                         \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

parse_res_t parser_parse(void* _p, char* chunk, size_t clen)
{
	prop_parser_t* p = (prop_parser_t*)_p;

	DEBUG_ASSERT(p != NULL);
	DEBUG_ASSERT(chunk != NULL);

	p->chunk = chunk;
	p->blen = 0;
	p->res.consumed = 0;

	while (p->res.consumed < clen) {
		p->token = chunk[p->res.consumed++];
		// printf("%c: %d -> ", p->token, p->state);
		switch (p->state) {
		case INIT_JSTATE:
			TRIM_OR(p);
			break;
		case KEY_TRANS_JSTATE:
			PARSER_PARSE_KEY_TRANSITION(p);
			break;
		case KEY_STR_JSTATE:
			PARSER_PARSE_KEY_STR(p);
			break;
		case KEY_STR_ESCAPE_JSTATE:
			PARSER_PARSE_HANDLE_ESCAPE(p, KEY_STR_JSTATE);
			break;
		case KEY_OTHER_JSTATE:
			PARSER_PARSE_KEY_OTHER(p);
			break;
		case VAL_SPLIT_JSTATE:
			PARSER_PARSE_VAL_SPLIT(p);
			break;
		case VAL_TRANS_JSTATE:
			PARSER_PARSE_VAL_TRANS(p);
			break;
		case VAL_OTHER_JSTATE:
			PARSER_PARSE_VAL_OTHER(p);
			break;
		case VAL_OBJ_JSTATE:
			PARSER_PARSE_VAL_NODE(p, '{', '}', VAL_OBJ_ESCAPE_JSTATE);
			break;
		case VAL_ARR_JSTATE:
			PARSER_PARSE_VAL_NODE(p, '[', ']', VAL_ARR_ESCAPE_JSTATE);
			break;
		case VAL_OBJ_ESCAPE_JSTATE:
			PARSER_PARSE_VAL_NODE_STR_SKIP(
			  p, VAL_OBJ_JSTATE, VAL_OBJ_STR_ESCAPE_JSTATE);
			break;
		case VAL_ARR_ESCAPE_JSTATE:
			PARSER_PARSE_VAL_NODE_STR_SKIP(
			  p, VAL_ARR_JSTATE, VAL_ARR_STR_ESCAPE_JSTATE);
			break;
		case VAL_OBJ_STR_ESCAPE_JSTATE:
			PARSER_PARSE_HANDLE_ESCAPE(p, VAL_OBJ_ESCAPE_JSTATE);
			break;
		case VAL_ARR_STR_ESCAPE_JSTATE:
			PARSER_PARSE_HANDLE_ESCAPE(p, VAL_ARR_ESCAPE_JSTATE);
			break;
		case VAL_NODE_COMMIT_JSTATE:
			PARSER_PARSE_VAL_NODE_COMMIT(p);
			break;
		case VAL_STR_JSTATE:
			PARSER_PARSE_VAL_STR(p);
			break;
		case VAL_STR_ESCAPE_JSTATE:
			PARSER_PARSE_HANDLE_ESCAPE(p, VAL_STR_JSTATE);
			break;
		/* skip input until newline is found and state is reset */
		case ERROR_JSTATE:
			PARSER_PARSE_ERROR(p);
			break;
		}
		// printf("%d\n", p->state);
	}

	return p->res;
}
