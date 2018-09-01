#include <errno.h>

#include "../src/config.h"
#include "../src/log.h"
#include "../src/parser.h"
#include "../src/default_parser.h"
#include "../src/util.h"
#include "test.h"

struct tcase {
	char* input;
	size_t ilen;
	log_t* expected;
};

#define LOG1                                                                   \
	"2017-09-07 14:54:39,474	DEBUG	[pool-5-thread-6]	"                         \
	"control.RaptorHandler	PublisherCreateRequest: flow: Publish, step: "      \
	"Attempt, operation: CreatePublisher, traceId: "                           \
	"Publish:Rumor:012ae1a5-3416-4458-b0c1-6eb3e0ab4c80\n"
#define LOG2                                                                   \
	"2017-09-07 14:54:39,474	DEBUG	[pool-\"5-thread-6]	"                       \
	"control.RaptorHandler	sessionId: "                                        \
	"1_MX4xMDB-fjE1MDQ4MjEyNzAxMjR-WThtTVpEN0J2c1Z2TlJGcndTN1lpTExGfn4, "      \
	"flow: Publish, conId: connectionId: "                                     \
	"f41973e5-b27c-49e4-bcaf-1d48b153683e,          step: Attempt, "           \
	"publisherId: "                                                            \
	"b4da82c4-cac5-4e13-b1dc-bb1f42b475dd, fromAddress: "                      \
	"f41973e5-b27c-49e4-bcaf-1d48b153683e, projectId: 100, operation: "        \
	"CreatePublisher, traceId: "                                               \
	"Publish:Rumor:112ae1a5-3416-4458-b0c1-6eb3e0ab4c80, streamId: "           \
	"b4da82c4-cac5-4e13-b1dc-bb1f42b475dd, remoteIpAddress:    "               \
	"{{{\"}ip\":\"127.\\\"}}0.0.1\"     }}}, "                                 \
	"correlationId: b90232b5-3ee5-4c65-bb4e-29286d6a2771\n"
#define LOG3                                                                   \
	"2017-04-19 18:01:11,437     INFO [Test worker]    "                       \
	"core.InstrumentationListener	i do not want to log anything special "      \
	"here\n"
#define LOG4                                                                   \
	"2017-04-19 18:01:11,437     INFO [Test worker]    "                       \
	"core.InstrumentationListener	only:            one\n"
#define LOG5                                                                   \
	"2017-11-16 19:07:56,883	WARN	[-]	-	flow: UpdateClientActivity, "          \
	"operation: HandleActiveEvent, step: Failure, lua Rocks: true\n"
#define LOG6                                                                   \
	"[2017-12-05T15:09:09.858] [WARN] main - flow: , operation: closePage, "   \
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
	"2018-06-05 13:12:12,852	 INFO	[Grizzly-worker(14)]	"                      \
	"v2.ArchiveResource	\n"

#define LOG8                                                                   \
	"2018-05-30 11:01:47,633	 INFO	[0x7f65b9365700]	"                          \
	"registry.ClientBuilder	getSupportedFormats: message: Ignoring "           \
	"unsupported codec telephone-event , connectionId: "                       \
	"139bca64-4480-4727-b241-74699b5a20cc, partnerId: 100, publisherId: "      \
	"72ee1ae2-8c55-4fb6-8cfc-653e8b0618c2, routerStreamId: "                   \
	"72ee1ae2-8c55-4fb6-8cfc-653e8b0618c2, sessionId: "                        \
	"2_MX4xMDB-flR1ZSBOb3YgMTkgMTE6MDk6NTggUFNUIDIwMTN-MC4zNzQxNzIxNX4, "      \
	"streamId: 72ee1ae2-8c55-4fb6-8cfc-653e8b0618c2, widgetType: Publisher	"   \
	"\n2018-05-30 11:01:47,633	 INFO	[0x7f65b"

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

int test_parser_create()
{
	parser_t* p = (parser_t*)parser_create();
	ASSERT_NEQ(p->pslab, NULL);
	parser_free(p);

	return 0;
}

int test_parse_partial2()
{
	parse_res_t res;
	parser_t* p = (parser_t*)parser_create();

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
	parser_t* p = (parser_t*)parser_create();

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
	for (int i = 0; i < LOGD_SLAB_CAP; i++) {
		char* prop_key = rcmalloc(100);
		ASSERT_NEQ(prop_key, NULL);
		sprintf(prop_key, "%d", i);
		prop_t* prop = slab_get(pslab);
		ASSERT_NEQ(prop, NULL);
		log_set(big_log, prop, prop_key, NULL);
	}
	size_t ELEN2 = snprintl(ERR2, BLEN2, big_log);

	parser_t* p = (parser_t*)parser_create();
	parse_res_t res;

	res = parser_parse(p, ERR1, ELEN1);
	ASSERT_EQ(res.type, PARSE_ERROR);
	ASSERT_EQ(res.consumed, ELEN1);
	ASSERT_EQ(res.error.msg, "invalid date or time in log header");
	ASSERT_STR_EQ(
	  res.error.at, "msg: this log does not have a header, subject: idiot");
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
	ASSERT_STR_EQ(res.error.msg,
	  "reached max number of log properties: " STR(LOGD_SLAB_CAP));
	ASSERT_STR_EQ(
	  res.error.at, " 5: null, 4: null, 3: null, 2: null, 1: null, 0: null, ");

	// prepare log comparison
	log_remove(big_log, "0");
	log_remove(big_log, "1");
	log_remove(big_log, "2");
	log_remove(big_log, "3");
	log_remove(big_log, "4");
	log_remove(big_log, "5");
	for (int i = 0; i < LOGD_SLAB_CAP; i++) {
		char* prop_key = rcmalloc(100);
		ASSERT_NEQ(prop_key, NULL);
		sprintf(prop_key, "%d", i);
		slab_put(pslab, log_remove(big_log, prop_key));
	}
	for (int i = LOGD_SLAB_CAP - 1; i >= 6; i--) {
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
	ASSERT_STR_EQ(res.error.at, "");
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
	parser_t* p = (parser_t*)parser_create();
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
	parser_t* p = (parser_t*)parser_create();
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

	init_data();

	TEST_RUN(ctx, test_parser_create);
	TEST_RUN(ctx, test_parse_reset);
	TEST_RUN(ctx, test_parse_partial1);
	TEST_RUN(ctx, test_parse_partial2);
	TEST_RUN(ctx, test_parse_multiple);
	TEST_RUN(ctx, test_parse_error);

	TEST_RELEASE(ctx);
	free(BUF8);
}
