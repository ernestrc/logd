#ifndef LOGD_PARSER_H
#define LOGD_PARSER_H

#include <stdbool.h>

#include "log.h"
#include "slab/slab.h"

#define KEY_THREAD "thread"
#define KEY_CLASS "class"
#define KEY_LEVEL "level"
#define KEY_TIME "time"
#define KEY_DATE "date"
#define KEY_TIMESTAMP "timestamp"
#define KEY_MESSAGE "msg"
#define KEY_CALLTYPE "callType"

typedef enum pstate_s {
	DATE_PSTATE = 0,
	TIME_PSTATE = 1,
	TRANSITIONLEVEL_PSTATE = 2,
	LEVEL_PSTATE = 3,
	TRANSITIONTHREAD_PSTATE = 4,
	THREAD_PSTATE = 5,
	THREADBRACKET_PSTATE = 6, // IF NEXT == '['
	THREADNOBRACKET_PSTATE = 7, // ELSE
	TRANSITIONCLASS_PSTATE = 8,
	CLASS_PSTATE = 9,
	TRANSITIONCALLTYPE_PSTATE = 10,
	CALLTYPE_PSTATE = 11,
	VERIFYCALLTYPE_PSTATE = 12,
	KEY_PSTATE = 13,
	MULTIKEY_PSTATE = 14,
	VALUE_PSTATE = 15,
	ERROR_PSTATE = 16,
} pstate_t;

typedef struct parser_s {
	pstate_t state;
	char* bstart;
	size_t blen;
	slab_t* pslab;
	log_t result;
} parser_t;

typedef struct presult_s {
	bool complete;
	size_t consumed;
} presult_t;

parser_t* parser_create();
void parser_init(parser_t* p, slab_t* pslab);
void parser_free(parser_t* p);

/* parser_parse takes a chunk of data, and attempts to consume its contents
 * populating the parser's `result` field.
 *
 * It will return either because EOF is reached, or because a log has been
 * consumed. The parser's return value indicates whether the log in `result` is
 * complete or partial and it contains a pointer to the next byte that should be
 * scanned by the parser or NULL if EOF was reached.
 */
presult_t parser_parse(parser_t* p, char* chunk, size_t clen);

#endif
