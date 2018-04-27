#include <errno.h>

#include "test.h"
#include "../src/parser.h"
#include "../src/log.h"

static slab_t* pslab;

static const char* LOG1 = "2017-09-07 14:54:39,474	DEBUG	[pool-5-thread-6]	control.RaptorHandler	PublisherCreateRequest: flow: Publish, step: Attempt, operation: CreatePublisher, traceId: Publish:Rumor:012ae1a5-3416-4458-b0c1-6eb3e0ab4c80\n";
static char* BUF1;
// const char* LOG2 = "2017-09-07 14:54:39,474	DEBUG	[pool-5-thread-6]	control.RaptorHandler	sessionId: 1_MX4xMDB-fjE1MDQ4MjEyNzAxMjR-WThtTVpEN0J2c1Z2TlJGcndTN1lpTExGfn4, flow: Publish, conId: connectionId: f41973e5-b27c-49e4-bcaf-1d48b153683e, step: Attempt, publisherId: b4da82c4-cac5-4e13-b1dc-bb1f42b475dd, fromAddress: f41973e5-b27c-49e4-bcaf-1d48b153683e, projectId: 100, operation: CreatePublisher, traceId: Publish:Rumor:112ae1a5-3416-4458-b0c1-6eb3e0ab4c80, streamId: b4da82c4-cac5-4e13-b1dc-bb1f42b475dd, remoteIpAddress: 127.0.0.1, correlationId: b90232b5-3ee5-4c65-bb4e-29286d6a2771\n";
// const char* LOG3 = "2017-04-19 18:01:11,437     INFO [Test worker]    core.InstrumentationListener	i do not want to log anything special here\n";
// const char* LOG4 = "2017-04-19 18:01:11,437     INFO [Test worker]    core.InstrumentationListener	only: one\n";
// const char* LOG5 = "2017-11-16 19:07:56,883	WARN	[-]	-	flow: UpdateClientActivity, operation: HandleActiveEvent, step: Failure, luaRocks: true\n";
// const char* LOG6 = "[2017-12-05T15:09:09.858] [WARN] main - flow: , operation: closePage, step: Failure, logLevel: WARN, url: https://10.1.6.113:6060/index.html?id=5NZM0okZ&wsUri=wss%3A%2F%2F10.1.6.113%3A6060%2Fws%3Fid%3D5NZM0okZ, err: Error  Protocol error (Target.closeTarget)  Target closed.     at Connection._onClose (/home/ernestrc/src/tbsip/node_modules/puppeteer/lib/Connection.js 124 23)     at emitTwo (events.js 125 13)     at WebSocket.emit (events.js 213 7)     at WebSocket.emitClose (/home/ernestrc/src/tbsip/node_modules/ws/lib/WebSocket.js 213 10)     at _receiver.cleanup (/home/ernestrc/src/tbsip/node_modules/ws/lib/WebSocket.js 195 41)     at Receiver.cleanup (/home/ernestrc/src/tbsip/node_modules/ws/lib/Receiver.js 520 15)     at WebSocket.finalize (/home/ernestrc/src/tbsip/node_modules/ws/lib/WebSocket.js 195 22)     at emitNone (events.js 110 20)     at Socket.emit (events.js 207 7)     at endReadableNT (_stream_readable.js 1047 12), class: Endpoint, id: 5NZM0okZ, timestamp: 1512515349858, duration: 2.017655998468399\n";

log_t EXPECTED1;
//log_t* EXPECTED2;
//log_t* EXPECTED3;
//log_t* EXPECTED4;
//log_t* EXPECTED5;
//log_t* EXPECTED6;

void init() {
	pslab = slab_create(1000, sizeof(prop_t));

	if (!pslab)
		perror("slab_create()");
 
	BUF1 = rcmalloc(strlen(LOG1));
	memcpy(BUF1, LOG1, strlen(LOG1));

	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR(KEY_TIME), MAKE_STR("14:54:39,474"));
	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR(KEY_DATE), MAKE_STR("2017-09-07"));
	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR(KEY_LEVEL), MAKE_STR("DEBUG"));
	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR(KEY_THREAD), MAKE_STR("pool-5-thread-6"));
	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR(KEY_THREAD), MAKE_STR("control.RaptorHandler"));
	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR("callType"), MAKE_STR("PublisherCreateRequest"));
	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR("flow"), MAKE_STR("Publish"));
	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR("step"), MAKE_STR("Attempt"));
	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR("operation"), MAKE_STR("CreatePublisher"));
	log_set(&EXPECTED1, slab_get(pslab), MAKE_STR("traceId"), MAKE_STR("Publish:Rumor:012ae1a5-3416-4458-b0c1-6eb3e0ab4c80"));
}

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
	size_t CASES_LEN = 1;
	struct tcase CASES[CASES_LEN];
	CASES[0] = (struct tcase){BUF1, strlen(LOG1), &EXPECTED1};

	for (int i = 0; i < CASES_LEN; i++) {
		struct tcase test = CASES[i];
		presult_t res = parser_parse(p, test.input, test.ilen);
		ASSERT_EQ(res.complete, true);
		ASSERT_EQ(res.next, NULL);
		ASSERT_EQ(log_cmp(&p->result, test.expected), 0);
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
