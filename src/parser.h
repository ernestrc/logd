#ifndef LOGD_PARSER_H
#define LOGD_PARSER_H

#include <stdbool.h>

#include "log.h"

/* parse result data structures */
enum presult_e {
	PARSE_PARTIAL = 0,
	PARSE_COMPLETE = 1,
	PARSE_ERROR = 2,
};

typedef struct parse_res_s {
	enum presult_e type;
	size_t consumed;
	struct {
		const char* msg;
		const char* at;
	} error;
	log_t* log;
} parse_res_t;

/* optional base fields for parser.h implementations.
 * This fields enable the use of the following macros */
#define LOGD_PARSER_FIELDS                                                     \
	/* log to fill up and return with parser_parse */                          \
	log_t result;                                                              \
	/* slab of properties to fill up */                                        \
	prop_t* pslab;                                                             \
	/* current property index in pslab */                                      \
	int pnext;                                                                 \
	/* current token */                                                        \
	char token;                                                                \
	/* address of current scanned chunk */                                     \
	char* chunk;                                                               \
	/* offset from chunk address for current iteration */                      \
	int blen;                                                                  \
	/* return value of parser_parse  */                                        \
	parse_res_t res;

#define LOGD_PARSER_INIT(p, pslab)                                             \
	p->pslab = pslab;                                                          \
	p->res.log = &p->result;                                                   \
	parser_reset(p);

#define LOGD_PARSER_RESET(p)                                                   \
	p->pnext = 0;                                                              \
	memset(p->pslab, 0, LOGD_SLAB_CAP);                                        \
	log_init(&p->result);                                                      \
	p->res.type = PARSE_PARTIAL;

#define PARSER_SET_ERROR(p, m, error_state)                                    \
	(p)->res.error.msg = (m);                                                  \
	(p)->res.error.at = (p)->chunk;                                            \
	(p)->state = error_state;

#define ADD_PROP(p)                                                            \
	(p)->pslab[(p)->pnext].next = (p)->result.props;                           \
	(p)->result.props = &(p)->pslab[(p)->pnext];                               \
	(p)->pnext++;

#define TRY_ADD_PROP(p, error_state)                                           \
	if ((p)->pnext == LOGD_SLAB_CAP) {                                         \
		PARSER_SET_ERROR(p,                                                    \
		  "reached max number of log properties: " STR(LOGD_SLAB_CAP),         \
		  error_state);                                                        \
		continue;                                                              \
	}                                                                          \
	ADD_PROP(p);

#define SET_VALUE(p, chunk) (p)->result.props->value = chunk;

#define SET_KEY(p, chunk) (p)->result.props->key = chunk;

#define NOOP(p, chunk) ;

#define COMMIT(p)                                                              \
	(p)->chunk[(p)->blen] = '\x00';                                            \
	(p)->chunk += (p)->blen + 1;                                               \
	(p)->blen = 0;

#define SKIP(p) (p)->chunk++;

#define REMOVE_PROP(p)                                                         \
	(p)->result.props = (p)->result.props->next;                               \
	(p)->pnext--;

#define PARSER_END_ERROR(p)                                                    \
	(p)->res.type = PARSE_ERROR;                                               \
	COMMIT(p); /* make sure 'at' is null terminated */                         \
	DEBUG_ASSERT((p)->res.error.msg != NULL);                                  \
	DEBUG_ASSERT((p)->res.error.at != NULL);

#define PARSER_PARSE_ERROR(p)                                                  \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		PARSER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define TRY_SET_NEW_KEY(p, chunk, error_state)                                 \
	TRY_ADD_PROP(p, error_state);                                              \
	SET_KEY(p, chunk);

void* parser_create();
void parser_init(void* p, prop_t* pslab);
void parser_free(void* p);

/* parser_parse takes a mutable chunk of data, and attempts to parse its
 * contents into a structured log.
 *
 * It will return either because all data has been scanned or because a log
 * has been fully parsed. The parser's return value indicates whether the log
 * is complete or partial and how many bytes have been consumed. If the result
 * is complete clients are responsible of calling parser_reset before the
 * next call to this function. The returned log pointer is not valid after that
 * and the behaviour is undefined if the log pointer is accessed after this
 * function returns a partial result.
 *
 * If an error is encountered while parsing, the parser takes care of skipping
 * until the beginning of the next log and returning the appropriate number of
 * bytes consumed so parse can be resumed after handling the error. The return
 * value will contain an error message, the remaining of the line,
 * and the partially parsed log.
 *
 * Subsequent calls to parser_parse are expected to pass memory-contiguous
 * chunks of data.
 *
 * Parser requirements:
 *
 * - Should return a log pointer that is valid until parser_reset is called
 * - Should skip until next log if error is encountered while parsing
 * - Should parse partial logs and return the appropriate consumed bytes
 * - Should be able to parse data multiple times except after a complete result
 */
parse_res_t parser_parse(void* p, char* chunk, size_t clen);

/* parser_reset should be called after a returned log has been used */
void parser_reset(void* p);

#endif
