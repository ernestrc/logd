#ifndef LOGD_SCANNER_H
#define LOGD_SCANNER_H

#include <stdbool.h>

#include "log.h"

/* scan result data structures */
enum presult_e {
	SCAN_PARTIAL = 0,
	SCAN_COMPLETE = 1,
	SCAN_ERROR = 2,
};

typedef struct scan_res_s {
	enum presult_e type;
	size_t consumed;
	struct {
		const char* msg;
		const char* at;
	} error;
	log_t* log;
} scan_res_t;

/* optional base fields for scanner.h implementations.
 * This fields enable the use of the following macros */
#define LOGD_SCANNER_FIELDS                                                    \
	/* log to fill up and return with scanner_scan */                          \
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
	/* return value of scanner_scan  */                                        \
	scan_res_t res;

#define LOGD_SCANNER_INIT(p, pslab)                                            \
	p->pslab = pslab;                                                          \
	p->res.log = &p->result;                                                   \
	scanner_reset(p);

#define LOGD_SCANNER_RESET(p)                                                   \
	p->pnext = 0;                                                              \
	memset(p->pslab, 0, LOGD_SLAB_CAP);                                        \
	log_init(&p->result);                                                      \
	p->res.type = SCAN_PARTIAL;

#define SCANNER_SET_ERROR(p, m, error_state)                                    \
	(p)->res.error.msg = (m);                                                  \
	(p)->res.error.at = (p)->chunk;                                            \
	(p)->state = error_state;

#define ADD_PROP(p)                                                            \
	(p)->pslab[(p)->pnext].next = (p)->result.props;                           \
	(p)->result.props = &(p)->pslab[(p)->pnext];                               \
	(p)->pnext++;

#define TRY_ADD_PROP(p, error_state)                                           \
	if ((p)->pnext == LOGD_SLAB_CAP) {                                         \
		SCANNER_SET_ERROR(p,                                                    \
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

#define SCANNER_END_ERROR(p)                                                    \
	(p)->res.type = SCAN_ERROR;                                               \
	COMMIT(p); /* make sure 'at' is null terminated */                         \
	DEBUG_ASSERT((p)->res.error.msg != NULL);                                  \
	DEBUG_ASSERT((p)->res.error.at != NULL);

#define SCANNER_SCAN_ERROR(p)                                                  \
	switch ((p)->token) {                                                      \
	case '\n':                                                                 \
		SCANNER_END_ERROR(p);                                                   \
		return (p)->res;                                                       \
	default:                                                                   \
		(p)->blen++;                                                           \
		break;                                                                 \
	}

#define TRY_SET_NEW_KEY(p, chunk, error_state)                                 \
	TRY_ADD_PROP(p, error_state);                                              \
	SET_KEY(p, chunk);

void* scanner_create();
void scanner_init(void* p, prop_t* pslab);
void scanner_free(void* p);

/* scanner_scan takes a mutable chunk of data, and attempts to scan its
 * contents into a structured log.
 *
 * It will return either because all data has been scanned or because a log
 * has been fully scanned. The scanner's return value indicates whether the log
 * is complete or partial and how many bytes have been consumed. If the result
 * is complete clients are responsible of calling scanner_reset before the
 * next call to this function. The returned log pointer is not valid after that
 * and the behaviour is undefined if the log pointer is accessed after this
 * function returns a partial result.
 *
 * If an error is encountered while parsing, the scanner takes care of skipping
 * until the beginning of the next log and returning the appropriate number of
 * bytes consumed so scan can be resumed after handling the error. The return
 * value will contain an error message, the remaining of the line,
 * and the partially scannedd log.
 *
 * Subsequent calls to scanner_scan are expected to pass memory-contiguous
 * chunks of data.
 *
 * Scanner requirements:
 *
 * - Should return a log pointer that is valid until scanner_reset is called
 * - Should skip until next log if error is encountered while parsing
 * - Should scan partial logs and return the appropriate consumed bytes
 * - Should be able to scan data multiple times except after a complete result
 */
scan_res_t scanner_scan(void* p, char* chunk, size_t clen);

/* scanner_reset should be called after a returned log has been used */
void scanner_reset(void* p);

#endif
