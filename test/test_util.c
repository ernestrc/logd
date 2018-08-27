#include "../src/util.h"
#include "test.h"

#define PRINTEDLOG1                                                            \
	"2017-09-07 14:54:39,474	DEBUG	[pool-5-thread-6]	"                         \
	"control.RaptorHandler	PublisherCreateRequest: traceId: "                  \
	"Publish:Rumor:012ae1a5-3416-4458-b0c1-6eb3e0ab4c80, operation: "          \
	"CreatePublisher, step: Attempt, flow: Publish, "

static int check_snprintl_trunc(log_t* log, char* buf, size_t size, char* printedlog, size_t sizeofprintedlog)
{
	int want;

	memset(buf, 0, 1000);
	want = snprintl(buf, size, log);
	ASSERT_EQ(want + 1, sizeofprintedlog);
	ASSERT_MEM_EQ(printedlog, buf, size - 1);

	// test that we didn't write to invalid memory
	ASSERT_EQ(0, buf[size + 1]);
	ASSERT_EQ(0, buf[size]);

	return 0;
}

int test_snprintl_null()
{
	int want;
	want = snprintl(NULL, 0, &EXPECTED1);
	ASSERT_EQ(want + 1, sizeof(PRINTEDLOG1));
	return 0;
}

int test_snprintl()
{
	size_t size = 1000;
	char buf1[size];
	int want;

	want = snprintl(buf1, size, &EXPECTED1);
	ASSERT_EQ(want + 1, sizeof(PRINTEDLOG1));
	ASSERT_MEM_EQ(buf1, PRINTEDLOG1, want);

	check_snprintl_trunc(&EXPECTED1, buf1, 212, PRINTEDLOG1, sizeof(PRINTEDLOG1));
	check_snprintl_trunc(&EXPECTED1, buf1, 211, PRINTEDLOG1, sizeof(PRINTEDLOG1));
	check_snprintl_trunc(&EXPECTED1, buf1, 150, PRINTEDLOG1, sizeof(PRINTEDLOG1));
	check_snprintl_trunc(&EXPECTED1, buf1, 10, PRINTEDLOG1, sizeof(PRINTEDLOG1));
	check_snprintl_trunc(&EXPECTED1, buf1, 1, PRINTEDLOG1, sizeof(PRINTEDLOG1));

	return 0;
}

int test_exp_backoff()
{
	ASSERT_EQ(next_attempt_backoff(10, 1, 2), 10);
	ASSERT_EQ(next_attempt_backoff(10, 2, 2), 100);
	ASSERT_EQ(next_attempt_backoff(10, 3, 2), 1000);

	ASSERT_EQ(next_attempt_backoff(10, 1, 1), 10);
	ASSERT_EQ(next_attempt_backoff(10, 2, 1), 20);
	ASSERT_EQ(next_attempt_backoff(10, 3, 1), 30);

	return 0;
}

int main(int argc, char* argv[])
{
	test_ctx_t ctx;
	TEST_INIT(ctx, argc, argv);
	init_test_data();

	TEST_RUN(ctx, test_snprintl);
	TEST_RUN(ctx, test_exp_backoff);

	TEST_RELEASE(ctx);
}
