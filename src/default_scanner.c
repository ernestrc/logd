#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./config.h"
#include "./default_scanner.h"
#include "./scanner.h"
#include "./util.h"

#define TRIM_SPACES(p, SET_MACRO, label, END_MACRO)                            \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		END_MACRO(p);                                                          \
		return (p)->res;                                                       \
	case '\t':                                                                 \
	case ' ':                                                                  \
		SKIP(p);                                                               \
		break;                                                                 \
	default:                                                                   \
		SET_MACRO(p, (p)->chunk);                                              \
		(p)->state++;                                                          \
		goto label;                                                            \
	}

#define SCAN_END_MESSAGE(p)                                                   \
	SET_KEY((p), KEY_MESSAGE);                                                 \
	SET_VALUE(p, (p)->chunk);                                                  \
	COMMIT(p);                                                                 \
	(p)->res.type = SCAN_COMPLETE;

#define SCAN_END(p)                                                           \
	SET_VALUE(p, (p)->chunk);                                                  \
	COMMIT(p);                                                                 \
	(p)->res.type = SCAN_COMPLETE;

#define SCANNER_END_ERROR_INCOMPLETE(p)                                         \
	SCANNER_SET_ERROR(p, "incomplete header", ERROR_PSTATE);                    \
	SET_VALUE((p), "");                                                        \
	SCANNER_END_ERROR(p);

#define SCANNER_SCAN_NEXT_KEY(p)                                               \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SCAN_END_MESSAGE(p);                                                  \
		return (p)->res;                                                       \
	case ':':                                                                  \
	case '\x00': /* re-submitted partial data */                               \
		COMMIT(p);                                                             \
		(p)->state = TRANSITIONVALUE_PSTATE;                                   \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define SCANNER_SCAN_NEXT_VALUE(p)                                             \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SCAN_END(p);                                                          \
		return (p)->res;                                                       \
	case ',':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		TRY_ADD_PROP(p, ERROR_PSTATE);                                         \
		(p)->state = TRANSITIONKEY_PSTATE;                                     \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define SCANNER_SCAN_NEXT_DATE(p, next_key)                                    \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SCANNER_END_ERROR_INCOMPLETE(p);                                        \
		return (p)->res;                                                       \
	case '[':                                                                  \
		SKIP(p);                                                               \
		SET_VALUE(p, (p)->chunk);                                              \
		break;                                                                 \
	case '0':                                                                  \
	case '1':                                                                  \
	case '2':                                                                  \
	case '3':                                                                  \
	case '4':                                                                  \
	case '5':                                                                  \
	case '6':                                                                  \
	case '7':                                                                  \
	case '8':                                                                  \
	case '9':                                                                  \
	case ':':                                                                  \
	case ',':                                                                  \
	case '.':                                                                  \
	case '-':                                                                  \
	case 'Z':                                                                  \
	case '+':                                                                  \
		(p)->blen++;                                                           \
		break;                                                                 \
	case 'T':                                                                  \
	case ' ':                                                                  \
	case ']':                                                                  \
	case '\t':                                                                 \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		ADD_PROP(p);                                                           \
		SET_KEY(p, next_key);                                                  \
		SET_VALUE(p, (p)->chunk);                                              \
		(p)->state++;                                                          \
		break;                                                                 \
	default:                                                                   \
		SCANNER_SET_ERROR(                                                      \
		  p, "invalid date or time in log header", ERROR_PSTATE);              \
		REMOVE_PROP(p);                                                        \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define SCANNER_SCAN_NEXT_HEADER(p, next_key)                                  \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SCANNER_END_ERROR_INCOMPLETE(p);                                        \
		return (p)->res;                                                       \
	case '[':                                                                  \
		SKIP(p);                                                               \
		SET_VALUE(p, (p)->chunk);                                              \
		break;                                                                 \
	case '\t':                                                                 \
	case ' ':                                                                  \
	case ']':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		ADD_PROP(p);                                                           \
		SET_KEY(p, next_key);                                                  \
		(p)->state++;                                                          \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define SCANNER_SCAN_NEXT_THREAD_BRACKET(p)                                     \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SCANNER_END_ERROR_INCOMPLETE(p);                                        \
		return (p)->res;                                                       \
	case ']':                                                                  \
	case '\t':                                                                 \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		ADD_PROP(p);                                                           \
		SET_KEY(p, KEY_CLASS);                                                 \
		(p)->state = TRANSITIONCLASS_PSTATE;                                   \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define SCANNER_SCAN_NEXT_THREAD(p)                                            \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SCANNER_END_ERROR_INCOMPLETE(p);                                        \
		return (p)->res;                                                       \
	case '[':                                                                  \
		(p)->state = THREADBRACKET_PSTATE;                                     \
		SKIP(p);                                                               \
		SET_VALUE(p, (p)->chunk);                                              \
		break;                                                                 \
	default:                                                                   \
		(p)->state = THREADNOBRACKET_PSTATE;                                   \
		SCANNER_SCAN_NEXT_HEADER(p, KEY_CLASS);                                \
		break;                                                                 \
	}

#define SCANNER_SCAN_NEXT_CALLTYPE(p)                                          \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SCAN_END_MESSAGE(p);                                                  \
		return (p)->res;                                                       \
	case ':':                                                                  \
	case '\x00':                                                               \
		COMMIT(p);                                                             \
		ADD_PROP(p);                                                           \
		(p)->state++;                                                          \
		break;                                                                 \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define SCANNER_SCAN_VERIFY_CALLTYPE(p)                                        \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SCAN_END_MESSAGE(p);                                                  \
		return (p)->res;                                                       \
	case ',':                                                                  \
		(p)->result.props->next->key = (p)->result.props->next->value;         \
		(p)->result.props->next->value = (p)->chunk;                           \
		REMOVE_PROP(p);                                                        \
		SCANNER_SCAN_NEXT_VALUE(p);                                            \
		break;                                                                 \
	default:                                                                   \
		SCANNER_SCAN_NEXT_KEY(p);                                              \
		break;                                                                 \
	}

void* scanner_create()
{
	scanner_t* p = NULL;
	prop_t* slab = NULL;

	if ((p = calloc(1, sizeof(scanner_t))) == NULL ||
	  (slab = calloc(LOGD_SLAB_CAP, sizeof(prop_t))) == NULL) {
		perror("calloc");
		goto error;
	}

	scanner_init(p, slab);

	return (void*)p;

error:
	if (p)
		free(p);
	if (slab)
		free(slab);
	return NULL;
}

void scanner_reset(void* _p)
{
	scanner_t* p = (scanner_t*)_p;

	DEBUG_ASSERT(p != NULL);

	p->state = INIT_PSTATE;

	LOGD_SCANNER_RESET(p);

	ADD_PROP(p);
	SET_KEY(p, KEY_DATE);
}

void scanner_init(void* _p, prop_t* pslab)
{
	scanner_t* p = (scanner_t*)_p;

	DEBUG_ASSERT(p != NULL);
	LOGD_SCANNER_INIT(p, pslab);
}

void scanner_free(void* _p)
{
	scanner_t* p = (scanner_t*)_p;

	if (p == NULL)
		return;

	free(p->pslab);
	free(p);
}

scan_res_t scanner_scan(void* _p, char* chunk, size_t clen)
{
	scanner_t* p = (scanner_t*)_p;

	DEBUG_ASSERT(p != NULL);
	DEBUG_ASSERT(chunk != NULL);

	p->chunk = chunk;
	p->blen = 0;
	p->res.consumed = 0;

	while (p->res.consumed < clen) {
		p->token = chunk[p->res.consumed++];
		switch (p->state) {
		case INIT_PSTATE:
			TRIM_SPACES(p, SET_VALUE, date, SCANNER_END_ERROR_INCOMPLETE);
			break;
		case DATE_PSTATE:
		date:
			SCANNER_SCAN_NEXT_DATE(p, KEY_TIME);
			break;
		case TIME_PSTATE:
			SCANNER_SCAN_NEXT_DATE(p, KEY_LEVEL);
			break;
		case LEVEL_PSTATE:
		level:
			SCANNER_SCAN_NEXT_HEADER(p, KEY_THREAD);
			break;
		case THREAD_PSTATE:
		thread:
			SCANNER_SCAN_NEXT_THREAD(p);
			break;
		case THREADBRACKET_PSTATE:
			SCANNER_SCAN_NEXT_THREAD_BRACKET(p);
			break;
		case THREADNOBRACKET_PSTATE:
			SCANNER_SCAN_NEXT_HEADER(p, KEY_CLASS);
			break;
		case CLASS_PSTATE:
		clazz:
			SCANNER_SCAN_NEXT_HEADER(p, KEY_CALLTYPE);
			break;
		case CALLTYPE_PSTATE:
		calltype:
			SCANNER_SCAN_NEXT_CALLTYPE(p);
			break;
		case VERIFYCALLTYPE_PSTATE:
		verifycalltype:
			SCANNER_SCAN_VERIFY_CALLTYPE(p);
			break;
		case TRANSITIONVERIFYCALLTYPE_PSTATE:
			TRIM_SPACES(p, SET_KEY, verifycalltype, SCAN_END_MESSAGE);
			break;
		case TRANSITIONKEY_PSTATE:
			TRIM_SPACES(p, SET_KEY, key, SCAN_END_MESSAGE);
			break;
		key:
		case KEY_PSTATE:
			SCANNER_SCAN_NEXT_KEY(p);
			break;
		case VALUE_PSTATE:
		value:
			SCANNER_SCAN_NEXT_VALUE(p);
			break;
		case TRANSITIONLEVEL_PSTATE:
			TRIM_SPACES(p, SET_VALUE, level, SCANNER_END_ERROR_INCOMPLETE);
			break;
		case TRANSITIONTHREAD_PSTATE:
			TRIM_SPACES(p, SET_VALUE, thread, SCANNER_END_ERROR_INCOMPLETE);
			break;
		case TRANSITIONCALLTYPE_PSTATE:
			TRIM_SPACES(p, SET_VALUE, calltype, SCAN_END_MESSAGE);
			break;
		case TRANSITIONCLASS_PSTATE:
			TRIM_SPACES(p, SET_VALUE, clazz, SCANNER_END_ERROR_INCOMPLETE);
			break;
		case TRANSITIONVALUE_PSTATE:
			TRIM_SPACES(p, SET_VALUE, value, SCAN_END);
			break;
		/* skip input until newline is found and state is reset */
		case ERROR_PSTATE:
			SCANNER_SCAN_ERROR(p);
			break;
		}
	}

	return p->res;
}
