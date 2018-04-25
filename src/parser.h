#ifndef LOGD_PARSER_H
#define LOGD_PARSER_H

#include "log.h"

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
	char* start;
	char* end;
	char* raw;
	log_t log;
} parser_t;

parser_t* parser_create();
void parser_init(parser_t* p);
void parser_free(parser_t* p);

#endif
