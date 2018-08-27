#ifndef LOGD_PROP_PARSER_H
#define LOGD_PROP_PARSER_H

#include "parse.h"

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

typedef struct prop_parser_s {
	jstate_t state;
	log_t result;
	prop_t* pslab;
	int pnext, tnext;
	char token;
	char* chunk;
	int blen;
	int nest;
	parse_res_t res;
} prop_parser_t;

void parser_reset(prop_parser_t* p);
void parser_init(prop_parser_t* p, prop_t* pslab);
void parser_free(prop_parser_t* p);
prop_parser_t* parser_create();
parse_res_t parser_parse(prop_parser_t* p, char* chunk, size_t clen);

#endif
