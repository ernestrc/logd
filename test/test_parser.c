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

int test_parse_partial()
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

	res = parser_parse(p, chunk1, len1);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, len1);

	res = parser_parse(p, chunk2, len2);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, len2);

	res = parser_parse(p, chunk3, len3);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, len3);

	res = parser_parse(p, chunk4, len4);
	ASSERT_EQ(res.type, PARSE_PARTIAL);
	ASSERT_EQ(res.consumed, len4);

	res = parser_parse(p, chunk5, len5);
	ASSERT_EQ(res.type, PARSE_COMPLETE);
	ASSERT_EQ(res.consumed, len5);

	ASSERT_LOG_EQ(res.result.log, &EXPECTED1);

	return 0;
}

#define ELOG1 "msg: this log does not have a header, subject: idiot\n"
#define ELEN1 strlen(ELOG1)

#define ELOG3 "\n"
#define ELEN3 strlen(ELOG3)

int test_parse_error()
{

	char* ERR1 = malloc(ELEN1);
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
	ASSERT_EQ(res.result.error, "invalid date or time in log header");
	parser_reset(p);

	res = parser_parse(p, ERR2, ELEN2);
	ASSERT_EQ(res.type, PARSE_ERROR);
	ASSERT_EQ(res.consumed, ELEN2);
	ASSERT_EQ(res.result.error, "reached max number of log properties: " STR(PARSER_SLAB_CAP));
	parser_reset(p);

	res = parser_parse(p, ERR3, ELEN3);
	ASSERT_EQ(res.type, PARSE_ERROR);
	ASSERT_EQ(res.consumed, ELEN3);
	ASSERT_EQ(res.result.error, "incomplete header");
	parser_reset(p);

	res = parser_parse(p, BUF6, LEN6);
	ASSERT_EQ(res.type, PARSE_COMPLETE);
	ASSERT_LOG_EQ(res.result.log, &EXPECTED6);
	parser_reset(p);

	free(ERR1);
	free(ERR2);

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
		ASSERT_LOG_EQ(res.result.log, test.expected);
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
	TEST_RUN(ctx, test_parse_error);
	TEST_RUN(ctx, test_parse_partial);

	TEST_RELEASE(ctx);
}
