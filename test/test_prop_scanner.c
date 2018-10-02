#include <errno.h>

#include "../src/config.h"
#include "../src/log.h"
#include "../src/prop_scanner.h"
#include "../src/util.h"
#include "test.h"

struct tcase {
	char* input;
	size_t ilen;
	log_t* expected;
};

#define LOG1                                                                   \
	" \"date\":\"2017-09-07\", \"time\":\"14:54:39,474\",	\"level\":	"         \
	"\"DEBUG\",	\"thread\": \"pool-5-thread-6\",	"                             \
	"\"class\": \"control.RaptorHandler\",	\"callType\": "                     \
	"\"PublisherCreateRequest\", \"flow\": \"Publish\", \"step\": "            \
	"\"Attempt\", \"operation\": \"CreatePublisher\", \"traceId\": "           \
	"\"Publish:Rumor:012ae1a5-3416-4458-b0c1-6eb3e0ab4c80\" \n"
#define LOG2                                                                   \
	"{\"date\":\"2017-09-07\", \"time\":\"14:54:39,474\"	\"level\": "          \
	"\"DEBUG\",	\"thread\": pool-\"5-thread-6,	"                               \
	"\"class\": \"control.RaptorHandler\",	\"sessionId\": "                    \
	"\"1_MX4xMDB-fjE1MDQ4MjEyNzAxMjR-WThtTVpEN0J2c1Z2TlJGcndTN1lpTExGfn4\", "  \
	"\"flow\": \"Publish\", \"conId\": "                                       \
	"\"connectionId: f41973e5-b27c-49e4-bcaf-1d48b153683e\",          "        \
	"\"step\": "                                                               \
	"\"Attempt\", \"publisherId\": "                                           \
	"\"b4da82c4-cac5-4e13-b1dc-bb1f42b475dd\", \"fromAddress\": "              \
	"\"f41973e5-b27c-49e4-bcaf-1d48b153683e\", \"projectId\": 100, "           \
	"\"operation\": "                                                          \
	"\"CreatePublisher\", \"traceId\": "                                       \
	"\"Publish:Rumor:112ae1a5-3416-4458-b0c1-6eb3e0ab4c80\", \"streamId\": "   \
	"\"b4da82c4-cac5-4e13-b1dc-bb1f42b475dd\", \"remoteIpAddress\":    "       \
	"{{{\"}ip\":\"127.\\\"}}0.0.1\"     }}}, "                                 \
	"\"correlationId\": \"b90232b5-3ee5-4c65-bb4e-29286d6a2771\"}\n"
#define LOG3                                                                   \
	"{\"date\":\"2017-04-19\", \"time\":\"18:01:11,437\"     \"level\": "      \
	"\"INFO\", \"thread\":\"Test worker\"    "                                 \
	"\"class\"            :            \"core.InstrumentationListener\"	msg: " \
	"i do not want to log anything special here\n"
#define LOG4                                                                   \
	"date: 2017-04-19, time: \"18:01:11,437\",     \"level\": \"INFO\", "      \
	"thread: "                                                                 \
	"Test worker,    "                                                         \
	"class: core.InstrumentationListener,	callType: \"only\"            "      \
	"msg: one\n"
#define LOG5                                                                   \
	"date: 2017-11-16, time: \"19:07:56,883\",	\"level\": \"WARN\",	"          \
	"thread:\"-\",	class: \"-\"	flow: UpdateClientActivity, "                  \
	"operation: HandleActiveEvent, step: Failure, lua Rocks: true\n"
#define LOG6                                                                   \
	"date: 2017-12-05, time:\"15:09:09.858\", \"level\": \"WARN\", thread: "   \
	"main, "                                                                   \
	"class: -, flow: , "                                                       \
	"operation: closePage, "                                                   \
	"step: Failure, logLevel: WARN, url: "                                     \
	"https://10.1.6.113:6060/"                                                 \
	"index.html?id=5NZM0okZ&wsUri=wss%3A%2F%2F10.1.6.113%3A6060%2Fws%3Fid%"    \
	"3D5NZM0okZ, err: Error  Protocol error (Target.closeTarget)  Target "     \
	"closed.     at Connection._onClose "                                      \
	"(/home/ernestrc/src/tbsip/node_modules/puppeteer/lib/Connection.js 124 "  \
	"23)     at emitTwo (events.js 125 13)     at WebSocket.emit (events.js "  \
	"213 7)     at WebSocket.emitClose "                                       \
	"(/home/ernestrc/src/tbsip/node_modules/ws/lib/WebSocket.js 213 10)     "  \
	"at _receiver.cleanup "                                                    \
	"(/home/ernestrc/src/tbsip/node_modules/ws/lib/WebSocket.js 195 41)     "  \
	"at Receiver.cleanup "                                                     \
	"(/home/ernestrc/src/tbsip/node_modules/ws/lib/Receiver.js 520 15)     "   \
	"at WebSocket.finalize "                                                   \
	"(/home/ernestrc/src/tbsip/node_modules/ws/lib/WebSocket.js 195 22)     "  \
	"at emitNone (events.js 110 20)     at Socket.emit (events.js 207 7)     " \
	"at endReadableNT (_stream_readable.js 1047 12), class: Endpoint, id: "    \
	"5NZM0okZ, timestamp: 1512515349858, duration: 2.017655998468399\n"

#define LOG7                                                                   \
	"date: 2018-06-05, time:\"13:12:12,852\",	 \"level\": \"INFO\",	"          \
	"thread: Grizzly-worker(14),	"                                             \
	"class: v2.ArchiveResource,	msg:\n"

#define LOG8                                                                   \
	"\"date\": 2018-05-30, time: \"11:01:47,633\",	 \"level\": \"INFO\",	"     \
	"thread: 0x7f65b9365700,	"                                                 \
	"class: registry.ClientBuilder,	callType: getSupportedFormats, message: "  \
	"Ignoring "                                                                \
	"unsupported codec telephone-event , connectionId: "                       \
	"139bca64-4480-4727-b241-74699b5a20cc, partnerId: 100, publisherId: "      \
	"72ee1ae2-8c55-4fb6-8cfc-653e8b0618c2, routerStreamId: "                   \
	"72ee1ae2-8c55-4fb6-8cfc-653e8b0618c2, sessionId: "                        \
	"2_MX4xMDB-flR1ZSBOb3YgMTkgMTE6MDk6NTggUFNUIDIwMTN-MC4zNzQxNzIxNX4, "      \
	"streamId: 72ee1ae2-8c55-4fb6-8cfc-653e8b0618c2, widgetType: Publisher	"   \
	"\ndate: 2018-05-30, time: \"11:01:47,633\",	 \"level\": \"INFO\",	"       \
	"\"thread\": \"0x7f65b"

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

static void init_data()
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
	init_test_data();
}

int test_scanner_create()
{
	prop_scanner_t* p = scanner_create();
	ASSERT_NEQ(p->pslab, NULL);
	scanner_free(p);

	return 0;
}

int test_scan_partial2()
{
	scan_res_t res;
	prop_scanner_t* p = scanner_create();

	char* chunk1 = BUF8;
	size_t len1 = 212;

	char* chunk2 = BUF8 + len1;
	size_t len2 = LEN8 - len1;

	res = scanner_scan(p, chunk1, len1);
	ASSERT_EQ(res.type, SCAN_PARTIAL);
	ASSERT_EQ(res.consumed, len1);

	res = scanner_scan(p, chunk2, len2);
	ASSERT_EQ(res.type, SCAN_COMPLETE);
	ASSERT_EQ(res.consumed, 310);

	ASSERT_LOG_EQ(res.log, &EXPECTED8);

	return 0;
}

int test_scan_partial1()
{

	scan_res_t res;
	prop_scanner_t* p = scanner_create();

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

	// printf("scan: '%.*s'\n", (int)len1, chunk1);
	res = scanner_scan(p, chunk1, len1);
	ASSERT_EQ(res.type, SCAN_PARTIAL);
	ASSERT_EQ(res.consumed, len1);

	// printf("scan: '%.*s'\n", (int)len2, chunk2);
	res = scanner_scan(p, chunk2, len2);
	ASSERT_EQ(res.type, SCAN_PARTIAL);
	ASSERT_EQ(res.consumed, len2);

	// printf("scan: '%.*s'\n", (int)len3, chunk3);
	res = scanner_scan(p, chunk3, len3);
	ASSERT_EQ(res.type, SCAN_PARTIAL);
	ASSERT_EQ(res.consumed, len3);

	// printf("scan: '%.*s'\n", (int)len4, chunk4);
	res = scanner_scan(p, chunk4, len4);
	ASSERT_EQ(res.type, SCAN_PARTIAL);
	ASSERT_EQ(res.consumed, len4);

	// printf("scan: '%.*s'\n", (int)len5, chunk5);
	res = scanner_scan(p, chunk5, len5);
	ASSERT_EQ(res.type, SCAN_COMPLETE);
	ASSERT_EQ(res.consumed, len5);

	ASSERT_LOG_EQ(res.log, &EXPECTED1);

	return 0;
}

#define ELOG1 "msg: value, hee\n"
#define ELEN1 strlen(ELOG1)

#define ELOG3 "\"hey\" \n"
#define ELEN3 strlen(ELOG3)
int test_scan_error()
{

	char* ERR1 = malloc(ELEN1 + 1);
	ASSERT_NEQ(ERR1, NULL);
	memcpy(ERR1, ELOG1, ELEN1);

	char* ERR3 = malloc(ELEN3);
	ASSERT_NEQ(ERR3, NULL);
	memcpy(ERR3, ELOG3, ELEN3);

	prop_scanner_t* p = scanner_create();
	scan_res_t res;

	res = scanner_scan(p, ERR1, ELEN1);
	ASSERT_EQ(res.type, SCAN_ERROR);
	ASSERT_EQ(res.consumed, ELEN1);
	ASSERT_EQ(res.error.msg, "invalid log");
	ASSERT_STR_EQ(res.error.at, "hee");
	log_t* log1 = log_create();
	ASSERT_EQ(log1->props, NULL);
	log_set(log1, slab_get(pslab), "msg", "value");

	ASSERT_LOG_EQ(res.log, log1);
	scanner_reset(p);

	res = scanner_scan(p, ERR3, ELEN3);
	ASSERT_EQ(res.type, SCAN_ERROR);
	ASSERT_EQ(res.consumed, ELEN3);
	ASSERT_STR_EQ(res.error.msg, "invalid log value");
	ASSERT_STR_EQ(res.error.at, "");
	scanner_reset(p);

	// check that we're able to scan after errors have been consumed
	res = scanner_scan(p, BUF6, LEN6);
	ASSERT_EQ(res.type, SCAN_COMPLETE);
	ASSERT_LOG_EQ(res.log, &EXPECTED6);
	scanner_reset(p);

	free(ERR1);

	return 0;
}

int test_scan_multiple()
{
	prop_scanner_t* p = scanner_create();
	scan_res_t res;

	/* feed partial data */
	res = scanner_scan(p, BUF7, LEN7 - 2);
	ASSERT_EQ(res.type, SCAN_PARTIAL);
	ASSERT_EQ(res.consumed, LEN7 - 2);
	scanner_reset(p);

	/* check that we are able to re-scan the data */
	res = scanner_scan(p, BUF7, LEN7);
	ASSERT_NEQ(res.type, SCAN_ERROR);
	ASSERT_EQ(res.type, SCAN_COMPLETE);
	ASSERT_EQ(res.consumed, LEN7);
	ASSERT_LOG_EQ(res.log, &EXPECTED7);

	return 0;
}

int test_scan_reset()
{
	prop_scanner_t* p = scanner_create();
	static const size_t CASES_LEN = 4;
	struct tcase CASES[CASES_LEN];
	CASES[0] = (struct tcase){BUF3, LEN3, &EXPECTED3};
	CASES[1] = (struct tcase){BUF2, LEN2, &EXPECTED2};
	CASES[2] = (struct tcase){BUF4, LEN4, &EXPECTED4};
	CASES[3] = (struct tcase){BUF5, LEN5, &EXPECTED5};

	for (int i = 0; i < CASES_LEN; i++) {
		struct tcase test = CASES[i];
		scan_res_t res = scanner_scan(p, test.input, test.ilen);
		ASSERT_EQ(res.type, SCAN_COMPLETE);
		ASSERT_LOG_EQ(res.log, test.expected);
		scanner_reset(p);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	test_ctx_t ctx;
	TEST_INIT(ctx, argc, argv);

	init_data();

	TEST_RUN(ctx, test_scanner_create);
	TEST_RUN(ctx, test_scan_partial1);
	TEST_RUN(ctx, test_scan_partial2);
	TEST_RUN(ctx, test_scan_reset);
	TEST_RUN(ctx, test_scan_multiple);
	TEST_RUN(ctx, test_scan_error);

	TEST_RELEASE(ctx);
	free(BUF8);
}
