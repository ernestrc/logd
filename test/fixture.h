#ifndef LOGD_TEST_FIXTURE_H
#define LOGD_TEST_FIXTURE_H
#define LOG1                                                                   \
	"2017-09-07 14:54:39,474	DEBUG	[pool-5-thread-6]	"                         \
	"control.RaptorHandler	PublisherCreateRequest: flow: Publish, step: "      \
	"Attempt, operation: CreatePublisher, traceId: "                           \
	"Publish:Rumor:012ae1a5-3416-4458-b0c1-6eb3e0ab4c80\n"
#define LOG2                                                                   \
	"2017-09-07 14:54:39,474	DEBUG	[pool-5-thread-6]	"                         \
	"control.RaptorHandler	sessionId: "                                        \
	"1_MX4xMDB-fjE1MDQ4MjEyNzAxMjR-WThtTVpEN0J2c1Z2TlJGcndTN1lpTExGfn4, "      \
	"flow: Publish, conId: connectionId: "                                     \
	"f41973e5-b27c-49e4-bcaf-1d48b153683e,          step: Attempt, "           \
	"publisherId: "                                                            \
	"b4da82c4-cac5-4e13-b1dc-bb1f42b475dd, fromAddress: "                      \
	"f41973e5-b27c-49e4-bcaf-1d48b153683e, projectId: 100, operation: "        \
	"CreatePublisher, traceId: "                                               \
	"Publish:Rumor:112ae1a5-3416-4458-b0c1-6eb3e0ab4c80, streamId: "           \
	"b4da82c4-cac5-4e13-b1dc-bb1f42b475dd, remoteIpAddress:    127.0.0.1, "    \
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

#endif
