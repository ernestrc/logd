#include "../src/util.h"
#include "test.h"

#define PRINTEDLOG1                                                            \
	"2017-09-07 14:54:39,474	DEBUG	[pool-5-thread-6]	"                         \
	"control.RaptorHandler	PublisherCreateRequest: traceId: "                  \
	"Publish:Rumor:012ae1a5-3416-4458-b0c1-6eb3e0ab4c80, operation: "          \
	"CreatePublisher, step: Attempt, flow: Publish, "

static int check_snprintl_trunc(char* buf, size_t size)
{
	int want;

	memset(buf, 0, 1000);
	want = snprintl(buf, size, &EXPECTED1);
	ASSERT_EQ(want + 2, sizeof(LOG1) + 2);
	ASSERT_MEM_EQ(PRINTEDLOG1, buf, size - 1);

	// test that we didn't write to invalid memory
	ASSERT_EQ(0, buf[size + 1]);
	ASSERT_EQ(0, buf[size]);

	return 0;
}

int test_snprintl()
{
	size_t size = 1000;
	char buf1[size];
	int want;

	want = snprintl(buf1, size, &EXPECTED1);
	// +2: new line and null terminator
	// +2: last comma and space
	ASSERT_EQ(want + 2, sizeof(LOG1) + 2);
	ASSERT_MEM_EQ(buf1, PRINTEDLOG1, want);

	check_snprintl_trunc(buf1, 212);
	check_snprintl_trunc(buf1, 211);
	check_snprintl_trunc(buf1, 150);
	check_snprintl_trunc(buf1, 10);
	check_snprintl_trunc(buf1, 1);

	return 0;
}

int main(int argc, char* argv[])
{
	test_ctx_t ctx;
	TEST_INIT(ctx, argc, argv);

	TEST_RUN(ctx, test_snprintl);

	TEST_RELEASE(ctx);
}
