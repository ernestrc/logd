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

#define TRY_ADD_PROP(p, ctx)                                                   \
	if (p->pnext == PARSER_SLAB_CAP) {                                         \
		parser_parse_error(p, ctx,                                             \
		  "reached max number of log properties: " STR(PARSER_SLAB_CAP));      \
		return;                                                                \
	}                                                                          \
	p->pslab[p->pnext].next = p->result.props;                                 \
	p->result.props = &p->pslab[p->pnext];                                     \
	p->pnext++;

#define SET_VALUE(p, chunk) p->result.props->value = chunk;

#define SET_KEY(p, chunk) p->result.props->key = chunk;

#define COMMIT_OFFSET(ctx)                                                     \
	(ctx)->chunk += (ctx)->blen + 1;                                           \
	(ctx)->blen = 0;

#define COMMIT_KEY(ctx)                                                        \
	(ctx)->chunk[(ctx)->blen] = 0;                                             \
	COMMIT_OFFSET(ctx);

#define COMMIT_VALUE(ctx)                                                      \
	(ctx)->chunk[(ctx)->blen] = 0;                                             \
	COMMIT_OFFSET(ctx);

#define SKIP_KEY(p, ctx)                                                       \
	(ctx)->chunk++;                                                            \
	SET_KEY(p, (ctx)->chunk)

#define SKIP_VALUE(p, ctx)                                                     \
	(ctx)->chunk++;                                                            \
	SET_VALUE(p, (ctx)->chunk)

typedef struct parse_ctx_s {
	char token;
	char* chunk;
	int blen;
	const char* error;
} parse_ctx_t;

INLINE static void parser_parse_error(
  parser_t* p, parse_ctx_t* ctx, const char* msg)
{
	p->state = ERROR_PSTATE;
	ctx->error = msg;
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

	TRY_ADD_PROP(p, NULL);
	SET_KEY(p, KEY_DATE);
}

void parser_init(parser_t* p, prop_t* pslab)
{
	DEBUG_ASSERT(p != NULL);

	p->pslab = pslab;
	parser_reset(p);
}

void parser_free(parser_t* p)
{
	if (p == NULL)
		return;

	free(p->pslab);
	free(p);
}

INLINE static void parser_parse_next_key(parser_t* p, parse_ctx_t* ctx)
{
	switch (ctx->token) {
	/* trim left spaces */
	case ' ':
	case '\t':
		if (ctx->blen == 0) {
			SKIP_KEY(p, ctx);
		}
		break;
	case ':':
		COMMIT_KEY(ctx);
		SET_VALUE(p, ctx->chunk);
		p->state = VALUE_PSTATE;
		break;
	default:
		ctx->blen++;
		break;
	}
}

INLINE void parser_parse_next_multikey(parser_t* p, parse_ctx_t* ctx)
{
	switch (ctx->token) {
	case ' ':
	case '\t':
		/* backtrack, clean first key and skip space */
		SET_KEY(p, ctx->chunk);
		ctx->blen--;
		COMMIT_KEY(ctx);
		ctx->chunk++;
		SET_VALUE(p, ctx->chunk);
		p->state = VALUE_PSTATE;
		break;
	default:
		/* reset state as char:char is valid */
		p->state = VALUE_PSTATE;
		ctx->blen++;
		break;
	}
}

INLINE void parser_parse_next_value(parser_t* p, parse_ctx_t* ctx)
{
	switch (ctx->token) {
	case ',':
		COMMIT_VALUE(ctx);
		TRY_ADD_PROP(p, ctx);
		SET_KEY(p, ctx->chunk);
		p->state = KEY_PSTATE;
		break;
	case ':':
		p->state = MULTIKEY_PSTATE;
		ctx->blen++;
		break;
	/* trim left spaces */
	case ' ':
	case '\t':
		if (ctx->blen == 0) {
			SKIP_VALUE(p, ctx);
		} else {
			ctx->blen++;
		}
		break;
	default:
		ctx->blen++;
		break;
	}
}

INLINE bool parser_parse_skip(parser_t* p, parse_ctx_t* ctx)
{
	switch (ctx->token) {
	case '\t':
	case ' ':
		SKIP_VALUE(p, ctx);
		return true;
	default:
		p->state++;
		return false;
	}
}

INLINE void parser_parse_next_date(
  parser_t* p, parse_ctx_t* ctx, const char* next_key)
{
	switch (ctx->token) {
	case '[':
		SKIP_VALUE(p, ctx);
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
		ctx->blen++;
		break;
	case 'T':
	case ' ':
	case ']':
	case '\t':
		COMMIT_VALUE(ctx);
		TRY_ADD_PROP(p, ctx);
		SET_KEY(p, next_key);
		SET_VALUE(p, ctx->chunk);
		p->state++;
		break;
	default:
		parser_parse_error(p, ctx, "invalid date or time in log header");
		break;
	}
}

INLINE void parser_parse_next_header(
  parser_t* p, parse_ctx_t* ctx, const char* next_key)
{
	switch (ctx->token) {
	case '[':
		SKIP_VALUE(p, ctx);
		break;
	case '\t':
	case ' ':
	case ']':
		COMMIT_VALUE(ctx);
		TRY_ADD_PROP(p, ctx);
		SET_KEY(p, next_key);
		SET_VALUE(p, ctx->chunk);
		p->state++;
		break;
	default:
		ctx->blen++;
		break;
	}
}

INLINE void parser_parse_next_thread_bracket(parser_t* p, parse_ctx_t* ctx)
{
	switch (ctx->token) {
	case ']':
	case '\t':
		COMMIT_VALUE(ctx);
		TRY_ADD_PROP(p, ctx);
		SET_KEY(p, KEY_CLASS);
		SET_VALUE(p, ctx->chunk);
		p->state = TRANSITIONCLASS_PSTATE;
		break;
	default:
		ctx->blen++;
		break;
	}
}

INLINE void parser_parse_next_thread(parser_t* p, parse_ctx_t* ctx)
{
	switch (ctx->token) {
	case '[':
		p->state = THREADBRACKET_PSTATE;
		SKIP_VALUE(p, ctx);
		break;
	default:
		p->state = THREADNOBRACKET_PSTATE;
		parser_parse_next_header(p, ctx, KEY_CLASS);
		break;
	}
}

INLINE void parser_parse_next_calltype(parser_t* p, parse_ctx_t* ctx)
{
	switch (ctx->token) {
	case ':':
		COMMIT_VALUE(ctx);
		TRY_ADD_PROP(p, ctx);
		SET_KEY(p, ctx->chunk);
		p->state++;
		break;
	default:
		ctx->blen++;
		break;
	}
}

INLINE void parser_parse_verify_calltype(parser_t* p, parse_ctx_t* ctx)
{
	switch (ctx->token) {
	case ',':
		p->result.props->next->key = p->result.props->next->value;
		p->result.props->next->value = ctx->chunk;
		p->result.props = p->result.props->next;
		p->pnext--;
		parser_parse_next_value(p, ctx);
		break;
	default:
		parser_parse_next_key(p, ctx);
		break;
	}
}

INLINE void parser_parse_end(parser_t* p, parse_ctx_t* ctx, parse_res_t* res)
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
		parser_parse_error(p, ctx, "incomplete header");
		res->type = PARSE_ERROR;
		res->result.error = ctx->error;
		return;
	case CALLTYPE_PSTATE:
	case VERIFYCALLTYPE_PSTATE:
	case KEY_PSTATE:
		SET_KEY(p, KEY_MESSAGE);
		SET_VALUE(p, ctx->chunk);
		COMMIT_VALUE(ctx);
		break;
	case MULTIKEY_PSTATE:
	case VALUE_PSTATE:
	case TRANSITIONLEVEL_PSTATE:
	case TRANSITIONTHREAD_PSTATE:
	case TRANSITIONCALLTYPE_PSTATE:
	case TRANSITIONCLASS_PSTATE:
		SET_VALUE(p, ctx->chunk);
		COMMIT_VALUE(ctx);
		break;
	case ERROR_PSTATE:
		res->type = PARSE_ERROR;
		res->result.error = ctx->error;
		return;
	}

	res->type = PARSE_COMPLETE;
	res->result.log = &p->result;
}

parse_res_t parser_parse(parser_t* p, char* chunk, size_t clen)
{
	DEBUG_ASSERT(p != NULL);
	DEBUG_ASSERT(chunk != NULL);

	parse_ctx_t ctx;
	parse_res_t res;

	ctx.chunk = chunk;
	ctx.blen = 0;

	for (res.consumed = 0; clen > 0; clen--) {
		ctx.token = chunk[res.consumed++];
		switch (ctx.token) {
		case '\n':
			parser_parse_end(p, &ctx, &res);
			return res;
		default:
		parse:
			switch (p->state) {
			case INIT_PSTATE:
				SET_VALUE(p, chunk);
				p->state++;
				/* fallthrough */
			case DATE_PSTATE:
				parser_parse_next_date(p, &ctx, KEY_TIME);
				break;
			case TIME_PSTATE:
				parser_parse_next_date(p, &ctx, KEY_LEVEL);
				break;
			case LEVEL_PSTATE:
				parser_parse_next_header(p, &ctx, KEY_THREAD);
				break;
			case THREAD_PSTATE:
				parser_parse_next_thread(p, &ctx);
				break;
			case THREADBRACKET_PSTATE:
				parser_parse_next_thread_bracket(p, &ctx);
				break;
			case THREADNOBRACKET_PSTATE:
				parser_parse_next_header(p, &ctx, KEY_CLASS);
				break;
			case CLASS_PSTATE:
				parser_parse_next_header(p, &ctx, KEY_CALLTYPE);
				break;
			case CALLTYPE_PSTATE:
				parser_parse_next_calltype(p, &ctx);
				break;
			case VERIFYCALLTYPE_PSTATE:
				parser_parse_verify_calltype(p, &ctx);
				break;
			case KEY_PSTATE:
				parser_parse_next_key(p, &ctx);
				break;
			case VALUE_PSTATE:
				parser_parse_next_value(p, &ctx);
				break;
			case MULTIKEY_PSTATE:
				parser_parse_next_multikey(p, &ctx);
				break;
			case TRANSITIONLEVEL_PSTATE:
			case TRANSITIONTHREAD_PSTATE:
			case TRANSITIONCALLTYPE_PSTATE:
			case TRANSITIONCLASS_PSTATE:
				if (!parser_parse_skip(p, &ctx)) {
					goto parse;
				}
				break;
			/* skip input until newline is found and state is reset */
			case ERROR_PSTATE:
				/* optimize non-error case */
				if (clen == 1 || chunk[res.consumed] == '\n') {
					if (clen != 1) {
						res.consumed++;
					}
					res.type = PARSE_ERROR;
					res.result.error = ctx.error;
					return res;
				}
				break;
			}
			break;
		}
	}

	res.type = PARSE_PARTIAL;
	return res;
}
