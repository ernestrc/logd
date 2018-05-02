#include <errno.h>

#include "test.h"
#include "../src/parser.h"
#include "../src/log.h"
#include "../src/util.h"

int test_parser_create()
{
	parser_t* p = parser_create();
	ASSERT_NEQ(p->pslab, NULL);
	parser_free(p);

	return 0;
}

int test_parse()
{
struct tcase {
	char* input;
	size_t ilen;
	log_t* expected;
};

	parser_t* p = parser_create();
	static const size_t CASES_LEN = 6;
	struct tcase CASES[CASES_LEN];
	CASES[0] = (struct tcase){BUF1, LEN1, &EXPECTED1};
	CASES[1] = (struct tcase){BUF2, LEN2, &EXPECTED2};
	CASES[2] = (struct tcase){BUF3, LEN3, &EXPECTED3};
	CASES[3] = (struct tcase){BUF4, LEN4, &EXPECTED4};
	CASES[4] = (struct tcase){BUF5, LEN5, &EXPECTED5};
	CASES[5] = (struct tcase){BUF6, LEN6, &EXPECTED6};

	for (int i = 0; i < CASES_LEN; i++) {
		struct tcase test = CASES[i];
		presult_t res = parser_parse(p, test.input, test.ilen);
		ASSERT_EQ(res.complete, true);
		ASSERT_EQ(res.next, test.input + test.ilen);
		ASSERT_LOG_EQ(&p->result, test.expected);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	test_ctx_t ctx;
	TEST_INIT(ctx, argc, argv);

	TEST_RUN(ctx, test_parser_create);
	TEST_RUN(ctx, test_parse);

	TEST_RELEASE(ctx);
}
