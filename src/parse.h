#ifndef LOGD_PARSE_H
#define LOGD_PARSE_H

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

#endif
