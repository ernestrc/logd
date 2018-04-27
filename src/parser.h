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

typedef enum pstate_s {
	DATE_PSTATE,
	TIME_PSTATE,
	TRANSITIONLEVEL_PSTATE,
	LEVEL_PSTATE,
	TRANSITIONTHREAD_PSTATE,
	THREAD_PSTATE,
	THREADBRACKET_PSTATE,   // IF NEXT == '['
	THREADNOBRACKET_PSTATE, // ELSE
	TRANSITIONCLASS_PSTATE,
	CLASS_PSTATE,
	TRANSITIONCALLTYPE_PSTATE,
	CALLTYPE_PSTATE,
	VERIFYCALLTYPE_PSTATE,
	KEY_PSTATE,
	MULTIKEY_PSTATE,
	VALUE_PSTATE,
	ERROR_PSTATE,
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
	char* next;
} presult_t;

parser_t* parser_create();
void parser_init(parser_t* p, slab_t* pslab);
void parser_free(parser_t* p);

/* parser_parse takes a chunk of data, and attempts to consume its contents populating the parser's `result` field. 
 *
 * It will return either because EOF is reached, or because a log has been consumed. The parser's return value indicates whether
 * the log in `result` is complete or partial and it contains a pointer to the next byte that should be scanned by the parser or NULL if EOF was reached.
 */
presult_t parser_parse(parser_t* p, char* chunk, size_t clen);

#endif
