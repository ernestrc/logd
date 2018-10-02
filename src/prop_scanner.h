#ifndef LOGD_PROP_SCANNER_H
#define LOGD_PROP_SCANNER_H

#include "scanner.h"

/*
 * Expects data to be in one of the following formats:
 *
 * key1: value2, key2: value2, ...
 * "key1": "value2", "key2": "value2", ...
 * { "key1": "value2", "key2": "value2", ... }
 *
 * Note that if the logs are in JSON format, only the top level key-value pairs
 * are scanned. For example, if a log looks like this:
 *
 * { "date": "2018-08-30", "count": 4, "lang": { "sucks": "java", "rocks": ["golang", "c", "lua"] }, "frameworks": []}
 *
 * After parsing is complete, calls to logd.log_get(logptr, <key>) will return the following lua strings:
 *
 * logd.log_get(logptr, "date") -- '2018-08-30'
 * logd.log_get(logptr, "count") -- '4'
 * logd.log_get(logptr, "languages") -- '{ "sucks": "java", "rocks": ["golang", "c", "lua"] }' 
 * logd.log_get(logptr, "frameworks") -- '[]' 
 *
 * Therefore, anything other than JSON strings is ready to be parsed by a regular JSON scanner:
 *
 * local JSON = require('json')
 *
 * JSON.parse(logd.log_get(logptr, "count")) -- 4
 * JSON.parse(logd.log_get(logptr, "frameworks")) -- {}
 */
typedef enum jstate_s {
	INIT_JSTATE,
	KEY_TRANS_JSTATE,
	KEY_STR_JSTATE,
	KEY_OTHER_JSTATE,
	VAL_SPLIT_JSTATE,
	VAL_TRANS_JSTATE,
	VAL_ARR_JSTATE,
	VAL_OBJ_JSTATE,
	VAL_ARR_ESCAPE_JSTATE,
	VAL_OBJ_ESCAPE_JSTATE,
	VAL_ARR_ESCAPE_ESCAPE_JSTATE,
	VAL_OBJ_ESCAPE_ESCAPE_JSTATE,
	VAL_NODE_COMMIT_JSTATE,
	VAL_OTHER_JSTATE,
	VAL_STR_JSTATE,
	ERROR_JSTATE,
} jstate_t;

typedef struct prop_scanner_s {
	jstate_t state;
	LOGD_SCANNER_FIELDS
} prop_scanner_t;

#endif
