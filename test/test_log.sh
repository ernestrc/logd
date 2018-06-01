#!/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/log.in"
SCRIPT="$DIR/log.lua"
OUT="$DIR/log.out"
LOGD_EXEC="$DIR/../bin/logd"

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $OUT
	rm -f $IN
	exit $CODE;
}

trap finish EXIT

touch $OUT
touch $IN

cat >$IN << EOF
2018-05-12 12:51:28 ERROR	[thread1]	clazz	a: A, 
2018-05-12 12:52:22 WARN	[thread2]	clazz	callType: b: B
2018-05-12 12:53:22 INFO	[thread3]	clazz	callType: c: C, 
2018-05-12 12:54:22 DEBUG	[thread4]	clazz	callType: b: ,
2018-05-12 12:55:22 TRACE	[thread5]	clazz	callType: b: c: C, 
EOF

cat >$SCRIPT << EOF
local logd = require("logd")
local counter = 0
function logd.on_log(logptr)
	if counter == 0 then
		assert("ERROR" == logd.log_get(logptr, "level"))
		assert("thread1" == logd.log_get(logptr, "thread"))
		assert("clazz" == logd.log_get(logptr, "class"))
		assert(nil == logd.log_get(logptr, "callType"))
		assert("A" == logd.log_get(logptr, "a"))
	elseif counter == 1 then
		assert("WARN" == logd.log_get(logptr, "level"))
		assert("thread2" == logd.log_get(logptr, "thread"))
		assert("clazz" == logd.log_get(logptr, "class"))
		assert("callType" == logd.log_get(logptr, "callType"))
		assert("B" == logd.log_get(logptr, "b"))
	elseif counter == 2 then
		assert("INFO" == logd.log_get(logptr, "level"))
		assert("thread3" == logd.log_get(logptr, "thread"))
		assert("clazz" == logd.log_get(logptr, "class"))
		assert("callType" == logd.log_get(logptr, "callType"))
		assert("C" == logd.log_get(logptr, "c"))
	elseif counter == 3 then
		assert("DEBUG" == logd.log_get(logptr, "level"))
		assert("thread4" == logd.log_get(logptr, "thread"))
		assert("clazz" == logd.log_get(logptr, "class"))
		assert("callType" == logd.log_get(logptr, "callType"))
		assert("" == logd.log_get(logptr, "b"))
	else
		assert("TRACE" == logd.log_get(logptr, "level"))
		assert("thread5" == logd.log_get(logptr, "thread"))
		assert("clazz" == logd.log_get(logptr, "class"))
		assert("callType" == logd.log_get(logptr, "callType"))
		assert("c: C" == logd.log_get(logptr, "b"))
		assert(nil == logd.log_get(logptr, "c"))
	end
	counter = counter + 1
end
function logd.on_eof()
	assert(counter == 5)
end
EOF

cat $IN | $LOGD_EXEC $SCRIPT 2> $OUT 1> $OUT
if [ $? -ne 0 ]; then
	cat $OUT
	exit 1
fi

# using SO parser
cat $IN | $LOGD_EXEC $SCRIPT --parser="$DIR/../lib/parser.so" 2> $OUT 1> $OUT
if [ $? -ne 0 ]; then
	cat $OUT
	exit 1
fi

exit 0
