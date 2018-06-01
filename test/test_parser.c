#include <errno.h>

#include "../src/log.h"
#include "../src/parser.h"
#include "../src/util.h"
#include "test.h"

struct tcase {
	char* input;
	size_t ilen;
	log_t* expected;
};

int test_parser_create()
{
	parser_t* p = parser_create();
	ASSERT_NEQ(p->pslab, NULL);
	parser_free(p);

	return 0;
}

int test_parse_partial2()
{
	parse_res_t res;
	parser_t* p = parser_create();

	char* chunk1 = BUF8;
	size_t len1 = 159;

	char* chunk2 = BUF8 + len1;
	size_t len2 = LEN8 - len1;

	res = parser_parse(p, chunk1, len1);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, len1);

	res = parser_parse(p, chunk2, len2);
	ASSERT_EQ(res.type, PARSE_COMPLETE);
	ASSERT_EQ(res.consumed, 308);

	ASSERT_LOG_EQ(res.log, &EXPECTED8);

	return 0;
}

int test_parse_partial1()
{

	parse_res_t res;
	parser_t* p = parser_create();

	char* chunk1 = BUF1;
	size_t len1 = 8;

	char* chunk2 = BUF1 + len1;
	size_t len2 = 22 - len1;

	char* chunk3 = BUF1 + len1 + len2;
	size_t len3 = 24 - len1 - len2;

	char* chunk4 = BUF1 + len1 + len2 + len3;
	size_t len4 = 47 - len3 - len2 - len1;

	char* chunk5 = BUF1 + len1 + len2 + len3 + len4;
	size_t len5 = LEN1 - len1 - len2 - len3 - len4;

	ASSERT_EQ(len5 + len4 + len3 + len2 + len1, LEN1);
 
 	// printf("parse: '%.*s'\n", (int)len1, chunk1);
	res = parser_parse(p, chunk1, len1);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, len1);

 	// printf("parse: '%.*s'\n", (int)len2, chunk2);
	res = parser_parse(p, chunk2, len2);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, len2);

 	// printf("parse: '%.*s'\n", (int)len3, chunk3);
	res = parser_parse(p, chunk3, len3);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, len3);

 	// printf("parse: '%.*s'\n", (int)len4, chunk4);
	res = parser_parse(p, chunk4, len4);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, len4);

 	// printf("parse: '%.*s'\n", (int)len5, chunk5);
	res = parser_parse(p, chunk5, len5);
	ASSERT_EQ(res.type, PARSE_COMPLETE);
	ASSERT_EQ(res.consumed, len5);

	ASSERT_LOG_EQ(res.log, &EXPECTED1);

	return 0;
}

#define ELOG1 "msg: this log does not have a header, subject: idiot\n"
#define ELEN1 strlen(ELOG1)

#define ELOG3 "\n"
#define ELEN3 strlen(ELOG3)
int test_parse_error()
{

	char* ERR1 = malloc(ELEN1 + 1);
	ASSERT_NEQ(ERR1, NULL);
	memcpy(ERR1, ELOG1, ELEN1);

	static const size_t BLEN2 = 10000000;
	char* ERR2 = malloc(BLEN2);
	ASSERT_NEQ(ERR2, NULL);

	char* ERR3 = malloc(ELEN3);
	ASSERT_NEQ(ERR3, NULL);
	memcpy(ERR3, ELOG3, ELEN3);

	log_t* big_log = log_create();
	log_set(big_log, slab_get(pslab), KEY_TIME, "14:54:39,474");
	log_set(big_log, slab_get(pslab), KEY_DATE, "2017-09-07");
	log_set(big_log, slab_get(pslab), KEY_LEVEL, "DEBUG");
	log_set(big_log, slab_get(pslab), KEY_THREAD, "pool-5-thread-6");
	log_set(big_log, slab_get(pslab), KEY_CLASS, "control.RaptorHandler");
	log_set(big_log, slab_get(pslab), KEY_CALLTYPE, "PublisherCreateRequest");
	// this + header = more properties than we should be able to parse
	for (int i = 0; i < PARSER_SLAB_CAP; i++) {
		char* prop_key = rcmalloc(100);
		ASSERT_NEQ(prop_key, NULL);
		sprintf(prop_key, "%d", i);
		prop_t* prop = slab_get(pslab);
		ASSERT_NEQ(prop, NULL);
		log_set(big_log, prop, prop_key, NULL);
	}
	size_t ELEN2 = snprintl(ERR2, BLEN2, big_log);

	parser_t* p = parser_create();
	parse_res_t res;

	res = parser_parse(p, ERR1, ELEN1);
	ASSERT_EQ(res.type, PARSE_ERROR);
	ASSERT_EQ(res.consumed, ELEN1);
	ASSERT_EQ(res.error.msg, "invalid date or time in log header");
	ASSERT_STR_EQ(res.error.remaining, "msg: this log does not have a header, subject: idiot");
	log_t* log1 = log_create();
	ASSERT_EQ(log1->props, NULL);
	ASSERT_LOG_EQ(res.log, log1);
	parser_reset(p);

	res = parser_parse(p, ERR2, ELEN2);
	// result is partial because ERR2 does not have a newline
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, ELEN2);

	ERR2[ELEN2] = '\n';
	res = parser_parse(p, &ERR2[ELEN2], 1);
	ASSERT_EQ(res.type, PARSE_ERROR);
	ASSERT_EQ(res.consumed, 1);
	ASSERT_STR_EQ(res.error.msg, "reached max number of log properties: " STR(PARSER_SLAB_CAP));
	ASSERT_STR_EQ(res.error.remaining, " 5: null, 4: null, 3: null, 2: null, 1: null, 0: null, ");

	// prepare log comparison
	log_remove(big_log, "0");
	log_remove(big_log, "1");
	log_remove(big_log, "2");
	log_remove(big_log, "3");
	log_remove(big_log, "4");
	log_remove(big_log, "5");
	for (int i = 0; i < PARSER_SLAB_CAP; i++) {
		char* prop_key = rcmalloc(100);
		ASSERT_NEQ(prop_key, NULL);
		sprintf(prop_key, "%d", i);
		slab_put(pslab, log_remove(big_log, prop_key));
	}
	for (int i = PARSER_SLAB_CAP - 1; i >= 6; i--) {
		char* prop_key = rcmalloc(100);
		ASSERT_NEQ(prop_key, NULL);
		sprintf(prop_key, "%d", i);
		prop_t* prop = slab_get(pslab);
		ASSERT_NEQ(prop, NULL);
		log_set(big_log, prop, prop_key, NULL);
	}
	// compare that log contains the partially parsed line properties
	ASSERT_LOG_EQ(res.log, big_log);
	parser_reset(p);

	res = parser_parse(p, ERR3, ELEN3);
	ASSERT_EQ(res.type, PARSE_ERROR);
	ASSERT_EQ(res.consumed, ELEN3);
	ASSERT_STR_EQ(res.error.msg, "incomplete header");
	ASSERT_STR_EQ(res.error.remaining, "");
	parser_reset(p);

	res = parser_parse(p, BUF6, LEN6);
	ASSERT_EQ(res.type, PARSE_COMPLETE);
	ASSERT_LOG_EQ(res.log, &EXPECTED6);
	parser_reset(p);

	free(ERR1);
	free(ERR2);

	return 0;
}

int test_parse_multiple()
{
	parser_t* p = parser_create();
	parse_res_t res;

	/* feed partial data */
	res = parser_parse(p, BUF7, LEN7 - 2);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, LEN7 - 2);
	parser_reset(p);

	/* check that we are able to re-parse the data */
	res = parser_parse(p, BUF7, LEN7);
	ASSERT_EQ(res.type, PARSE_COMPLETE);
	ASSERT_EQ(res.consumed, LEN7);
	ASSERT_LOG_EQ(res.log, &EXPECTED7);

	return 0;
}

int test_parse_reset()
{
	parser_t* p = parser_create();
	static const size_t CASES_LEN = 4;
	struct tcase CASES[CASES_LEN];
	CASES[0] = (struct tcase){BUF3, LEN3, &EXPECTED3};
	CASES[1] = (struct tcase){BUF2, LEN2, &EXPECTED2};
	CASES[2] = (struct tcase){BUF4, LEN4, &EXPECTED4};
	CASES[3] = (struct tcase){BUF5, LEN5, &EXPECTED5};

	for (int i = 0; i < CASES_LEN; i++) {
		struct tcase test = CASES[i];
		parse_res_t res = parser_parse(p, test.input, test.ilen);
		ASSERT_EQ(res.type, PARSE_COMPLETE);
		ASSERT_EQ(res.consumed, test.ilen);
		ASSERT_LOG_EQ(res.log, test.expected);
		parser_reset(p);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	test_ctx_t ctx;
	TEST_INIT(ctx, argc, argv);

	TEST_RUN(ctx, test_parser_create);
	TEST_RUN(ctx, test_parse_reset);
	TEST_RUN(ctx, test_parse_partial1);
	TEST_RUN(ctx, test_parse_partial2);
	TEST_RUN(ctx, test_parse_multiple);
	TEST_RUN(ctx, test_parse_error);

	TEST_RELEASE(ctx);
}
