// Copyright (c) 2017, eaugeas <eaugeas at gmail dot com>

#ifndef TEST_TEST_H_
#define TEST_TEST_H_

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slab/rcmalloc.h>
#include <slab/slab.h>

#include "../src/log.h"
#include "../src/parser.h"
#include "../src/util.h"
#include "fixture.h"

typedef struct test_ctx_s {
	char* test_expr;
	int failure;
	int success;
	int silent;
} test_ctx_t;

#define COND_EQ(a, b) ((a) == (b))
#define COND_NEQ(a, b) (!COND_EQ(a, b))
#define COND_MEM_EQ(a, b, l) (memcmp((a), (b), (l)) == 0)
#define COND_STR_EQ(a, b) (strcmp((a), (b)) == 0)
#define COND_TRUE(a) COND_EQ(!!a, 1)
#define COND_FALSE(a) COND_EQ(!a, 1)

#define STRING_ERROR "\t\tassertion error %s %s at line %d: "

#define PRINT_ERR(a, b, COND)                                                  \
	fprintf(stderr, STRING_ERROR #a " " #COND " " #b "\n", __FILE__,           \
	  __FUNCTION__, __LINE__);

#define ASSERT_COND1(a, COND)                                                  \
	if (!COND(a)) {                                                            \
		PRINT_ERR(a, a, COND)                                                  \
		return EXIT_FAILURE;                                                   \
	}

#define ASSERT_COND2(a, b, COND)                                               \
	if (!COND(a, b)) {                                                         \
		PRINT_ERR(a, b, COND)                                                  \
		return EXIT_FAILURE;                                                   \
	}

#define ASSERT_COND3(a, b, n, COND)                                            \
	if (!COND(a, b, n)) {                                                      \
		PRINT_ERR(a, b, COND)                                                  \
		return EXIT_FAILURE;                                                   \
	}

#define ASSERT_MEM_EQ(a, b, l) ASSERT_COND3(a, b, l, COND_MEM_EQ)
#define ASSERT_STR_EQ(a, b) ASSERT_COND2(a, b, COND_STR_EQ)
#define ASSERT_NEQ(a, b) ASSERT_COND2(a, b, COND_NEQ)
#define ASSERT_EQ(a, b) ASSERT_COND2(a, b, COND_EQ)
#define ASSERT_TRUE(a) ASSERT_COND1(a, COND_TRUE)
#define ASSERT_FALSE(a) ASSERT_COND1(a, COND_FALSE)
#define ASSERT_NULL(a) ASSERT_COND2(a, NULL, COND_EQ)

static slab_t* pslab;

#define LEN1 strlen(LOG1)
#define LEN2 strlen(LOG2)
#define LEN3 strlen(LOG3)
#define LEN4 strlen(LOG4)
#define LEN5 strlen(LOG5)
#define LEN6 strlen(LOG6)
#define LEN7 strlen(LOG7)
#define LEN8 strlen(LOG8)

char* BUF1;
char* BUF2;
char* BUF3;
char* BUF4;
char* BUF5;
char* BUF6;
char* BUF7;
char* BUF8;

log_t EXPECTED1;
log_t EXPECTED2;
log_t EXPECTED3;
log_t EXPECTED4;
log_t EXPECTED5;
log_t EXPECTED6;
log_t EXPECTED7;
log_t EXPECTED8;

static void init_test_data()
{
	BUF1 = rcmalloc(LEN1);
	BUF2 = rcmalloc(LEN2);
	BUF3 = rcmalloc(LEN3);
	BUF4 = rcmalloc(LEN4);
	BUF5 = rcmalloc(LEN5);
	BUF6 = rcmalloc(LEN6);
	BUF7 = rcmalloc(LEN7);
	BUF8 = malloc(LEN8);
	memcpy(BUF1, LOG1, LEN1);
	memcpy(BUF2, LOG2, LEN2);
	memcpy(BUF3, LOG3, LEN3);
	memcpy(BUF4, LOG4, LEN4);
	memcpy(BUF5, LOG5, LEN5);
	memcpy(BUF6, LOG6, LEN6);
	memcpy(BUF7, LOG7, LEN7);
	memcpy(BUF8, LOG8, LEN8);

	log_set(&EXPECTED1, slab_get(pslab), KEY_TIME, "14:54:39,474");
	log_set(&EXPECTED1, slab_get(pslab), KEY_DATE, "2017-09-07");
	log_set(&EXPECTED1, slab_get(pslab), KEY_LEVEL, "DEBUG");
	log_set(&EXPECTED1, slab_get(pslab), KEY_THREAD, "pool-5-thread-6");
	log_set(&EXPECTED1, slab_get(pslab), KEY_CLASS, "control.RaptorHandler");
	log_set(
	  &EXPECTED1, slab_get(pslab), KEY_CALLTYPE, "PublisherCreateRequest");
	log_set(&EXPECTED1, slab_get(pslab), "flow", "Publish");
	log_set(&EXPECTED1, slab_get(pslab), "step", "Attempt");
	log_set(&EXPECTED1, slab_get(pslab), "operation", "CreatePublisher");
	log_set(&EXPECTED1, slab_get(pslab), "traceId",
	  "Publish:Rumor:012ae1a5-3416-4458-b0c1-6eb3e0ab4c80");

	log_set(&EXPECTED2, slab_get(pslab), KEY_DATE, "2017-09-07");
	log_set(&EXPECTED2, slab_get(pslab), KEY_TIME, "14:54:39,474");
	log_set(&EXPECTED2, slab_get(pslab), KEY_LEVEL, "DEBUG");
	log_set(&EXPECTED2, slab_get(pslab), KEY_THREAD, "pool-5-thread-6");
	log_set(&EXPECTED2, slab_get(pslab), KEY_CLASS, "control.RaptorHandler");
	log_set(&EXPECTED2, slab_get(pslab), "sessionId",
	  "1_MX4xMDB-fjE1MDQ4MjEyNzAxMjR-WThtTVpEN0J2c1Z2TlJGcndTN1lpTExGfn4");
	log_set(&EXPECTED2, slab_get(pslab), "flow", "Publish");
	log_set(&EXPECTED2, slab_get(pslab), "conId",
	  "connectionId: f41973e5-b27c-49e4-bcaf-1d48b153683e");
	log_set(&EXPECTED2, slab_get(pslab), "step", "Attempt");
	log_set(&EXPECTED2, slab_get(pslab), "publisherId",
	  "b4da82c4-cac5-4e13-b1dc-bb1f42b475dd");
	log_set(&EXPECTED2, slab_get(pslab), "fromAddress",
	  "f41973e5-b27c-49e4-bcaf-1d48b153683e");
	log_set(&EXPECTED2, slab_get(pslab), "projectId", "100");
	log_set(&EXPECTED2, slab_get(pslab), "operation", "CreatePublisher");
	log_set(&EXPECTED2, slab_get(pslab), "traceId",
	  "Publish:Rumor:112ae1a5-3416-4458-b0c1-6eb3e0ab4c80");
	log_set(&EXPECTED2, slab_get(pslab), "streamId",
	  "b4da82c4-cac5-4e13-b1dc-bb1f42b475dd");
	log_set(&EXPECTED2, slab_get(pslab), "remoteIpAddress", "127.0.0.1");
	log_set(&EXPECTED2, slab_get(pslab), "correlationId",
	  "b90232b5-3ee5-4c65-bb4e-29286d6a2771");

	log_set(&EXPECTED3, slab_get(pslab), KEY_DATE, "2017-04-19");
	log_set(&EXPECTED3, slab_get(pslab), KEY_TIME, "18:01:11,437");
	log_set(&EXPECTED3, slab_get(pslab), KEY_LEVEL, "INFO");
	log_set(&EXPECTED3, slab_get(pslab), KEY_THREAD, "Test worker");
	log_set(
	  &EXPECTED3, slab_get(pslab), KEY_CLASS, "core.InstrumentationListener");
	log_set(&EXPECTED3, slab_get(pslab), KEY_MESSAGE,
	  "i do not want to log anything special here");

	log_set(&EXPECTED4, slab_get(pslab), KEY_DATE, "2017-04-19");
	log_set(&EXPECTED4, slab_get(pslab), KEY_TIME, "18:01:11,437");
	log_set(&EXPECTED4, slab_get(pslab), KEY_LEVEL, "INFO");
	log_set(&EXPECTED4, slab_get(pslab), KEY_THREAD, "Test worker");
	log_set(
	  &EXPECTED4, slab_get(pslab), KEY_CLASS, "core.InstrumentationListener");
	log_set(&EXPECTED4, slab_get(pslab), KEY_MESSAGE, "one");
	log_set(&EXPECTED4, slab_get(pslab), KEY_CALLTYPE, "only");

	log_set(&EXPECTED5, slab_get(pslab), KEY_DATE, "2017-11-16");
	log_set(&EXPECTED5, slab_get(pslab), KEY_TIME, "19:07:56,883");
	log_set(&EXPECTED5, slab_get(pslab), KEY_LEVEL, "WARN");
	log_set(&EXPECTED5, slab_get(pslab), KEY_THREAD, "-");
	log_set(&EXPECTED5, slab_get(pslab), KEY_CLASS, "-");
	log_set(&EXPECTED5, slab_get(pslab), "flow", "UpdateClientActivity");
	log_set(&EXPECTED5, slab_get(pslab), "operation", "HandleActiveEvent");
	log_set(&EXPECTED5, slab_get(pslab), "step", "Failure");
	log_set(&EXPECTED5, slab_get(pslab), "lua Rocks", "true");

	log_set(&EXPECTED6, slab_get(pslab), KEY_DATE, "2017-12-05");
	log_set(&EXPECTED6, slab_get(pslab), KEY_TIME, "15:09:09.858");
	log_set(&EXPECTED6, slab_get(pslab), KEY_LEVEL, "WARN");
	log_set(&EXPECTED6, slab_get(pslab), KEY_THREAD, "main");
	log_set(&EXPECTED6, slab_get(pslab), KEY_CLASS, "-");
	log_set(&EXPECTED6, slab_get(pslab), "flow", "");
	log_set(&EXPECTED6, slab_get(pslab), "operation", "closePage");
	log_set(&EXPECTED6, slab_get(pslab), "step", "Failure");
	log_set(&EXPECTED6, slab_get(pslab), "logLevel", "WARN");
	log_set(&EXPECTED6, slab_get(pslab), "url",
	  "https://10.1.6.113:6060/"
	  "index.html?id=5NZM0okZ&wsUri=wss%3A%2F%2F10.1.6.113%3A6060%2Fws%3Fid%"
	  "3D5NZM0okZ");
	log_set(&EXPECTED6, slab_get(pslab), "err",
	  "Error  Protocol error (Target.closeTarget)  Target closed.     at "
	  "Connection._onClose "
	  "(/home/ernestrc/src/tbsip/node_modules/puppeteer/lib/Connection.js 124 "
	  "23)     at emitTwo (events.js 125 13)     at WebSocket.emit (events.js "
	  "213 7)     at WebSocket.emitClose "
	  "(/home/ernestrc/src/tbsip/node_modules/ws/lib/WebSocket.js 213 10)     "
	  "at _receiver.cleanup "
	  "(/home/ernestrc/src/tbsip/node_modules/ws/lib/WebSocket.js 195 41)     "
	  "at Receiver.cleanup "
	  "(/home/ernestrc/src/tbsip/node_modules/ws/lib/Receiver.js 520 15)     "
	  "at WebSocket.finalize "
	  "(/home/ernestrc/src/tbsip/node_modules/ws/lib/WebSocket.js 195 22)     "
	  "at emitNone (events.js 110 20)     at Socket.emit (events.js 207 7)     "
	  "at endReadableNT (_stream_readable.js 1047 12)");
	log_set(&EXPECTED6, slab_get(pslab), "class", "Endpoint");
	log_set(&EXPECTED6, slab_get(pslab), "id", "5NZM0okZ");
	log_set(&EXPECTED6, slab_get(pslab), "timestamp", "1512515349858");
	log_set(&EXPECTED6, slab_get(pslab), "duration", "2.017655998468399");

	log_set(&EXPECTED7, slab_get(pslab), KEY_DATE, "2018-06-05");
	log_set(&EXPECTED7, slab_get(pslab), KEY_TIME, "13:12:12,852");
	log_set(&EXPECTED7, slab_get(pslab), KEY_LEVEL, "INFO");
	log_set(&EXPECTED7, slab_get(pslab), KEY_THREAD, "Grizzly-worker(14)");
	log_set(&EXPECTED7, slab_get(pslab), KEY_CLASS, "v2.ArchiveResource");
	log_set(&EXPECTED7, slab_get(pslab), KEY_MESSAGE, "");

	log_set(&EXPECTED8, slab_get(pslab), KEY_DATE, "2018-05-30");
	log_set(&EXPECTED8, slab_get(pslab), KEY_TIME, "11:01:47,633");
	log_set(&EXPECTED8, slab_get(pslab), KEY_LEVEL, "INFO");
	log_set(&EXPECTED8, slab_get(pslab), KEY_THREAD, "0x7f65b9365700");
	log_set(&EXPECTED8, slab_get(pslab), KEY_CLASS, "registry.ClientBuilder");
	log_set(&EXPECTED8, slab_get(pslab), "callType", "getSupportedFormats");
	log_set(&EXPECTED8, slab_get(pslab), "message",
	  "Ignoring unsupported codec telephone-event ");
	log_set(&EXPECTED8, slab_get(pslab), "connectionId",
	  "139bca64-4480-4727-b241-74699b5a20cc");
	log_set(&EXPECTED8, slab_get(pslab), "partnerId", "100");
	log_set(&EXPECTED8, slab_get(pslab), "publisherId",
	  "72ee1ae2-8c55-4fb6-8cfc-653e8b0618c2");
	log_set(&EXPECTED8, slab_get(pslab), "routerStreamId",
	  "72ee1ae2-8c55-4fb6-8cfc-653e8b0618c2");
	log_set(&EXPECTED8, slab_get(pslab), "sessionId",
	  "2_MX4xMDB-flR1ZSBOb3YgMTkgMTE6MDk6NTggUFNUIDIwMTN-MC4zNzQxNzIxNX4");
	log_set(&EXPECTED8, slab_get(pslab), "streamId",
	  "72ee1ae2-8c55-4fb6-8cfc-653e8b0618c2");
	log_set(&EXPECTED8, slab_get(pslab), "widgetType",
	  "Publisher\t"); // TODO improve space triming to end of property
}

static void __test_print_help(const char* prog)
{
	printf("usage: %s [-v] [-h] [-t]\n", prog);
	printf("\noptions:\n");
	printf("\t-s, --silent: do not output information about tests run\n");
	printf("\t-h, --help: prints this menu\n");
	printf("\t-t, --test: expression to match against tests to run\n");
	printf("\n");
}

static int __test_release(test_ctx_t* ctx)
{
	int failures = ctx->failure;
	free(ctx->test_expr);
	free(BUF8);
	rcmalloc_deinit();
	slab_free(pslab);
	if (failures != 0)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static int __test_ctx_init(test_ctx_t* ctx, int argc, char* argv[])
{
	ctx->test_expr = (char*)malloc(128 * sizeof(char));
	memset(ctx->test_expr, 0, 128);
	ctx->failure = 0;
	ctx->success = 0;
	ctx->silent = 0;

	if (rcmalloc_init(10000)) {
		perror("rcmalloc_init()");
	}

	pslab = slab_create(10000, sizeof(prop_t));
	if (!pslab)
		perror("slab_create()");

	init_test_data();

	while (1) {
		static struct option long_options[] = {
		  {"test", required_argument, 0, 't'}, {"silent", no_argument, 0, 's'},
		  {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

		size_t len;
		int option_index = 0;
		int c = getopt_long(argc, argv, "t:vh", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 't':
			len = strlen(optarg) > 127 ? 127 : strlen(optarg);
			strncpy(ctx->test_expr, optarg, len);
			ctx->test_expr[len] = '\0';
			break;
		case 's':
			ctx->silent = 1;
			break;
		case 'h':
			__test_print_help(argv[0]);
			__test_release(ctx);
			return -11;
		case '?':
			// getopt_long already printed an error message
			return -1;
		default:
			break;
		}
	}

	return 0;
}

#define TEST_RELEASE(ctx) return __test_release(&ctx);

#define TEST_INIT(ctx, argc, argv)                                             \
	if (__test_ctx_init(&ctx, argc, argv)) {                                   \
		return EXIT_SUCCESS;                                                   \
	}

#define TEST_RUN(ctx, test)                                                    \
	if (strlen(ctx.test_expr) == 0 || strstr(#test, ctx.test_expr) != NULL) {  \
		int result = test();                                                   \
		if (result == EXIT_FAILURE) {                                          \
			printf("  TEST\t" #test "\tFAILURE\n");                            \
			ctx.failure++;                                                     \
		} else {                                                               \
			if (!ctx.silent)                                                   \
				printf("  TEST\t" #test "\n");                                 \
			ctx.success++;                                                     \
		}                                                                      \
	}

// compares la with lb. Return will be 0 if all properties are the same and in
// the same order.
int log_cmp(log_t* la, log_t* lb)
{
	static const int bufsize = 10000;
	char loga[bufsize];
	char logb[bufsize];

	DEBUG_ASSERT(la != NULL);
	DEBUG_ASSERT(lb != NULL);

	size_t wanta = snprintl(loga, bufsize, la);
	size_t wantb = snprintl(logb, bufsize, lb);

	// otherwise test will always fail
	ASSERT_EQ(wanta < sizeof(loga), true);
	ASSERT_EQ(wantb < sizeof(logb), true);

	return strcmp(loga, logb);
}

void print_bytes(const char* bytes, size_t size)
{
	size_t i;

	printf("<");
	for (i = 0; i < size; i++) {
		printf("%c", bytes[i]);
	}
	printf(">");
}

#define ASSERT_LOG_EQ(la, lb)                                                  \
	if (log_cmp(la, lb) != 0) {                                                \
		printf(">>>>> '");                                                     \
		printl(la);                                                            \
		printf("'\n VS:\n");                                                   \
		printf(">>>>> '");                                                     \
		printl(lb);                                                            \
		printf("'\n: %d\n", log_cmp(la, lb));                                  \
		return 1;                                                              \
	}

#endif // TEST_TEST_H_
