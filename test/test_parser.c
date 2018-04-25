#include "test.h"
#include "../src/parser.h"

int test_parser_create()
{
	parser_t* b = parser_create();
	// ASSERT_NEQ(b->buf, NULL);
	parser_free(b);

	return 0;
}

int main(int argc, char* argv[])
{
	test_ctx_t ctx;
	TEST_INIT(ctx, argc, argv);

	TEST_RUN(ctx, test_parser_create);

	TEST_RELEASE(ctx);
}
