#ifndef LOGD_PARSER_H
#define LOGD_PARSER_H

#include <stdbool.h>

#include "parse.h"

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
	TRANSITIONVERIFYCALLTYPE_PSTATE = 13,
	VERIFYCALLTYPE_PSTATE = 14,
	TRANSITIONKEY_PSTATE = 15,
	KEY_PSTATE = 16,
	TRANSITIONVALUE_PSTATE = 17,
	VALUE_PSTATE = 18,
	ERROR_PSTATE = 19,
} pstate_t;

typedef struct parser_s {
	pstate_t state;
	LOGD_PARSER_FIELDS
} parser_t;

parser_t* parser_create();
void parser_init(parser_t* p, prop_t* pslab);
void parser_free(parser_t* p);

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
parse_res_t parser_parse(parser_t* p, char* chunk, size_t clen);

/* parser_reset should be called after a returned log has been used */
void parser_reset(parser_t* p);

#endif
