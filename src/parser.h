#ifndef LOGD_PARSER_H
#define LOGD_PARSER_H

#include <stdbool.h>

#include "log.h"

#ifndef LOGD_PROP_CAP
#define PARSER_SLAB_CAP 100
#else
#define PARSER_SLAB_CAP LOGD_PROP_CAP
#endif

typedef enum pstate_s {
	INIT_PSTATE = 0,
	DATE_PSTATE = 1,
	TIME_PSTATE = 2,
	TRANSITIONLEVEL_PSTATE = 3,
	LEVEL_PSTATE = 4,
	TRANSITIONTHREAD_PSTATE = 5,
	THREAD_PSTATE = 6,
	THREADBRACKET_PSTATE = 7, // IF NEXT == '['
	THREADNOBRACKET_PSTATE = 8, // ELSE
	TRANSITIONCLASS_PSTATE = 9,
	CLASS_PSTATE = 10,
	TRANSITIONCALLTYPE_PSTATE = 11,
	CALLTYPE_PSTATE = 12,
	VERIFYCALLTYPE_PSTATE = 13,
	KEY_PSTATE = 14,
	MULTIKEY_PSTATE = 15,
	VALUE_PSTATE = 16,
	ERROR_PSTATE = 17,
} pstate_t;

typedef struct parser_s {
	const char* error;
	prop_t* pslab;
	int pnext;
	pstate_t state;
	log_t result;
} parser_t;

enum presult_e {
	PARSE_PARTIAL = 0,
	PARSE_COMPLETE = 1,
	PARSE_ERROR = 2,
};

typedef struct parse_res_s {
	enum presult_e type;
	size_t consumed;
	union {
		const char* error;
		log_t* log;
	} result;
} parse_res_t;

parser_t* parser_create();
void parser_init(parser_t* p, prop_t* pslab);
void parser_free(parser_t* p);

/* parser_parse takes a mutable chunk of data, and attempts to parse its
 * contents into a structured log.
 *
 * It will return either because all data has been scanned or because a log
 * has been fully parsed. The parser's return value indicates whether the log in
 * is complete or partial and how many bytes have been consumed. If the result
 * is complete clients are responsible for calling parser_consume before the
 * next parsing iteration. The returned log pointer is valid only until client
 * calls parser_reset. The behaviour is undefined if the log pointer is accessed
 * after this function returns a partial or error result.
 *
 * If an error is encountered while parsing, the parser takes care of skipping
 * until the beggingin of the next log and returning the appropiate number of
 * bytes consumed so parse can be resumed after handling the error.
 *
 * Parser requirements:
 *
 * - Should return a log pointer that is valid until parser_reset is called
 * - Should skip until next log if error is encountered while parsing
 * - Should parse partial logs and return the appropiate consumed bytes
 * - TODO Should be able to parse its input twice if required
 */
parse_res_t parser_parse(parser_t* p, char* chunk, size_t clen);

/* parser_reset should be called after a complete parse result and after log has
 * been consumed. */
void parser_reset(parser_t* p);

#endif
